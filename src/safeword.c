#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>

#ifdef WIN32
#include "windows.h"
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>
#endif

#include "dbg.h"
#include "safeword.h"
#include "commands/Command.h"

int safeword_errno = 0;
static long int _tag_id;
static int _copy_once = 0;

char* safeword_strerror(int errnum)
{
	switch (errnum) {
	case ESAFEWORD_DBEXIST:
	case -ESAFEWORD_DBEXIST:
		return "Database does not exist";
	case ESAFEWORD_INVARG:
	case -ESAFEWORD_INVARG:
		return "Invalid argument";
	case ESAFEWORD_BACKENDSTORAGE:
	case -ESAFEWORD_BACKENDSTORAGE:
		return "Backend storage";
	case ESAFEWORD_NOMEM:
	case -ESAFEWORD_NOMEM:
		return "Out of memory";
	case ESAFEWORD_NOCREDENTIAL:
	case -ESAFEWORD_NOCREDENTIAL:
		return "Credential does not exist";
	default:
		return strerror(errnum);
	}
}

void safeword_perror(const char *string)
{
	if (string)
		fprintf(stderr, "%s: ", string);

	fprintf(stderr, "%s", safeword_strerror(safeword_errno));
}

int safeword_config(const char* key, const char* value)
{
	if (!strcmp(key, "copy_once")) {
		if (strlen(value) == 1 && value[0] == '1')
			_copy_once = 1;
	} else
		return -1;

	return 0;
}

/* #region safeword init function */

