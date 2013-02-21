#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "safeword.h"
#include "commands/Command.h"

static sqlite3_int64 tag_id;

static int tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	tag_id = atoi(argv[0]);
	return 0;
}

int safeword_db_open(sqlite3 **handle)
{
	int ret = 0;
	char *db = malloc(sizeof(char*));

	if (!db) {
		ret = -ENOMEM;
		goto fail;
	}
	db = getenv("SAFEWORD_DB");
	if (!db) {
		ret = -ESAFEWORD_ENV_NOT_SET;
		fprintf(stderr, "SAFEWORD_DB environment variable not set\n");
		goto fail;
	}

	// fail if file does not exist, otherwise sqlite3 will create it
	if (access(db, F_OK) == -1) {
		ret = -ESAFEWORD_DB_NOEXIST;
		fprintf(stderr, "safeword database '%s' does not exist\n", db);
		goto fail;
	}

	ret = sqlite3_open(db, handle);
	if (ret) {
		fprintf(stderr, "failed to open safeword database '%s'\n", db);
		goto fail;
	}

	/* enable foreign key support in Sqlite3 so delete cascading works. */
	ret = sqlite3_exec(*handle, "PRAGMA foreign_keys = ON;", 0, 0, 0);

fail:
	return ret;
}

int safeword_tag_credential(sqlite3 *handle, sqlite3_int64 credential_id, const char *tag)
{
	int ret;
	char *sql;

	tag_id = -1; /* set to invalid value to represent it does not exist */

	sql = calloc(strlen(tag) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(handle, sql, tag_callback, 0, 0);
	free(sql);

	if (tag_id == -1) {
		sql = calloc(strlen(tag) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}
		sprintf(sql, "INSERT INTO tags (tag) VALUES ('%s');", tag);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
		tag_id = sqlite3_last_insert_rowid(handle);
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "INSERT OR REPLACE INTO tagged_credentials "
		"(credentialid, tagid) VALUES (%d, %d);",
		credential_id, tag_id);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_delete_tag(sqlite3* handle, const char *tag)
{
	int ret;
	char *sql;

	sql = calloc(strlen(tag) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "DELETE FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

int safeword_rename_tag(sqlite3* handle, const char *old, const char *new)
{
	int ret;
	char *sql;

	sql = calloc(strlen(old) + strlen(new) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "UPDATE tags SET tag = '%s' WHERE tag = '%s';", new, old);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}
