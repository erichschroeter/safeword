#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <polarssl/aes.h>
#include <polarssl/sha256.h>

#include "security.h"

static void _resolve_key(struct safeword_security_spec *spec)
{
    int n;
    FILE *fkey;
    unsigned char key[512];
    char *p;

    /*
     * Read the secret key and clean the command line.
     */
    if (spec->key == NULL)
	    return;

    if( ( fkey = fopen( spec->key, "rb" ) ) != NULL )
    {
        spec->keylen = fread( key, 1, sizeof( key ), fkey );
        fclose( fkey );
    }
    else
    {
        if(memcmp( spec->key, "hex:", 4 ) == 0 )
        {
            p = &spec->key[4];
            spec->keylen = 0;

            while( sscanf( p, "%02X", &n ) > 0 &&
                   spec->keylen < (int) sizeof( key ) )
            {
                key[spec->keylen++] = (unsigned char) n;
                p += 2;
            }
        }
        else
        {
            spec->keylen = strlen( spec->key );

            if( spec->keylen > (int) sizeof( key ) )
                spec->keylen = (int) sizeof( key );

            memcpy( key, spec->key, spec->keylen );
        }
    }

    /* Clear the key */
    /*memset( spec->key, 0, strlen( spec->key ) );*/

}

int safeword_encrypt(struct safeword_security_spec *spec, char *data, int datalen, char **encrypted, int *encrypted_size)
{
	int i, n, offset, data_offset;
	int lastn;
	char *p;
	unsigned char IV[16];
	unsigned char digest[32];
	unsigned char buffer[1024];

	aes_context aes_ctx;
	sha256_context sha_ctx;

	/*
	* Read the secret key and clean the command line.
	*/
	if (spec->key == NULL)
		goto exit;

	/* Resolve the key to get the keylen */
	_resolve_key(spec);

	*encrypted = calloc(datalen + 256, sizeof(char));
	if (!*encrypted) {
		fprintf(stderr, "Couldn't alloc encrypt memory.\n");
		goto exit;
	}
	*encrypted_size = 0;

	/*
	 * Generate the initialization vector as:
	 * IV = SHA-256( datalen || data )[0..15]
	 */
	for( i = 0; i < 8; i++ )
		buffer[i] = (unsigned char)( datalen >> ( i << 3 ) );

	/*p = (char *) data;*/
	p = (char *) spec->key;

	sha256_starts(&sha_ctx, 0);
	sha256_update(&sha_ctx, buffer, 8);
	sha256_update(&sha_ctx, (unsigned char *) p, strlen(p));
	sha256_finish(&sha_ctx, digest);

	memcpy(IV, digest, 16);

	/*
	 * The last four bits in the IV are actually used
	 * to store the data size modulo the AES block size.
	 */
	lastn = (int)(datalen & 0x0F);

	IV[15] = (unsigned char) ((IV[15] & 0xF0 ) | lastn);

	/*
	 * Append the IV at the beginning of the output.
	 */
	memcpy(*encrypted, IV, 16);
	*encrypted_size += 16;

	/*
	 * Hash the IV and the secret key together 8192 times
	 * using the result to setup the AES context and HMAC.
	 */
	memset(digest, 0,  32);
	memcpy(digest, IV, 16);

	for (i = 0; i < 8192; i++)
	{
		sha256_starts(&sha_ctx, 0);
		sha256_update(&sha_ctx, digest, 32);
		sha256_update(&sha_ctx, (unsigned char*) spec->key, spec->keylen);
		sha256_finish(&sha_ctx, digest);
	}

	/*memset(spec->key, 0, spec->keylen);*/
	aes_setkey_enc(&aes_ctx, digest, 256);
	sha256_hmac_starts(&sha_ctx, digest, 32, 0);

	data_offset = 0;
	/*
	 * Encrypt and write the ciphertext.
	 */
	for (offset = 0; offset < datalen; offset += 16)
	{
		n = (datalen - offset > 16) ? 16 : (int) (datalen - offset);

		memcpy(buffer, (data + data_offset), n);
		data_offset += n;

		for (i = 0; i < 16; i++)
			buffer[i] = (unsigned char)(buffer[i] ^ IV[i]);

		aes_crypt_ecb(&aes_ctx, AES_ENCRYPT, buffer, buffer);
		sha256_hmac_update(&sha_ctx, buffer, 16);

		memcpy((*encrypted) + (*encrypted_size), buffer, 16);
		*encrypted_size += 16;

		memcpy(IV, buffer, 16);
	}

	/*
	 * Finally write the HMAC.
	 */
	sha256_hmac_finish(&sha_ctx, digest);

	memcpy((*encrypted) + (*encrypted_size), digest, 32);
	*encrypted_size += 32;

exit:
	memset(&digest, 0, sizeof(digest));
	memset(&buffer, 0, sizeof(buffer));

	memset(&aes_ctx, 0, sizeof(aes_context));
	memset(&sha_ctx, 0, sizeof(sha256_context));

	return 0;
}