int safeword_init(const char *path)
{
	int ret = 0;
	sqlite3* handle;
	char sql[512];

	safeword_check(path, ESAFEWORD_INVARG, fail);

	/* ensure that path does not already exist */
	safeword_check(access(path, F_OK) == -1, ESAFEWORD_DBEXIST, fail);

	ret = sqlite3_open(path, &handle);
	safeword_check(!ret, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS tags "
		"(id INTEGER PRIMARY KEY, "
		"tag TEXT NOT NULL, "
		"wiki TEXT, "
		"UNIQUE (tag) ON CONFLICT ABORT, "
		"CONSTRAINT no_empty_tag CHECK (tag != '')"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS usernames "
		"(id INTEGER PRIMARY KEY, "
		"username TEXT, "
		"UNIQUE (username) ON CONFLICT ABORT"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS passwords "
		"(id INTEGER PRIMARY KEY, "
		"password TEXT, "
		"UNIQUE (password) ON CONFLICT ABORT "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS credentials ("
		"id INTEGER PRIMARY KEY, "
		"usernameid INTEGER REFERENCES usernames(id), "
		"passwordid INTEGER REFERENCES passwords(id), "
		"description TEXT "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS tagged_credentials ("
		"credentialid INTEGER NOT NULL REFERENCES credentials(id) ON DELETE CASCADE, "
		"tagid INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE, "
		"PRIMARY KEY (credentialid, tagid) "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, ESAFEWORD_BACKENDSTORAGE, fail);

	sqlite3_close(handle);

	return 0;
fail:
	return -1;
}

/* #endregion safeword init function */

/* #region safeword open & close */

int safeword_open(struct safeword_db *db, const char *path)
{
	int ret = 0;

	if (path) {
		db->path = calloc(strlen(path) + 1, sizeof(char));
		safeword_check(db->path, ESAFEWORD_NOMEM, fail);
		strcpy(db->path, path);
	} else {
		char *env = getenv("SAFEWORD_DB");
		if (!env)
			debug("SAFEWORD_DB environment variable not set");
		safeword_check(env, ESAFEWORD_DBEXIST, fail);
		db->path = calloc(strlen(env) + 1, sizeof(char));
		safeword_check(db->path, ESAFEWORD_NOMEM, fail);
		strcpy(db->path, env);
	}

	/*
	 * Fail if file does not exist, otherwise sqlite3 will create it. We
	 * require the init function to take care of that.
	 */
	ret = access(db->path, F_OK);
	if (ret)
		debug("safeword database '%s' does not exist", db->path);
	safeword_check(ret == 0, ESAFEWORD_DBEXIST, fail);

	ret = sqlite3_open(db->path, &(db->handle));
	if (ret)
		debug("failed to open safeword database '%s'", db->path);
	safeword_check(ret == 0, ESAFEWORD_DBEXIST, fail);

	/* enable foreign key support in Sqlite3 so delete cascading works. */
	ret = sqlite3_exec(db->handle, "PRAGMA foreign_keys = ON;", 0, 0, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

int safeword_close(struct safeword_db *db)
{
	int ret = 0;

	safeword_check(db, ESAFEWORD_DBEXIST, fail);

	free(db->path);
	ret = sqlite3_close(db->handle);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

/* #endregion safeword open & close */

/* #region safeword list functions */

static int credentials_callback(void* not_used, int argc, char** argv, char** col_name)
{
	char* description = argv[1];
	long int credential_id = atoi(argv[0]);

	if (!description)
		description = "";

	printf("%ld : %s\n", credential_id, description);
	return 0;
}

int safeword_list_credentials(struct safeword_db *db, unsigned int tags_size, char **tags)
{
	int ret = 0, i;
	char *sql;

	if (tags) {
		char *tags_concat;
		/* init to include the commas & null terminator */
		int tags_concat_size = tags_size + 1;

		for (i = 0; i < tags_size; i++) {
			/* add 2 for wrapping in single quotes */
			tags_concat_size += strlen(tags[i]) + 2;
		}
		tags_concat = calloc(tags_concat_size, sizeof(char));
		safeword_check(tags_concat, ESAFEWORD_NOMEM, fail);

		for (i = 0; i < tags_size; i++) {
			strcat(tags_concat, "'");
			strcat(tags_concat, tags[i]);
			strcat(tags_concat, "'");
			if (i != tags_size - 1)
				strcat(tags_concat, ",");
		}

		sql = calloc(strlen(tags_concat) + 256, sizeof(char));
		safeword_check(sql, ESAFEWORD_NOMEM, fail);

		/* Find credentials with the specified tags */
		sprintf(sql, "SELECT c.id,c.description FROM credentials AS c "
			"INNER JOIN tagged_credentials AS tc "
			"INNER JOIN tags AS t ON "
			"(c.id = tc.credentialid AND tc.tagid = t.id) WHERE "
			"t.tag IN (%s) GROUP BY c.id HAVING COUNT(c.id) = %d;",
			tags_concat, tags_size);
		ret = sqlite3_exec(db->handle, sql, credentials_callback, 0, 0);
		free(tags_concat);
		free(sql);
	} else {
		if (tags_size == UINT_MAX) {
			sql = calloc(100, sizeof(char));
			safeword_check(sql, ESAFEWORD_NOMEM, fail);

			/* Find all credentials */
			sprintf(sql, "SELECT id,description FROM credentials;");
			ret = sqlite3_exec(db->handle, sql, credentials_callback, 0, 0);
			free(sql);
		} else {
			sql = calloc(256, sizeof(char));
			safeword_check(sql, ESAFEWORD_NOMEM, fail);

			/* Find credentials that have no tags */
			sprintf(sql, "SELECT id,description FROM credentials "
			"WHERE id NOT IN ("
			"SELECT credentialid FROM tagged_credentials "
			"WHERE tagid IN ("
			"SELECT id FROM tags));");
			ret = sqlite3_exec(db->handle, sql, credentials_callback, 0, 0);
			free(sql);
		}
	}

fail:
	return ret;
}

/* #endregion safeword list functions */

/* #region safeword info functions */

char* safeword_credential_tostring(struct safeword_credential *credential)
{
	int i, len = 0;
	char *str;

	if (!credential)
		return NULL;

	if (credential->username)
		len += strlen(credential->username);
	if (credential->password)
		len += strlen(credential->password);
	if (credential->description)
		len += strlen(credential->description);
	for (i = 0; i < credential->tags_size; i++)
		if (credential->tags[i])
			len += strlen(credential->tags[i]);

	str = calloc(256 + len + 1, sizeof(char));
	safeword_check(str != NULL, ESAFEWORD_NOMEM, fail);

	len = sprintf(str, "{ id='%d', username='%s', password='%s', description='%s', tags='",
		credential->id,
		credential->username ? credential->username : "",
		credential->password ? credential->password : "",
		credential->description ? credential->description : "");

	for (i = 0; i < credential->tags_size; i++) {
		if (!credential->tags[i])
			continue;
		if (i != 0)
			len += sprintf(len + str, " ");
		len += sprintf(len + str, "%s", credential->tags[i]);
	}
	sprintf(len + str, "' }");

	return str;
fail:
	return NULL;
}

static int credential_exists_callback(void* existsptr, int argc, char** argv, char** columns)
{
	int *exists = (int*) existsptr;
	*exists = 1;
	return 0;
}

int safeword_credential_exists(struct safeword_db *db, long int credential_id)
{
	int ret, exists = 0;
	char* sql;

	/* Credential ids are only positive. */
	if (credential_id < 0) return 0;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);

	sql = calloc(256, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "SELECT id FROM credentials WHERE id = %ld;", credential_id);
	ret = sqlite3_exec(db->handle, sql, &credential_exists_callback, &exists, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	free(sql);

	return exists;
fail_sql:
	free(sql);
fail:
	return 0;
}

static int credential_get_tags_size_callback(void* cred, int argc, char** argv, char** columns)
{
	struct safeword_credential *credential = (struct safeword_credential*) cred;
	credential->tags_size = strtol(argv[0], NULL, 10);
	return 0;
}

static int credential_get_credential_callback(void* cred, int argc, char** argv, char** columns)
{
	struct safeword_credential *credential = (struct safeword_credential*) cred;
	credential->id = argv[0] ? strtol(argv[0], NULL, 10) : 0;
	if (argv[1]) {
		credential->description = calloc(strlen(argv[1]) + 1, sizeof(char));
		safeword_check(credential->description, ESAFEWORD_NOMEM, fail);
		strcpy(credential->description, argv[1]);
	}
	return 0;
fail:
	return -1;
}

static int credential_get_username_callback(void* cred, int argc, char** argv, char** columns)
{
	struct safeword_credential *credential = (struct safeword_credential*) cred;
	if (argv[0]) {
		credential->username = calloc(strlen(argv[0]) + 1, sizeof(char));
		safeword_check(credential->username, ESAFEWORD_NOMEM, fail);
		strcpy(credential->username, argv[0]);
	}
	return 0;
fail:
	return -1;
}

static int credential_get_password_callback(void* cred, int argc, char** argv, char** columns)
{
	struct safeword_credential *credential = (struct safeword_credential*) cred;
	if (argv[0]) {
		credential->password = calloc(strlen(argv[0]) + 1, sizeof(char));
		safeword_check(credential->password, ESAFEWORD_NOMEM, fail);
		strcpy(credential->password, argv[0]);
	}
	return 0;
fail:
	return -1;
}

int safeword_credential_read(struct safeword_db *db, struct safeword_credential *credential)
{
	int ret, i = 0;
	char* sql;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);
	ret = safeword_credential_exists(db, credential->id);
	safeword_check(ret == 1, ESAFEWORD_NOCREDENTIAL, nocredential);

	sql = calloc(256, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	if (credential->username) {
		free(credential->username);
		credential->username = NULL;
	}
	sprintf(sql, "SELECT u.username FROM credentials AS c "
		"INNER JOIN usernames AS u ON (c.usernameid = u.id) "
		"WHERE c.id = %d;", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_username_callback, credential, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	if (credential->password) {
		free(credential->password);
		credential->password = NULL;
	}
	sprintf(sql, "SELECT p.password FROM credentials AS c "
		"INNER JOIN passwords AS p ON (c.passwordid = p.id) "
		"WHERE c.id = %d;", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_password_callback, credential, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	if (credential->description) {
		free(credential->description);
		credential->description = NULL;
	}
	sprintf(sql, "SELECT id,description FROM credentials WHERE id = %d;", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_credential_callback, credential, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	/* Find all tags for the specified credential ID */
	credential->tags_size = 0;
	credential->tags = 0;
	sprintf(sql, "SELECT count(*) FROM (SELECT t.tag FROM tags AS t INNER JOIN tagged_credentials AS tc "
		"ON (tc.tagid = t.id) WHERE tc.credentialid = %d);", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_tags_size_callback, credential, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	credential->tags = calloc(credential->tags_size, sizeof(char*));
	safeword_check(credential->tags != 0, ESAFEWORD_NOMEM, fail_sql);

	sqlite3_stmt *stmt = NULL;
	sprintf(sql, "SELECT t.tag FROM tags AS t INNER JOIN tagged_credentials AS tc "
		"ON (tc.tagid = t.id) WHERE tc.credentialid = %d;", credential->id);
	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_tags);
	const char *tag;
	do {
		ret = sqlite3_step(stmt);

		switch (ret) {
		case SQLITE_DONE:
			break;
		case SQLITE_ROW:
			tag = (const char*) sqlite3_column_text(stmt, 0);
			if (!tag) continue;
			credential->tags[i] = calloc(strlen(tag) + 1, sizeof(char));
			safeword_check(credential->tags[i], ESAFEWORD_NOMEM, fail_tag);
			strcpy(credential->tags[i], tag);
			i++;
			break;
		default:
			break;
		}
	} while (ret == SQLITE_ROW);
	safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	free(sql);
	return 0;
nocredential:
	return 0;
fail_tag:
	for (; i > 0; i--)
		free(credential->tags[i]);
fail_tags:
	free(credential->tags);
fail_sql:
	free(sql);
fail:
	return -1;
}

int safeword_credential_free(struct safeword_credential *credential)
{
	int i;

	if (!credential)
		return 0;

	free(credential->username);
	free(credential->password);
	free(credential->description);

	for (i = 0; i < credential->tags_size; i++)
		free(credential->tags[i]);
	free(credential->tags);

	return 0;
}

int safeword_tag_read(struct safeword_db *db, struct safeword_tag *tag)
{
	int ret;
	char *sql;
	sqlite3_stmt *stmt = NULL;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(tag != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(tag->tag != NULL, ESAFEWORD_INVARG, fail);

	sql = "SELECT wiki FROM tags WHERE tag = ?;";
	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_text(stmt, 1, tag->tag, strlen(tag->tag), SQLITE_STATIC);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) {
		const char *col = (const char*) sqlite3_column_text(stmt, 0);
		if (col) {
			tag->wiki = calloc(strlen(col) + 1, sizeof(char));
			if (tag->wiki) {
				strcpy(tag->wiki, col);
			}
		}
	}
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

/* #endregion safeword info functions */

/* #region safeword credential functions */

static int map_to_credential(sqlite3* handle, const char* value, const char* table, const char* field,
	sqlite3_int64 credential_rowid)
{
	int ret;
	char* sql = 0;
	sqlite3_int64 id = 0; /* set to invalid value */
	sqlite3_stmt *stmt = NULL;

	sql = calloc(strlen(value) + strlen(table) + strlen(field) + 100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "SELECT id FROM %s WHERE %s = ?;", table, field);;
	ret = sqlite3_prepare_v2(handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_text(stmt, 1, value, strlen(value) + 1, SQLITE_STATIC);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW)
		id = sqlite3_column_int64(stmt, 0);
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	/* if the field does not exist create it, so we can map it */
	if (!id) {
		sprintf(sql, "INSERT INTO %s (%s) VALUES (?);", table, field);
		ret = sqlite3_prepare_v2(handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, value, strlen(value) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		id = sqlite3_last_insert_rowid(handle);
	}

	sprintf(sql, "UPDATE credentials SET %sid = %d WHERE id = %d;", field, (int) id, (int) credential_rowid);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

	return 0;
fail:
	free(sql);
	return -1;
}

int map_username(sqlite3* handle, const char* username, sqlite3_int64 credential_rowid)
{
	return map_to_credential(handle, username, "usernames", "username", credential_rowid);
}

int map_password(sqlite3* handle, const char* password, sqlite3_int64 credential_rowid)
{
	return map_to_credential(handle, password, "passwords", "password", credential_rowid);
}

int safeword_credential_add(struct safeword_db *db, struct safeword_credential *credential)
{
	int ret = 0;
	char* sql;
	sqlite3_stmt *stmt = NULL;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(credential != NULL, ESAFEWORD_INVARG, fail);

	sql = calloc(100, sizeof(char));
	sprintf(sql, "INSERT INTO credentials DEFAULT VALUES;");
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);
	credential->id = sqlite3_last_insert_rowid(db->handle);

	if (credential->description) {
		sql = "UPDATE credentials SET description = ? WHERE id = ?;";
		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_int64(stmt, 2, credential->id);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->description, strlen(credential->description) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	if (credential->username)
		map_username(db->handle, credential->username, credential->id);
	if (credential->password)
		map_password(db->handle, credential->password, credential->id);

	return 0;
fail:
	return -1;
}

struct safeword_credential *safeword_credential_create(const char *username, const char *password, const char *description)
{
	struct safeword_credential *cred = calloc(1, sizeof(struct safeword_credential));
	safeword_check(cred != NULL, ESAFEWORD_NOMEM, fail);

	if (username) {
		cred->username = calloc(strlen(username) + 1, sizeof(char));
		safeword_check(cred != NULL, ESAFEWORD_NOMEM, fail_username);
		strcpy(cred->username, username);
	}
	if (password) {
		cred->password = calloc(strlen(password) + 1, sizeof(char));
		safeword_check(cred != NULL, ESAFEWORD_NOMEM, fail_password);
		strcpy(cred->password, password);
	}
	if (description) {
		cred->description = calloc(strlen(description) + 1, sizeof(char));
		safeword_check(cred != NULL, ESAFEWORD_NOMEM, fail_description);
		strcpy(cred->description, description);
	}

	return cred;
fail_description:
	if (cred->password)
		free(cred->password);
fail_password:
	if (cred->username)
		free(cred->username);
fail_username:
	free(cred);
fail:
	return NULL;
}

int safeword_credential_delete(struct safeword_db *db, long int credential_id)
{
	int ret;
	char *sql;

	safeword_check(db != NULL, ESAFEWORD_DBEXIST, fail);

	sql = calloc(100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "DELETE FROM credentials WHERE id = '%ld';", credential_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	safeword_check(ret == 0, ESAFEWORD_BACKENDSTORAGE, fail_sql);
	free(sql);

	return 0;
fail_sql:
	free(sql);
fail:
	return -1;
}

int safeword_credential_update(struct safeword_db *db, struct safeword_credential *credential)
{
	int ret = 0;
	char *sql;

	if (!credential->id)
		goto fail;

	sql = calloc(512, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	if (credential->username) {
		/* Add the new username to the database if it does not already exist. */
		sprintf(sql, "INSERT OR IGNORE INTO usernames (username) VALUES ('%s');", credential->username);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		/* Update the usernameid to the id of the username if it exists. */
		sprintf(sql, "UPDATE OR ABORT credentials SET usernameid = COALESCE("
			"(SELECT id FROM usernames WHERE username = '%s'), usernameid) WHERE id = %d;",
			credential->username, credential->id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	}

	if (credential->password) {
		/* Add the new password to the database if it does not already exist. */
		sprintf(sql, "INSERT OR IGNORE INTO passwords (password) VALUES ('%s');", credential->password);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		/* Update the passwordid to the id of the password if it exists. */
		sprintf(sql, "UPDATE OR ABORT credentials SET passwordid = COALESCE("
			"(SELECT id FROM passwords WHERE password = '%s'), passwordid) WHERE id = %d;",
			credential->password, credential->id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	}

	if (credential->description) {
		/* Update the description if it exists. */
		sprintf(sql, "UPDATE OR ABORT credentials SET description = '%s' WHERE id = %d;",
			credential->description, credential->id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	}

fail:
	free(sql);
	return ret;
}

/* #endregion safeword credential functions */

/* #region safeword tag functions */

static int tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	_tag_id = atoi(argv[0]);
	return 0;
}

int safeword_credential_tag(struct safeword_db *db, long int credential_id, const char *tag)
{
	int ret;
	char *sql;
	_tag_id = 0; /* set to invalid value to represent it does not exist */

	safeword_check(credential_id, ESAFEWORD_INVARG, fail);
	safeword_check(tag, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_callback, 0, 0);
	free(sql);

	if (!_tag_id) {
		sql = calloc(strlen(tag) + 100, sizeof(char));
		safeword_check(sql, ESAFEWORD_NOMEM, fail);

		sprintf(sql, "INSERT INTO tags (tag) VALUES ('%s');", tag);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		free(sql);
		_tag_id = sqlite3_last_insert_rowid(db->handle);
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	/* Either create or overwrite the mapping between credential and tag */
	sprintf(sql, "INSERT OR REPLACE INTO tagged_credentials "
		"(credentialid, tagid) VALUES (%ld, %ld);",
		credential_id, _tag_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_credential_untag(struct safeword_db *db, long int credential_id, const char *tag)
{
	int ret;
	char *sql;
	_tag_id = 0; /* set to invalid value to represent it does not exist */

	safeword_check(credential_id, ESAFEWORD_INVARG, fail);
	safeword_check(tag, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_callback, 0, 0);
	free(sql);

	if (!_tag_id) {
		ret = ESAFEWORD_INVARG;
		goto fail;
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	/* Delete the mapping between the credential and tag */
	sprintf(sql, "DELETE FROM tagged_credentials WHERE "
		"(credentialid = %ld AND tagid = %ld);",
		credential_id, _tag_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_tag_update(struct safeword_db *db, const char *tag, const char *wiki)
{
	int ret;
	char *sql;

	safeword_check(tag, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_callback, 0, 0);
	free(sql);

	/* make sure the tag exists */
	safeword_check(_tag_id, ESAFEWORD_BACKENDSTORAGE, fail);

	if (wiki) {
		sql = calloc(strlen(wiki) + strlen(tag) + 100, sizeof(char));
		safeword_check(sql, ESAFEWORD_NOMEM, fail);

		/* Replace the existing wiki column value with the new value */
		sprintf(sql, "UPDATE OR ABORT tags SET wiki = '%s' WHERE tag = '%s';", wiki, tag);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		free(sql);
	}
fail:
	return ret;
}

int safeword_list_tags(struct safeword_db *db, unsigned int *tags_size, char ***tags,
	unsigned int filter_size, const char **filter)
{
	int ret = 0, rows = 0, i = 0;
	char *sql;
	const char *tag;
	sqlite3_stmt *stmt = NULL;

	safeword_check(tags_size != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(tags != NULL, ESAFEWORD_INVARG, fail);

	if (filter_size > 0) {
		safeword_check(filter != NULL, ESAFEWORD_INVARG, fail);
		int total = 0;
		/* Get a count of bytes of all tags for SQL prepared statement. */
		for (i = 0; i < filter_size; i++)
			total += strlen(filter[i]);

		/* Include enough for the '?' and ',' for the prepared statement. */
		total += (filter_size * 2);

		sql = calloc(512 + total + 1, sizeof(char));
		safeword_check(sql != NULL, ESAFEWORD_NOMEM, fail);
		sprintf(sql, "SELECT count(*) FROM (SELECT tag FROM tags WHERE id IN ("
		"SELECT tagid FROM tagged_credentials WHERE credentialid IN ("
		"SELECT c.id FROM credentials AS c INNER JOIN tagged_credentials AS tc INNER JOIN tags AS t "
		"ON (tc.credentialid = c.id AND tc.tagid = t.id) WHERE t.tag IN (");
		for (i = 0; i < filter_size; i++) {
			if (i != 0)
				strcat(sql, ",");
			strcat(sql, "?");
		}
		sprintf(sql + strlen(sql), ") GROUP BY c.id HAVING count(c.id) = %d)));", filter_size);

		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		for (i = 0; i < filter_size; i++) {
			ret = sqlite3_bind_text(stmt, i + 1, filter[i], strlen(filter[i]), SQLITE_STATIC);
			safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		}
	} else {
		sql = "SELECT count(*) FROM (SELECT tag FROM tags);";
		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	/* Get the number of tags that will be returned so we can allocate memory for them. */
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW)
		rows = sqlite3_column_int(stmt, 0);
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	*tags_size = rows;

	/* Now bind the tags to the prepared statement. */
	if (filter_size > 0) {
		int total = 0;
		/* Get a count of bytes of all tags for SQL prepared statement. */
		for (i = 0; i < filter_size; i++)
			total += strlen(filter[i]);

		/* Include enough for the '?' and ',' for the prepared statement. */
		total += (filter_size * 2);

		/*sql = calloc(512 + total + 1, sizeof(char));*/
		/*safeword_check(sql != NULL, ESAFEWORD_NOMEM, fail);*/
		sprintf(sql, "SELECT tag FROM tags WHERE id IN ("
		"SELECT tagid FROM tagged_credentials WHERE credentialid IN ("
		"SELECT c.id FROM credentials AS c INNER JOIN tagged_credentials AS tc INNER JOIN tags AS t "
		"ON (tc.credentialid = c.id AND tc.tagid = t.id) WHERE t.tag IN (");
		for (i = 0; i < filter_size; i++) {
			if (i != 0)
				strcat(sql, ",");
			strcat(sql, "?");
		}
		sprintf(sql + strlen(sql), ") GROUP BY c.id HAVING count(c.id) = %d));", filter_size);

		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		for (i = 0; i < filter_size; i++) {
			ret = sqlite3_bind_text(stmt, i + 1, filter[i], strlen(filter[i]), SQLITE_STATIC);
			safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		}
	} else {
		sql = "SELECT tag FROM tags;";
		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	/* If we have any tags, create an array and copy them. */
	if (*tags_size > 0) {
		*tags = calloc(*tags_size, sizeof(char*));
		safeword_check(*tags != NULL, ESAFEWORD_NOMEM, fail);

		i = 0; /* reset */
		do {
			ret = sqlite3_step(stmt);
			if (ret == SQLITE_ROW) {
				tag = (const char*) sqlite3_column_text(stmt, 0);
				if (!tag) continue;
				(*tags)[i] = calloc(strlen(tag) + 1, sizeof(char));
				if (!((*tags)[i])) continue;
				strcpy((*tags)[i], tag);
				i++;
			}
		} while (ret == SQLITE_ROW);
	}
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	return 0;
fail:
	/* TODO free sql variable */
	return -1;
}

int safeword_tag_delete(struct safeword_db *db, const char *tag)
{
	int ret;
	char *sql;

	safeword_check(tag, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "DELETE FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

int safeword_tag_rename(struct safeword_db *db, const char *old, const char *new)
{
	int ret;
	char *sql;

	safeword_check(old, ESAFEWORD_INVARG, fail);
	safeword_check(new, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(old) + strlen(new) + 100, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "UPDATE tags SET tag = '%s' WHERE tag = '%s';", new, old);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

/* #endregion safeword tag functions */

/* #region safeword cp functions */

/* returns the difference in milliseconds */
static unsigned int diff(struct timespec *start, struct timespec *end)
{
	return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
		((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}

#ifndef WIN32
struct __async_waiting_data {
	volatile int *waiting;
	Display *dpy;
	Window *win;
	char *data;
};

static void* wait_for_clipboard_request(void *waiting_data)
{
	struct __async_waiting_data *data = (struct __async_waiting_data*) waiting_data;
	Display *dpy = data->dpy;
	Window win = *data->win;
	XSelectionRequestEvent *req;
	XEvent e, respond;
	static Atom targets;

	if (!targets)
		targets = XInternAtom(dpy, "TARGETS", False);

	XSelectInput(dpy, win, PropertyChangeMask);
	XSelectInput(dpy, win, StructureNotifyMask+ExposureMask);
	XSetSelectionOwner(dpy, XA_CLIPBOARD(dpy), win, CurrentTime);

	while (*data->waiting) {
		XFlush(dpy);
		XNextEvent(dpy, &e);
		if (e.type == SelectionRequest) {
			req=&(e.xselectionrequest);
			if (req->target == targets) {
				Atom supported[] = {
					XA_UTF8_STRING(dpy),
					XA_STRING
				};
				XChangeProperty(dpy,
					req->requestor,
					req->property,
					XA_ATOM,
					32,
					PropModeReplace,
					(unsigned char*) supported,
					(int) (sizeof(supported) / sizeof(Atom)));
				respond.xselection.property=req->property;
			} else if (req->target == XA_UTF8_STRING(dpy) ||
				req->target == XA_STRING) {
				XChangeProperty(dpy,
					req->requestor,
					req->property,
					req->target,
					8,
					PropModeReplace,
					(unsigned char*) data->data,
					strlen(data->data));
				respond.xselection.property=req->property;

				if (_copy_once) {
					*data->waiting = 0;
				}
			} else {
				respond.xselection.property= None;
			}
			respond.xselection.type= SelectionNotify;
			respond.xselection.display= req->display;
			respond.xselection.requestor= req->requestor;
			respond.xselection.selection=req->selection;
			respond.xselection.target= req->target;
			respond.xselection.time = req->time;
			XSendEvent(dpy, req->requestor,0,0,&respond);
			XFlush(dpy);
		}
	}
	return 0;
}
#endif

static int copy_credential_callback(void* millis, int argc, char** argv, char** col_name)
{
#ifdef WIN32
	DWORD len = strlen(argv[0]);
	HGLOBAL lock;
	LPWSTR data;

	lock = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len + 1) * sizeof(char));
	data = (LPWSTR)GlobalLock(lock);
	memcpy(data, argv[0], len * sizeof(char));
	data[len] = 0;
	GlobalUnlock(lock);

	// Set clipboard data
	if (!OpenClipboard(NULL)) return GetLastError();
	EmptyClipboard();
	if (!SetClipboardData(CF_TEXT, lock)) return GetLastError();
	CloseClipboard();

	return 0;
#else
	int ret = 0;
	unsigned int *ms = millis;
	volatile int waiting = 1;
	char *data;
	struct timespec start, now;
	Display *dpy;
	Window win;

	/* copy the data to local variable that will stay in scope for waiting thread */
	data = calloc(strlen(argv[0]) + 1, sizeof(char));
	safeword_check(data, ESAFEWORD_NOMEM, fail);

	strcpy(data, argv[0]);

	if (!(dpy = XOpenDisplay(NULL))) {
		debug("could not open display\n");
		ret = ESAFEWORD_BACKENDSTORAGE;
		goto fail;
	}

	/* create a window to trap events */
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	clock_gettime(CLOCK_MONOTONIC, &now);

	struct __async_waiting_data waiting_data;
	waiting_data.waiting = &waiting;
	waiting_data.dpy = dpy;
	waiting_data.win = &win;
	waiting_data.data = data;

	pthread_t tid;
	pthread_create(&tid, NULL, &wait_for_clipboard_request, &waiting_data);

	while (waiting) {
		if (diff(&start, &now) > *ms) {
			pthread_cancel(tid);
			waiting = 0;
		}

		clock_gettime(CLOCK_MONOTONIC, &now);
	}

fail:
	free(data);
	return ret;
#endif
}

static int safeword_cp(struct safeword_db *db, int credential_id, unsigned int ms,
	const char* table, const char* field)
{
	int ret;
	char *sql;

	sql = calloc(256, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "SELECT t.%s FROM %s AS t "
		"INNER JOIN credentials AS c "
		"ON (c.%sid = t.id) WHERE c.id = %d;", field, table, field, credential_id);
	ret = sqlite3_exec(db->handle, sql, copy_credential_callback, &ms, 0);
	free(sql);
fail:
	return ret;
}

int safeword_cp_username(struct safeword_db *db, int credential_id, unsigned int ms)
{
	return safeword_cp(db, credential_id, ms, "usernames", "username");
}

int safeword_cp_password(struct safeword_db *db, int credential_id, unsigned int ms)
{
	return safeword_cp(db, credential_id, ms, "passwords", "password");
}

/* #endregion safeword cp functions */

