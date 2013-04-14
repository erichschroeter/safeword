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

int safeword_errno = 0;

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

int safeword_config_get(struct safeword_db *db, const char *key, char **value)
{
	const char *sql = "SELECT value FROM properties WHERE key LIKE ?;";
	sqlite3_stmt *stmt = NULL;
	int ret;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(key != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(value != NULL, ESAFEWORD_INVARG, fail);

	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_text(stmt, 1, key, strlen(key) + 1, SQLITE_STATIC);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) {
		const char *field = (const char*) sqlite3_column_text(stmt, 0);
		*value = calloc(strlen(field) + 1, sizeof(char));
		safeword_check(*value != NULL, ESAFEWORD_NOMEM, fail);
		strcpy(*value, field);
	} else {
		*value = NULL;
	}
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

int safeword_config_set(struct safeword_db *db, const char *key, char *value, int persist)
{
	const char *sql = "UPDATE properties SET value = ? WHERE key = ?;";
	sqlite3_stmt *stmt = NULL;
	int ret;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(key != NULL, ESAFEWORD_INVARG, fail);

	if (!strcmp(key, "copy_once")) {
		if (value == NULL || strlen(value) == 0 || value[0] == '0')
			db->config.copy_once = 0;
		else
			db->config.copy_once = 1;
	}

	if (persist) {
		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		if (value == NULL) {
			ret = sqlite3_bind_null(stmt, 1);
		} else {
			ret = sqlite3_bind_text(stmt, 1, value, strlen(value) + 1, SQLITE_STATIC);
		}
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 2, key, strlen(key) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	return 0;
fail:
	return -1;
}

static int safeword_config_load(struct safeword_db *db)
{
	const char *sql = "SELECT key, value FROM properties;";
	sqlite3_stmt *stmt = NULL;
	int ret;

	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	do {
		ret = sqlite3_step(stmt);
		if (ret != SQLITE_ROW)
			break;

		const char *key = (const char*) sqlite3_column_text(stmt, 0);
		const char *value = (const char*) sqlite3_column_text(stmt, 1);

		if (!strcmp(key, "copy_once")) {
			if (value == NULL || strlen(value) == 0 || value[0] == '0')
				db->config.copy_once = 0;
			else
				db->config.copy_once = 1;
		}
	} while (ret == SQLITE_ROW);
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

/* #region safeword init function */

int safeword_init(const char *path)
{
	int ret = 0;
	sqlite3* handle;
	sqlite3_stmt *stmt = NULL;
	char sql[512];

	safeword_check(path, ESAFEWORD_INVARG, fail);

	/* ensure that path does not already exist */
	safeword_check(access(path, F_OK) == -1, ESAFEWORD_DBEXIST, fail);

	ret = sqlite3_open(path, &handle);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS tags "
		"(id INTEGER PRIMARY KEY, "
		"tag TEXT NOT NULL, "
		"wiki TEXT, "
		"UNIQUE (tag) ON CONFLICT ABORT, "
		"CONSTRAINT no_empty_tag CHECK (tag != '')"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS usernames "
		"(id INTEGER PRIMARY KEY, "
		"username TEXT, "
		"UNIQUE (username) ON CONFLICT ABORT"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS passwords "
		"(id INTEGER PRIMARY KEY, "
		"password TEXT, "
		"UNIQUE (password) ON CONFLICT ABORT "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS credentials ("
		"id INTEGER PRIMARY KEY, "
		"usernameid INTEGER REFERENCES usernames(id), "
		"passwordid INTEGER REFERENCES passwords(id), "
		"description TEXT, "
		"note TEXT "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS tagged_credentials ("
		"credentialid INTEGER NOT NULL REFERENCES credentials(id) ON DELETE CASCADE, "
		"tagid INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE, "
		"PRIMARY KEY (credentialid, tagid) "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS properties ("
		"key TEXT PRIMARY KEY NOT NULL, "
		"value TEXT"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	/* Set default values. */
	sprintf(sql, "INSERT INTO properties VALUES ( ?, ? );");
	ret = sqlite3_prepare_v2(handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_text(stmt, 1, "version", strlen("version") + 1, SQLITE_STATIC);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_text(stmt, 2, SAFEWORD_VERSION, strlen(SAFEWORD_VERSION) + 1, SQLITE_STATIC);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(stmt);
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

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
	safeword_check(ret == SQLITE_OK, ESAFEWORD_DBEXIST, fail);

	/* enable foreign key support in Sqlite3 so delete cascading works. */
	ret = sqlite3_exec(db->handle, "PRAGMA foreign_keys = ON;", 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	/* Default all config values to zero/NULL. */
	memset(&db->config, 0, sizeof(db->config));
	safeword_config_load(db);

	return 0;
fail:
	return -1;
}

int safeword_close(struct safeword_db *db)
{
	int ret = 0;

	safeword_check(db != NULL, ESAFEWORD_DBEXIST, fail);

	free(db->path);
	ret = sqlite3_close(db->handle);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

/* #endregion safeword open & close */

/* #region safeword list functions */

static sqlite3_stmt *get_creds_prepared_stmt(struct safeword_db *db, unsigned int tags_size, char **tags,
	int select_count)
{
	int ret = 0, i = 0;
	/* Include enough for the '?' and ',' for the prepared statement. */
	char sql[512 + (tags_size * 4)];
	sqlite3_stmt *stmt = NULL;

	safeword_check(tags != NULL, ESAFEWORD_INVARG, fail);
	memset(sql, 0, sizeof(sql));

	if (select_count)
		sprintf(sql, "SELECT count(*) FROM (");
	sprintf(sql + strlen(sql), "SELECT c.id,c.description FROM credentials AS c "
		"INNER JOIN tagged_credentials AS tc "
		"INNER JOIN tags AS t ON "
		"(c.id = tc.credentialid AND tc.tagid = t.id) WHERE "
		"t.tag LIKE ");
	for (i = 0; i < tags_size; i++) {
		if (i != 0)
			strcat(sql, " OR tag LIKE ");
		strcat(sql, "?");
	}
	sprintf(sql + strlen(sql), " GROUP BY c.id HAVING COUNT(c.id) = %d", tags_size);
	if (select_count)
		strcat(sql, ")");
	strcat(sql, ";");

	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	for (i = 0; i < tags_size; i++) {
		ret = sqlite3_bind_text(stmt, i + 1, tags[i], strlen(tags[i]), SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	return stmt;
fail:
	return NULL;
}

int safeword_credential_list(struct safeword_db *db, unsigned int tags_size, char **tags,
	unsigned int *credentials_size, struct safeword_credential ***credentials)
{
	int ret = 0, i = 0, rows = 0;
	char *sql_all_count = "SELECT count(*) FROM (SELECT id,description FROM credentials);";
	char *sql_all = "SELECT id,description FROM credentials;";
	char *sql_no_tags_count = "SELECT count(*) FROM ("
		"SELECT id,description FROM credentials WHERE id NOT IN ("
		"SELECT credentialid FROM tagged_credentials WHERE tagid IN ("
		"SELECT id FROM tags)));";
	char *sql_no_tags = "SELECT id,description FROM credentials WHERE id NOT IN ("
		"SELECT credentialid FROM tagged_credentials WHERE tagid IN ("
		"SELECT id FROM tags));";
	sqlite3_stmt *stmt = NULL;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);

	/* Prepare SQL for getting number of rows in result set. */
	if (tags_size > 0 && tags != NULL) {
		stmt = get_creds_prepared_stmt(db, tags_size, tags, 1);
		safeword_check(stmt != NULL, safeword_errno, fail);
	} else {
		if (tags_size == UINT_MAX) {
			ret = sqlite3_prepare_v2(db->handle, sql_all_count, strlen(sql_all_count) + 1, &stmt, NULL);
		} else {
			ret = sqlite3_prepare_v2(db->handle, sql_no_tags_count, strlen(sql_no_tags_count) + 1, &stmt, NULL);
		}
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	/* Get the number of creds that will be returned so we can allocate memory for them. */
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW)
		rows = sqlite3_column_int(stmt, 0);
	ret = sqlite3_finalize(stmt);
	stmt = NULL;
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	*credentials_size = rows;

	/* Prepare SQL for getting credentials result set. */
	if (tags_size > 0 && tags != NULL) {
		stmt = get_creds_prepared_stmt(db, tags_size, tags, 0);
		safeword_check(stmt != NULL, safeword_errno, fail);
	} else {
		if (tags_size == UINT_MAX) {
			ret = sqlite3_prepare_v2(db->handle, sql_all, strlen(sql_all) + 1, &stmt, NULL);
		} else {
			ret = sqlite3_prepare_v2(db->handle, sql_no_tags, strlen(sql_no_tags) + 1, &stmt, NULL);
		}
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	/* If we have any creds, create an array and copy them. */
	if (*credentials_size > 0) {
		*credentials = calloc(*credentials_size, sizeof(struct safeword_credential*));
		safeword_check(*credentials != NULL, ESAFEWORD_NOMEM, fail);

		i = 0; /* reset */
		do {
			ret = sqlite3_step(stmt);
			if (ret == SQLITE_ROW) {
				const char *id = (const char*) sqlite3_column_text(stmt, 0);
				const char *description = (const char*) sqlite3_column_text(stmt, 1);

				/*(*credentials)[i] = calloc(1, sizeof(struct safeword_credential));*/
				(*credentials)[i] = safeword_credential_create(NULL, NULL, description, NULL);
				if (!((*credentials)[i])) continue;
				(*credentials)[i]->id = strtol(id, NULL, 10);
				i++;
			}
		} while (ret == SQLITE_ROW);
	}
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
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
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_sql);

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
	if (argv[2]) {
		credential->note = calloc(strlen(argv[2]) + 1, sizeof(char));
		safeword_check(credential->note, ESAFEWORD_NOMEM, fail);
		strcpy(credential->note, argv[2]);
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
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	if (credential->password) {
		free(credential->password);
		credential->password = NULL;
	}
	sprintf(sql, "SELECT p.password FROM credentials AS c "
		"INNER JOIN passwords AS p ON (c.passwordid = p.id) "
		"WHERE c.id = %d;", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_password_callback, credential, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	if (credential->description) {
		free(credential->description);
		credential->description = NULL;
	}
	if (credential->note) {
		free(credential->note);
		credential->note = NULL;
	}
	sprintf(sql, "SELECT id,description,note FROM credentials WHERE id = %d;", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_credential_callback, credential, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	/* Find all tags for the specified credential ID */
	credential->tags_size = 0;
	credential->tags = 0;

	sprintf(sql, "SELECT count(*) FROM (SELECT t.tag FROM tags AS t INNER JOIN tagged_credentials AS tc "
		"ON (tc.tagid = t.id) WHERE tc.credentialid = %d);", credential->id);
	ret = sqlite3_exec(db->handle, sql, &credential_get_tags_size_callback, credential, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_sql);

	if (credential->tags_size == 0) goto skip_tags;

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
skip_tags:

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

struct safeword_tag *safeword_tag_create(char *name, char *wiki)
{
	struct safeword_tag *tag = calloc(1, sizeof(struct safeword_tag));
	safeword_check(tag != NULL, ESAFEWORD_NOMEM, fail);

	if (name) {
		tag->tag = calloc(strlen(name) + 1, sizeof(char));
		safeword_check(tag->tag != NULL, ESAFEWORD_NOMEM, fail_tag);
		strcpy(tag->tag, name);
	}
	if (wiki) {
		tag->wiki = calloc(strlen(wiki) + 1, sizeof(char));
		safeword_check(tag->wiki != NULL, ESAFEWORD_NOMEM, fail_wiki);
		strcpy(tag->wiki, wiki);
	}

	return tag;
fail_wiki:
	if (tag->tag)
		free(tag->tag);
fail_tag:
	free(tag);
fail:
	return NULL;
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
	safeword_check(sql != NULL, ESAFEWORD_NOMEM, fail);

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
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
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
	char *sql_insert = "INSERT INTO credentials DEFAULT VALUES;";
	char *sql_update;
	sqlite3_stmt *stmt = NULL;

	safeword_check(db != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(credential != NULL, ESAFEWORD_INVARG, fail);

	ret = sqlite3_exec(db->handle, sql_insert, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	credential->id = sqlite3_last_insert_rowid(db->handle);

	if (credential->description) {
		sql_update = "UPDATE credentials SET description = ? WHERE id = ?;";
		ret = sqlite3_prepare_v2(db->handle, sql_update, strlen(sql_update) + 1, &stmt, NULL);
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

	if (credential->note) {
		sql_update = "UPDATE credentials SET note = ? WHERE id = ?;";
		ret = sqlite3_prepare_v2(db->handle, sql_update, strlen(sql_update) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_int64(stmt, 2, credential->id);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->note, strlen(credential->note) + 1, SQLITE_STATIC);
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

struct safeword_credential *safeword_credential_create(const char *username, const char *password,
	const char *description, const char *note)
{
	struct safeword_credential *cred = calloc(1, sizeof(struct safeword_credential));
	safeword_check(cred != NULL, ESAFEWORD_NOMEM, fail);

	if (username) {
		cred->username = calloc(strlen(username) + 1, sizeof(char));
		safeword_check(cred->username != NULL, ESAFEWORD_NOMEM, fail_username);
		strcpy(cred->username, username);
	}
	if (password) {
		cred->password = calloc(strlen(password) + 1, sizeof(char));
		safeword_check(cred->password != NULL, ESAFEWORD_NOMEM, fail_password);
		strcpy(cred->password, password);
	}
	if (description) {
		cred->description = calloc(strlen(description) + 1, sizeof(char));
		safeword_check(cred->description != NULL, ESAFEWORD_NOMEM, fail_description);
		strcpy(cred->description, description);
	}
	if (note) {
		cred->note = calloc(strlen(note) + 1, sizeof(char));
		safeword_check(cred->note != NULL, ESAFEWORD_NOMEM, fail_note);
		strcpy(cred->note, note);
	}

	return cred;
fail_note:
	if (cred->description)
		free(cred->description);
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
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail_sql);
	free(sql);

	return 0;
fail_sql:
	free(sql);
fail:
	return -1;
}

int safeword_credential_update(struct safeword_db *db, struct safeword_credential *credential)
{
	int ret;
	sqlite3_stmt *stmt = NULL;

	if (!credential->id)
		goto fail;

	if (credential->username) {
		char *sql_insert = "INSERT OR IGNORE INTO usernames (username) VALUES (?);";
		char *sql_update = "UPDATE OR ABORT credentials SET usernameid = COALESCE("
			"(SELECT id FROM usernames WHERE username = ?), usernameid) WHERE id = ?;";

		/* Add the new username to the database if it does not already exist. */
		ret = sqlite3_prepare_v2(db->handle, sql_insert, strlen(sql_insert) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->username, strlen(credential->username) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);

		/* Update the usernameid to the id of the username if it exists. */
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_prepare_v2(db->handle, sql_update, strlen(sql_update) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_int64(stmt, 2, credential->id);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->username, strlen(credential->username) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	if (credential->password) {
		char *sql_insert = "INSERT OR IGNORE INTO passwords (password) VALUES (?);";
		char *sql_update = "UPDATE OR ABORT credentials SET passwordid = COALESCE("
			"(SELECT id FROM passwords WHERE password = ?), passwordid) WHERE id = ?;";

		/* Add the new password to the database if it does not already exist. */
		ret = sqlite3_prepare_v2(db->handle, sql_insert, strlen(sql_insert) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->password, strlen(credential->password) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);

		/* Update the passwordid to the id of the password if it exists. */
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_prepare_v2(db->handle, sql_update, strlen(sql_update) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_int64(stmt, 2, credential->id);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->password, strlen(credential->password) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	if (credential->description) {
		char *sql_update = "UPDATE OR ABORT credentials SET description = ? WHERE id = ?;";

		/* Update the description if it exists. */
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_prepare_v2(db->handle, sql_update, strlen(sql_update) + 1, &stmt, NULL);
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

	if (credential->note) {
		char *sql_update = "UPDATE OR ABORT credentials SET note = ? WHERE id = ?;";

		/* Update the description if it exists. */
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_prepare_v2(db->handle, sql_update, strlen(sql_update) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_int64(stmt, 2, credential->id);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, credential->note, strlen(credential->note) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	return 0;
fail:
	return -1;
}

/* #endregion safeword credential functions */

/* #region safeword tag functions */

static sqlite3_int64 _safeword_get_tag_id(struct safeword_db *db, const char *tag)
{
	char *sql = "SELECT id FROM tags WHERE tag = ?;";
	sqlite3_stmt *stmt = NULL;
	sqlite3_int64 id = 0; /* set to invalid value */
	int ret;

	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_text(stmt, 1, tag, strlen(tag) + 1, SQLITE_STATIC);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(stmt);
	id = sqlite3_column_int64(stmt, 0);
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

fail:
	return id;
}

int safeword_credential_tag(struct safeword_db *db, long int credential_id, const char *tag)
{
	int ret;
	char *id_sql = "INSERT INTO tags (tag) VALUES (?);";
	char *map_sql = "INSERT OR REPLACE INTO tagged_credentials (credentialid, tagid) VALUES (?, ?);";
	sqlite3_int64 tag_id = 0; /* set to invalid value to represent it does not exist */
	sqlite3_stmt *id_stmt = NULL, *map_stmt = NULL;

	safeword_check(credential_id > 0, ESAFEWORD_INVARG, fail);
	safeword_check(tag != NULL, ESAFEWORD_INVARG, fail);

	tag_id = _safeword_get_tag_id(db, tag);

	if (!tag_id) {
		ret = sqlite3_prepare_v2(db->handle, id_sql, strlen(id_sql) + 1, &id_stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(id_stmt, 1, tag, strlen(tag) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(id_stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(id_stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

		tag_id = sqlite3_last_insert_rowid(db->handle);
	}

	ret = sqlite3_prepare_v2(db->handle, map_sql, strlen(map_sql) + 1, &map_stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_int64(map_stmt, 2, tag_id);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_bind_int64(map_stmt, 1, credential_id);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(map_stmt);
	safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_finalize(map_stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
}

int safeword_credential_untag(struct safeword_db *db, long int credential_id, const char *tag)
{
	int ret;
	char *sql;
	sqlite3_int64 tag_id = 0; /* set to invalid value to represent it does not exist */

	safeword_check(credential_id, ESAFEWORD_INVARG, fail);
	safeword_check(tag, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql != NULL, ESAFEWORD_NOMEM, fail);

	tag_id = _safeword_get_tag_id(db, tag);

	if (!tag_id) {
		ret = ESAFEWORD_INVARG;
		goto fail;
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	safeword_check(sql, ESAFEWORD_NOMEM, fail);

	/* Delete the mapping between the credential and tag */
	sprintf(sql, "DELETE FROM tagged_credentials WHERE "
		"(credentialid = %ld AND tagid = %ld);",
		credential_id, (long int) tag_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	free(sql);

fail:
	return ret;
}

int safeword_tag_update(struct safeword_db *db, struct safeword_tag *tag)
{
	int ret;
	char *sql = "UPDATE OR ABORT tags SET wiki = ? WHERE tag = ?;";
	sqlite3_stmt *stmt = NULL;

	safeword_check(tag != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(tag->tag != NULL, ESAFEWORD_INVARG, fail);

	if (tag->wiki) {
		/* Replace the existing wiki column value with the new value */
		ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 1, tag->wiki, strlen(tag->wiki) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_bind_text(stmt, 2, tag->tag, strlen(tag->tag) + 1, SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_step(stmt);
		safeword_check(ret == SQLITE_DONE, ESAFEWORD_BACKENDSTORAGE, fail);
		ret = sqlite3_finalize(stmt);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	return 0;
fail:
	return -1;
}

static sqlite3_stmt *get_filter_prepared_stmt(struct safeword_db *db, unsigned int filter_size, const char **filter,
	int select_count)
{
	int ret = 0, i = 0;
	/* Include enough for the '?' and ',' for the prepared statement. */
	char sql[512 + (filter_size * 4)];
	sqlite3_stmt *stmt = NULL;

	safeword_check(filter != NULL, ESAFEWORD_INVARG, fail);
	memset(sql, 0, sizeof(sql));

	if (select_count)
		sprintf(sql, "SELECT count(*) FROM (");
	sprintf(sql + strlen(sql),
	"SELECT tag FROM tags WHERE id IN ("
	  "SELECT tagid FROM tagged_credentials WHERE credentialid IN ("
	    "SELECT credentialid FROM tagged_credentials "
	    "WHERE tagid IN "
	      "(SELECT id FROM tags WHERE tag LIKE ");
	for (i = 0; i < filter_size; i++) {
		if (i != 0)
			strcat(sql, " OR tag LIKE ");
		strcat(sql, "?");
	}
	sprintf(sql + strlen(sql),
	      ") "
	    "GROUP BY credentialid "
	    "HAVING count(DISTINCT tagid) = %d"
	    ")"
	") AND tag NOT LIKE ", filter_size);
	for (i = 0; i < filter_size; i++) {
		if (i != 0)
			strcat(sql, " AND tag NOT LIKE ");
		strcat(sql, "?");
	}
	if (select_count)
		strcat(sql, ")");
	sprintf(sql + strlen(sql), " ;");

	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	for (i = 0; i < filter_size; i++) {
		ret = sqlite3_bind_text(stmt, i + 1, filter[i], strlen(filter[i]), SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}
	for (i = 0; i < filter_size; i++) {
		ret = sqlite3_bind_text(stmt, i + filter_size + 1, filter[i], strlen(filter[i]), SQLITE_STATIC);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	return stmt;
fail:
	return NULL;
}

int safeword_tag_list(struct safeword_db *db, unsigned int *tags_size, char ***tags,
	unsigned int filter_size, const char **filter)
{
	int ret = 0, rows = 0, i = 0;
	char *sql_tags_count = "SELECT count(*) FROM (SELECT tag FROM tags);";
	char *sql_tags = "SELECT tag FROM tags;";
	const char *tag;
	sqlite3_stmt *stmt = NULL;

	safeword_check(tags_size != NULL, ESAFEWORD_INVARG, fail);
	safeword_check(tags != NULL, ESAFEWORD_INVARG, fail);

	if (filter_size > 0) {
		stmt = get_filter_prepared_stmt(db, filter_size, filter, 1);
		safeword_check(stmt != NULL, safeword_errno, fail);
	} else {
		ret = sqlite3_prepare_v2(db->handle, sql_tags_count, strlen(sql_tags_count) + 1, &stmt, NULL);
		safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	}

	/* Get the number of tags that will be returned so we can allocate memory for them. */
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW)
		rows = sqlite3_column_int(stmt, 0);
	ret = sqlite3_finalize(stmt);
	stmt = NULL;
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	*tags_size = rows;

	/* Now bind the tags to the prepared statement. */
	if (filter_size > 0) {
		stmt = get_filter_prepared_stmt(db, filter_size, filter, 0);
		safeword_check(stmt != NULL, safeword_errno, fail);
	} else {
		ret = sqlite3_prepare_v2(db->handle, sql_tags, strlen(sql_tags) + 1, &stmt, NULL);
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
	return -1;
}

int safeword_tag_delete(struct safeword_db *db, const char *tag)
{
	int ret;
	char *sql;

	safeword_check(tag, ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql != NULL, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "DELETE FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
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
	safeword_check(sql != NULL, ESAFEWORD_NOMEM, fail);

	sprintf(sql, "UPDATE tags SET tag = '%s' WHERE tag = '%s';", new, old);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	free(sql);
fail:
	return ret;
}

/* #endregion safeword tag functions */

/* #region safeword cp functions */

/* returns the difference in milliseconds */
#ifdef WIN32
static unsigned int diff(LARGE_INTEGER *start, LARGE_INTEGER *end)
{
	LARGE_INTEGER frequency;

	QueryPerformanceFrequency(&frequency);

	return (end->QuadPart - start->QuadPart) * 1000.0 / frequency.QuadPart;
}
#else
static unsigned int diff(struct timespec *start, struct timespec *end)
{
	return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
		((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}
#endif

#ifndef WIN32
struct __async_waiting_data {
	volatile int *waiting;
	Display *dpy;
	Window *win;
	char *data;
	struct safeword_db *db;
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

				if (data->db->config.copy_once) {
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

static int safeword_cp(struct safeword_db *db, int credential_id, unsigned int ms,
	const char* table, const char* field)
{
	int ret;
	char sql[256];
	sqlite3_stmt *stmt = NULL;
	const char *to_copy = NULL;

	sprintf(sql, "SELECT t.%s FROM %s AS t "
		"INNER JOIN credentials AS c "
		"ON (c.%sid = t.id) WHERE c.id = %d;", field, table, field, credential_id);

	ret = sqlite3_prepare_v2(db->handle, sql, strlen(sql) + 1, &stmt, NULL);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);
	ret = sqlite3_step(stmt);
	if (ret == SQLITE_ROW) {
		to_copy = (const char*) sqlite3_column_text(stmt, 0);
	}

#ifdef WIN32
	DWORD len = strlen(to_copy);
	HGLOBAL lock;
	LPSTR data;

	lock = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len + 1) * sizeof(char));
	data = (LPWSTR)GlobalLock(lock);
	memcpy(data, to_copy, len * sizeof(char));
	data[len] = 0;
	GlobalUnlock(lock);

	/* Set clipboard data */
	if (!OpenClipboard(NULL)) return GetLastError();
	EmptyClipboard();
	if (!SetClipboardData(CF_TEXT, lock)) return GetLastError();
	CloseClipboard();
#else
	volatile int waiting = 1;
	char *data;
	struct timespec start, now;
	Display *dpy;
	Window win;

	/* copy the data to local variable that will stay in scope for waiting thread */
	data = calloc(strlen(to_copy) + 1, sizeof(char));
	safeword_check(data != NULL, ESAFEWORD_NOMEM, fail);

	strcpy(data, to_copy);

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
	waiting_data.db = db;

	pthread_t tid;
	pthread_create(&tid, NULL, &wait_for_clipboard_request, &waiting_data);

	while (waiting) {
		if (diff(&start, &now) > ms) {
			pthread_cancel(tid);
			waiting = 0;
		}

		clock_gettime(CLOCK_MONOTONIC, &now);
	}
#endif
	ret = sqlite3_finalize(stmt);
	safeword_check(ret == SQLITE_OK, ESAFEWORD_BACKENDSTORAGE, fail);

	return 0;
fail:
	return -1;
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