int safeword_decrypt(struct safeword_security_spec *spec, char *data, int datalen, char **decrypted, int *decrypted_size)
{
	int i, n;
	int lastn, offset, data_offset;
	unsigned char IV[16];
	unsigned char digest[32];
	unsigned char buffer[1024];
	unsigned char diff;

	aes_context aes_ctx;
	sha256_context sha_ctx;

	/*
	* Read the secret key and clean the command line.
	*/
	if (spec->key == NULL)
		goto exit;

	/* Resolve the key to get the keylen */
	_resolve_key(spec);

	*decrypted = calloc(datalen, sizeof(char));
	if (!(*decrypted)) {
		perror("decrypted calloc");
		goto exit;
	}
	*decrypted_size = 0;

        unsigned char tmp[16];

	/*
	 *  The encrypted file must be structured as follows:
	 *
	 *        00 .. 15              Initialization Vector
	 *        16 .. 31              AES Encrypted Block #1
	 *           ..
	 *      N*16 .. (N+1)*16 - 1    AES Encrypted Block #N
	 *  (N+1)*16 .. (N+1)*16 + 32   HMAC-SHA-256(ciphertext)
	 */
	if (datalen < 48)
	{
		fprintf(stderr, "File too short to be encrypted.\n");
		goto exit;
	}

	if ((datalen & 0x0F) != 0)
	{
		fprintf(stderr, "File size not a multiple of 16.\n");
		goto exit;
	}

	/*
	 * Subtract the IV + HMAC length.
	 */
	datalen -= (16 + 32);

	data_offset = 0;
	/*
	 * Read the IV and original data size modulo 16.
	 */
	memcpy(buffer, data, 16);
	data_offset += 16;

	memcpy(IV, buffer, 16);
	lastn = IV[15] & 0x0F;

	/*
	 * Hash the IV and the secret key together 8192 times
	 * using the result to setup the AES context and HMAC.
	 */
	memset(digest, 0,  32);
	memcpy(digest, IV, 16);

	for (i = 0; i < 8192; i++)
	{
		sha256_starts(&sha_ctx, 0);
		sha256_update(&sha_ctx, digest, 32);
		sha256_update(&sha_ctx, (unsigned char*) spec->key, spec->keylen);
		sha256_finish(&sha_ctx, digest);
	}

	/*memset(spec->key, 0, spec->keylen);*/
	aes_setkey_dec(&aes_ctx, digest, 256);
	sha256_hmac_starts(&sha_ctx, digest, 32, 0);

	/*
	 * Decrypt and write the plaintext.
	 */
	for (offset = 0; offset < datalen; offset += 16)
	{
		memcpy(buffer, (data + data_offset), 16);
		data_offset += 16;

		memcpy(tmp, buffer, 16);

		sha256_hmac_update(&sha_ctx, buffer, 16);
		aes_crypt_ecb(&aes_ctx, AES_DECRYPT, buffer, buffer);

		for(i = 0; i < 16; i++)
			buffer[i] = (unsigned char)(buffer[i] ^ IV[i]);

		memcpy(IV, tmp, 16);

		n = (lastn > 0 && offset == datalen - 16) ? lastn : 16;

		memcpy((*decrypted) + (*decrypted_size), buffer, n);
		*decrypted_size += n;
	}

	/*
	 * Verify the message authentication code.
	 */
	sha256_hmac_finish(&sha_ctx, digest);

	memcpy(buffer, (data + data_offset), 32);
	data_offset += 32;

	/* Use constant-time buffer comparison */
	diff = 0;
	for (i = 0; i < 32; i++)
		diff |= digest[i] ^ buffer[i];

	if (diff != 0)
	{
		fprintf(stderr, "HMAC check failed: wrong key, or file corrupted.\n");
		goto exit;
	}

exit:
	memset(&digest, 0, sizeof(digest));
	memset(&buffer, 0, sizeof(buffer));

	memset(&aes_ctx, 0, sizeof(aes_context));
	memset(&sha_ctx, 0, sizeof(sha256_context));

	return 0;
}

