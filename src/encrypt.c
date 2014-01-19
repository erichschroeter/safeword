#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <sys/types.h>

#include "security.h"

#define USAGE "encrypt [-k <key>] [--decrypt] <in> <out>"

/*
 * Usage:
 *   encrypt [-k <key>] <in> <out>
 */
int main(int argc, char **argv)
{
	struct safeword_security_spec spec;
	int ret, c, n, decrypt = 0, bufsize;
	long buflen;
	int outputlen;
	char *buf, *output;
	size_t keylen;
	unsigned char key[512];
	FILE *fkey, *fin, *fout;
	char *ptr;
	struct option long_options[] = {
		{"key", required_argument, NULL, 'k'},
		{"decrypt", no_argument, NULL, 'd'},
	};

	memset(&spec, 0, sizeof(spec));

	while ((c = getopt_long(argc, argv, "dk:", long_options, 0)) != -1) {
		switch (c) {
		case 'd':
			decrypt = 1;
			break;
		case 'k':
			/*if ((fkey = fopen(optarg, "rb")) != NULL) {*/
				/*keylen = fread(key, 1, sizeof(key), fkey);*/
			/*} else {*/
				/*if (memcmp(optarg, "hex:", 4) == 0) {*/
					/*ptr = &optarg[4];*/
					/*keylen = 0;*/

					/*while (sscanf(ptr, "%02X", &n) > 0 &&*/
						/*keylen < (int) sizeof(key)) {*/
						/*key[keylen++] = (unsigned char) n;*/
						/*ptr += 2;*/
					/*}*/
				/*} else {*/
					/*keylen = strlen(optarg);*/

					/*if (keylen > (int) sizeof(key))*/
						/*keylen = (int) sizeof(key);*/

					/*memcpy(key, optarg, keylen);*/
				/*}*/
			/*}*/

			spec.key = calloc(strlen(optarg) + 1, sizeof(char));
			memcpy(spec.key, optarg, strlen(optarg));

			memset(optarg, 0, strlen(optarg));
			break;
		default:
			fprintf(stderr, "%s\n", USAGE);
			return 1;
		}
	}

	if ((argc - optind) < 2) {
		fprintf(stderr, "%s\n", USAGE);
		return 1;
	}

	if ((fin = fopen(argv[optind], "rb")) != NULL) {
		long filesize, bread;
		/* Encrypting contents of file. */
		if( ( filesize = lseek( fileno( fin ), 0, SEEK_END ) ) < 0 )
		{
			perror( "lseek" );
			return 1;
		}
		/* reset file pointer */
		if( fseek( fin, 0, SEEK_SET ) < 0 )
		{
			fprintf( stderr, "fseek(0,SEEK_SET) failed\n" );
			return 1;
		}
		buf = calloc(filesize, sizeof(char));
		if (!buf) {
			perror("calloc");
			return 1;
		}
		bread = fread(buf, 1, filesize, fin);
		if (bread != filesize) {
			fprintf(stderr, "Could not read entire input file.\n");
			return 1;
		}
		buflen = filesize;

		fclose(fin);
	} else {
		/* Encrypting plain text from command line. */
		buflen = strlen(argv[optind]) + 1;
		buf = calloc(buflen, sizeof(char));
		if (!buf) {
			perror("calloc");
			return 1;
		}
		memcpy(buf, argv[optind], buflen);
	}
	optind++;

	if ((fout = fopen(argv[optind], "wb+")) == NULL) {
		perror("fout");
		return 1;
	}

	if (decrypt) {
		ret = safeword_decrypt(&spec, buf, buflen, &output, &outputlen);
	} else {
		ret = safeword_encrypt(&spec, buf, buflen, &output, &outputlen);
	}
	/*printf("outputlen: %d\n", outputlen);*/

	if (ret) {
		fprintf(stderr, "Safeword Error: %d\n", ret);
		return 1;
	}

	int w = fwrite(output, 1, outputlen, fout);
	/*printf("written: %d\n", w);*/

	/*printf("Unencrypted\n");*/
	/*fwrite(buf, 1, sizeof(buf), stdout);*/
	/*printf("\n");*/
	/*printf("Encrypted\n");*/
	/*FILE *out = fopen("/home/erich/src/safeword/build/text.aes", "wb+");*/
	/*if (!out) {*/
		/*perror("fopen");*/
	/*}*/
	/*fwrite(enc, 1, sizeof(enc), out);*/
	/*[>fwrite(enc, 1, sizeof(enc), stdout);<]*/
	/*printf("\n");*/

	fclose(fout);

	return 0;
}
