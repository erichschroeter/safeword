#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sqlite3.h>

#include "ListCommand.h"

static char** tags;
static int tags_size;

char* listCmd_help(void)
{
	return "SYNOPSIS\n"
"	list [ TAGS ... ]\n"
"\n";
}

int listCmd_parse(int argc, char** argv)
{
	int ret, i, remaining_args = 0;

	remaining_args = argc - optind;
	if (remaining_args > 0) {
		tags_size = 0;
		tags = malloc(remaining_args * sizeof(*tags));
		if (!tags) {
			ret = -ENOMEM;
			goto fail;
		}

		for (i = 0; i < remaining_args; i++) {
			tags[i] = calloc(strlen(argv[optind]), sizeof(char));
			if (!tags[i]) {
				ret = -ENOMEM;
				goto fail;
			}
			strcpy(tags[i], argv[optind]);
			tags_size++;
			optind++;
		}
	}

fail:
	return 0;
}

static int credentials_callback(void* not_used, int argc, char** argv, char** col_name)
{
	char* description = argv[1];
	int credential_id = atoi(argv[0]);

	if (!description)
		description = "";

	printf("%ld : %s\n", credential_id, description);
	return 0;
}

int listCmd_execute(void)
{
	int ret, i;
	sqlite3* handle;
	char* sql;
	char* db;

	db = getenv("SAFEWORD_DB");
	if (!db) {
		ret = -ENOENT;
		goto fail;
	}
	ret = sqlite3_open(db, &handle);
	if (ret) {
		fprintf(stderr, "failed to open database\n");
		return -ret;
	}

	if (tags) {
		char* tags_concat;
		/* init to include the commas & null terminator */
		int tags_concat_size = tags_size + 1;

		for (i = 0; i < tags_size; i++) {
			/* add 2 for wrapping in single quotes */
			tags_concat_size += strlen(tags[i]) + 2;
		}
		tags_concat = calloc(tags_concat_size, sizeof(char));
		if (!tags_concat) {
			ret = -ENOMEM;
			goto fail;
		}
		for (i = 0; i < tags_size; i++) {
			strcat(tags_concat, "'");
			strcat(tags_concat, tags[i]);
			strcat(tags_concat, "'");
			if (i != tags_size - 1)
				strcat(tags_concat, ",");
		}

		sql = calloc(strlen(tags_concat) + 256, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}
		sprintf(sql, "SELECT id,description FROM credentials WHERE id IN ("
			"SELECT credentialid FROM tagged_credentials "
			"WHERE tagid IN "
			"(SELECT id FROM tags WHERE tag IN (%s) LIMIT 1));",
			tags_concat);
		ret = sqlite3_exec(handle, sql, credentials_callback, 0, 0);
		free(tags_concat);
	} else {
		sql = "SELECT id,description FROM credentials;";
		ret = sqlite3_exec(handle, sql, credentials_callback, 0, 0);
	}

	sqlite3_close(handle);

fail:
	for (i = 0; i < tags_size; i++)
		free(tags[i]);
	free(tags);
	free(sql);
	return 0;
}
