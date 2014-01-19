#ifndef SAFEWORD_SECURITY_H
#define SAFEWORD_SECURITY_H

struct safeword_security_spec {
	/** A string identifier for the method of encryption. */
	char *name;
	/** The encryption key. (string, hex, file) */
	char *key;
	size_t keylen;
	/** Any method-specific data required for @c method. */
	void *data;
	/**
	 * An optional nested encryption method. This is used for chaining
	 * encryption.
	 */
	struct safeword_security_spec *spec;
};

int safeword_encrypt(struct safeword_security_spec *spec, char *buf, int size, char **encrypted, int *encrypted_size);
int safeword_decrypt(struct safeword_security_spec *spec, char *encrypted, int encrypted_size, char **buf, int *size);

#endif /* SAFEWORD_SECURITY_H */
