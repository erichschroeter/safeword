#ifndef SAFEWORD_H
#define SAFEWORD_H

#define SAFEWORD_VERSION_MAJOR 0
#define SAFEWORD_VERSION_MINOR 1
#define SAFEWORD_VERSION_PATCH 0
#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)
#define SAFEWORD_VERSION STR(SAFEWORD_VERSION_MAJOR) "." STR(SAFEWORD_VERSION_MINOR) "." STR(SAFEWORD_VERSION_PATCH)

#include <errno.h>
#include <sqlite3.h>

#define ESAFEWORD_DBEXIST    ENOENT /* safeword db does not exist */
#define ESAFEWORD_INVARG     256    /* invalid argument */
#define ESAFEWORD_FIELDEXIST 257    /* field does not exist */
#define ESAFEWORD_INTERNAL   258    /* internal to safeword */

extern int errno_safeword;

#define safeword_check(T, ERR, GOTO) if (!(T)) { errno_safeword = (ERR); goto GOTO; }

char* safeword_strerror(int errnum);
void safeword_perror(const char *string);

struct safeword_db {
	char    *path;
	sqlite3 *handle;
};

struct safeword_credential {
	int  id;
	char *username;
	char *password;
	char *message;
	char **tags;
	unsigned int tags_size;
};

int safeword_init(const char *path);
int safeword_open(struct safeword_db *db, const char *path);
int safeword_close(struct safeword_db *db);
int safeword_config(const char* key, const char* value);
int safeword_credential_add(struct safeword_db *db, int *credential_id,
	const char *username, const char *password, const char *description);
int safeword_credential_remove(struct safeword_db *db, int credential_id);
int safeword_credential_untag(struct safeword_db *db, long int credential_id, const char *tag);
int safeword_credential_info(struct safeword_db *db, struct safeword_credential *credential);
int safeword_credential_free(struct safeword_credential *credential);
int safeword_credential_update(struct safeword_db *db, struct safeword_credential *credential);
int safeword_credential_tag(struct safeword_db *db, long int credential_id, const char *tag);
int safeword_tag_info(struct safeword_db *db, const char *tag);
int safeword_tag_update(struct safeword_db *db, const char *tag, const char *wiki);
int safeword_tag_delete(struct safeword_db *db, const char *tag);
int safeword_tag_rename(struct safeword_db *db, const char *old, const char *new);
int safeword_cp_username(struct safeword_db *db, int credential_id, unsigned int ms);
int safeword_cp_password(struct safeword_db *db, int credential_id, unsigned int ms);
int safeword_list_tags(struct safeword_db *db, int credential_id, unsigned int *tags_size, const char ***tags);
int safeword_list_credentials(struct safeword_db *db, unsigned int tags_size, char **tags);

#endif // SAFEWORD_H
