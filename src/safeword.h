/**
 * @defgroup safeword
 * @{
 * @author Erich Schroeter <erich.schroeter+safeword@gmail.com>
 */
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

#define ESAFEWORD_DBEXIST        1 /* Database does not exist */
#define ESAFEWORD_INVARG         2 /* Invalid argument */
#define ESAFEWORD_BACKENDSTORAGE 3 /* Backend storage */
#define ESAFEWORD_NOMEM          4 /* Out of memory */
#define ESAFEWORD_NOCREDENTIAL   5 /* Credential does not exist */

extern int safeword_errno;

#define safeword_check(T, ERR, GOTO) if (!(T)) { safeword_errno = (ERR); goto GOTO; }

char* safeword_strerror(int errnum);
void safeword_perror(const char *string);

struct safeword_db {
	char    *path;
	sqlite3 *handle;
};

struct safeword_tag {
	char *tag;
	char *wiki;
};

struct safeword_credential {
	int  id;
	char *username;
	char *password;
	char *description;
	unsigned int tags_size;
	char **tags;
};

/**
 * create a safeword database
 *
 * This function creates a safeword database file specified by @c path.
 * If @c path exists it will not be overwritten; it is left to the
 * caller to remove the file before calling this function.
 *
 * @param path the safeword database file to be created
 *
 * @see safeword_open, safeword_close
 */
int safeword_init(const char *path);
/**
 * open the safeword database specified by @c path
 *
 * This function opens the safeword database @c path to be used by other
 * safeword functions and initializes @c db.
 *
 * @param db a pointer to the safeword database to be initialized
 * @param path the safeword database file to be initialized
 *
 * @see safeword_init, safeword_close
 */
int safeword_open(struct safeword_db *db, const char *path);
/**
 * close the specified safeword database
 *
 * This function cleans up memory allocated in @link safeword_open @endlink.
 *
 * @param db a pointer to the safeword database to be closed
 *
 * @see safeword_init, safeword_open
 */
int safeword_close(struct safeword_db *db);
int safeword_config(const char* key, const char* value);
char* safeword_credential_tostring(struct safeword_credential *credential);
/**
 * test the existence of the credential specified by @c credential_id
 *
 * Queries the safeword database for the existence of a credential with the id
 * specified by @c credential_id.
 *
 * @param db a pointer to the safeword database to be queried
 * @param credential_id the credential id to test
 *
 * @see safeword_credential_add, safeword_credential_read,
 * safeword_credential_update, safeword_credential_delete
 */
int safeword_credential_exists(struct safeword_db *db, long int credential_id);
/**
 * add a credential to the specified safeword database
 *
 * Adds @c credential to the safeword database by inserting a new row in the
 * @a credentials table. If @c credential.username or @c credential.password
 * do not exist in their respective tables, they will be created. @c
 * credential.id will be updated to the value returned by the database.
 *
 * @param db a pointer to the safeword database to be modified
 * @param credential the credential to add
 *
 * @see safeword_credential_create, safeword_credential_read,
 * safeword_credential_update, safeword_credential_delete
 */
int safeword_credential_add(struct safeword_db *db, struct safeword_credential *credential);
/**
 * create a safeword credential object in memory
 *
 * This function allocates memory for a safeword_credential struct and the
 * field values from the specified arguments.
 *
 * @see safeword_credential_free
 */
struct safeword_credential *safeword_credential_create(const char *username, const char *password, const char *description);
/**
 * read an existing credential
 *
 * Reads the database and populates the members in @c credential. If @c
 * credential.id is zero or less nothing is read. @c credential.id must be set
 * to the id of the credential to be read.
 *
 * @param the database to query
 * @param credential a pointer to a @link safeword_credential @endlink to
 * store data results in
 *
 * @see safeword_credential_create, safeword_credential_read,
 * safeword_credential_delete, safeword_credential_add
 */
int safeword_credential_read(struct safeword_db *db, struct safeword_credential *credential);
/**
 * modify an existing credential
 *
 * Updates the credential with the information specified in @c credential. If
 * @c credential.id is zero or less the database is not modified.
 *
 * @param the database to modify
 * @param credential the data to update the credential with
 *
 * @see safeword_credential_create, safeword_credential_read,
 * safeword_credential_delete, safeword_credential_add
 */
int safeword_credential_update(struct safeword_db *db, struct safeword_credential *credential);
/**
 * delete an existing credential
 *
 * Deletes the credential with the specified @c credential_id from the
 * database.
 *
 * @param the database to modify
 * @param credential_id the id of the credential to delete
 *
 * @see safeword_credential_create, safeword_credential_read,
 * safeword_credential_update, safeword_credential_add
 */
int safeword_credential_delete(struct safeword_db *db, long int credential_id);
/**
 * free allocated memory of a safeword credential
 *
 * This function frees the memory allocated in @link
 * safeword_credential_create @endlink.
 *
 * @param credential a pointer to the credential created by @link
 * safeword_credential_create @endlink.
 *
 * @see safeword_credential_create
 */
int safeword_credential_free(struct safeword_credential *credential);
/**
 * tag an existing credential
 *
 * Associates the credential with the specified @c credential_id with @c tag.
 *
 * @param db the database to modify
 * @param credential_id the id of the credential to be associated with @c tag
 * @param tag the tag to be associated
 *
 * @see safeword_credential_untag
 */
int safeword_credential_tag(struct safeword_db *db, long int credential_id, const char *tag);
/**
 * untag a tagged credential
 *
 * Disassociates the credential with the specified @c credential_id associated
 * with @c tag.
 *
 * @param db the database to modify
 * @param credential_id the id of the credential associated with @c tag
 * @param tag the tag to be disassociated
 *
 * @see safeword_credential_tag
 */
int safeword_credential_untag(struct safeword_db *db, long int credential_id, const char *tag);
/**
 * list credentials in a safeword database
 *
 * This function will create an array of @link safeword_credential @endlink
 * and assign it to @c credentials, also setting @c credentials_size to the
 * number of credentials in the array.
 *
 * If @c tags_size is zero or @c tags is @c NULL then @c credentials will
 * contain all credentials within the database.
 *
 * @param db the safeword database to query
 * @param tags_size number of tags in @c tags
 * @param tags the tags to filter credentials by
 * @param credentials_size number of credentials in @c credentials
 * @param credentials the credentials found in the database
 *
 * @see safeword_credential_free, safeword_credential_read
 */
int safeword_credential_list(struct safeword_db *db, unsigned int tags_size, char **tags);
/**
 * create a safeword tag object in memory
 *
 * This function allocates memory for a safeword_tag struct and the field
 * values from the specified arguments.
 *
 * @see safeword_tag_read, safeword_tag_update, safeword_tag_delete,
 * safeword_tag_add
 */
struct safeword_tag *safeword_tag_create(char *tag, char *wiki);
/**
 * read an existing tag
 *
 * Reads the database and populates the members in @c tag.
 *
 * @param db the database to query
 * @param tag a pointer to a @link safeword_tag @endlink to update
 *
 * @see safeword_tag_create, safeword_tag_update, safeword_tag_delete,
 * safeword_tag_rename
 */
int safeword_tag_read(struct safeword_db *db, struct safeword_tag *tag);
/**
 * modify an existing tag
 *
 * Updates the tag with the information specified in @c tag. If @c tag.id is
 * @c NULL or does not exist the database is not modified.
 *
 * @param db the database to modify
 * @param tag the data to update the tag with
 *
 * @see safeword_tag_create, safeword_tag_read, safeword_tag_delete,
 * safeword_tag_rename
 */
int safeword_tag_update(struct safeword_db *db, struct safeword_tag *tag);
/**
 * delete an existing tag
 *
 * Deletes @c tag from the database.
 *
 * @param db the database to modify
 * @param tag the tag to delete
 *
 * @see safeword_tag_create, safeword_tag_read, safeword_tag_update,
 * safeword_tag_rename
 */
int safeword_tag_delete(struct safeword_db *db, const char *tag);
/**
 * rename an existing tag
 *
 * Renames the @c old tag to @c new.
 *
 * @param db the database to modify
 * @param old the existing tag
 * @param new what to rename @c old to
 *
 * @see safeword_tag_read, safeword_tag_update, safeword_tag_delete
 */
int safeword_tag_rename(struct safeword_db *db, const char *old, const char *new);
/**
 * list tags in a safeword database
 *
 * This function will create an array of @link safeword_tag @endlink and
 * assign it to @c tags, also setting @c tags_size to the number of tags
 * in the array.
 *
 * If @c filter_size is zero or @c filter is @c NULL then @c tags will contain
 * all tags within the database.
 *
 * @param db the safeword database to query
 * @param tags_size number of tags in @c tags
 * @param tags the tags found in the database
 * @param filter_size number of tags in filter
 * @param filter the tags to filter by
 *
 * @see safeword_tag_delete, safeword_tag_read
 */
int safeword_tag_list(struct safeword_db *db, unsigned int *tags_size, char ***tags,
	unsigned int filter_size, const char **filter);
/**
 * copy username to clipboard
 *
 * This function will place the username associated with the credential whose
 * id is @c credential_id on the clipboard for @c ms milliseconds. After a
 * duration of @c ms milliseconds the clipboard will be cleared.
 *
 * @param db the database to query
 * @param credential_id the credential id whose username to copy
 * @param ms milliseconds before clearing the clipboard
 *
 * @see safeword_cp_password
 */
int safeword_cp_username(struct safeword_db *db, int credential_id, unsigned int ms);
/**
 * copy password to clipboard
 *
 * This function will place the password associated with the credential whose
 * id is @c credential_id on the clipboard for @c ms milliseconds. After a
 * duration of @c ms milliseconds the clipboard will be cleared.
 *
 * @param db the database to query
 * @param credential_id the credential id whose password to copy
 * @param ms milliseconds before clearing the clipboard
 *
 * @see safeword_cp_username
 */
int safeword_cp_password(struct safeword_db *db, int credential_id, unsigned int ms);

#endif // SAFEWORD_H
/** @} */
