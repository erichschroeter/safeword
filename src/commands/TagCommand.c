#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>

#include <safeword.h>
#include "TagCommand.h"

static int *credential_ids;
static int credential_ids_size;
static char **tags;
static int tags_size;
static sqlite3_int64 tag_id;

char* tagCmd_help(void)
{
	return "SYNOPSIS\n"
"	tag ID TAGS ...\n"
"\n";
}

int tagCmd_parse(int argc, char** argv)
{
	int ret = 0, remaining_args = 0, c, backup;
	struct option long_options[] = {
		{"untag",	no_argument,	NULL,	'u'},
		{"list",	no_argument,	NULL,	'l'},
	};

	while ((c = getopt_long(argc, argv, "", long_options, 0)) != -1) {
		switch (c) {
		case 'l':
			break;
		case 'u':
			break;
		}
	}

	remaining_args = argc - optind;
	backup = optind;
	if (remaining_args > 1) {
		int i = 0, id;
		char *id_str;
		char *ids_backup = calloc(strlen(argv[optind]), sizeof(char));
		tags_size = 0;
		tags = malloc(remaining_args * sizeof(*tags));
		strcpy(ids_backup, argv[optind]);

		/* get number of credential ids in string */
		id_str = strtok(ids_backup, ",");
		while (id_str != NULL) {
			id_str = strtok(NULL, ",");
			credential_ids_size++;
		}

		credential_ids = calloc(credential_ids_size, sizeof(*credential_ids));
		ids_backup = argv[optind];
		id_str = strtok(ids_backup, ",");
		while (id_str != NULL) {
			id = atoi(id_str);
			credential_ids[i] = id;
			id_str = strtok(NULL, ",");
			i++;
		}
		optind++;

		for (i = 0; i < remaining_args - 1; i++) {
			tags[i] = calloc(strlen(argv[optind]), sizeof(char));
			if (!tags[i]) {
				ret = -ENOMEM;
				goto fail;
			}
			strcpy(tags[i], argv[optind]);
			tags_size++;
			optind++;
		}
		optind = backup;
	}
fail:
	return ret;
}

static int tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	tag_id = atoi(argv[0]);

	return 0;
}

int map_tag(sqlite3* handle, const char* tag, sqlite3_int64 credential_id)
{
	int ret;
	char *sql;

	tag_id = -1; /* set to invalid value to represent it does not exist */

	sql = calloc(strlen(tag) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT id FROM tags WHERE tag='%s'", tag);
	ret = sqlite3_exec(handle, sql, tag_callback, &credential_id, 0);
	free(sql);

	if (tag_id == -1) {
		sql = calloc(strlen(tag) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}
		sprintf(sql, "INSERT INTO tags (tag) VALUES ('%s')", tag);
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
		"(credentialid, tagid) VALUES (%d, %d)",
		credential_id, tag_id);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

static int print_tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	printf("%s\n", argv[0]);
	return 0;
}

int tagCmd_execute(void)
{
	int ret = 0, i, j;
	sqlite3 *handle;
	char *sql;

	ret = safeword_db_open(&handle);
	if (ret)
		goto fail;

	if (tags) {
		for (i = 0; i < credential_ids_size; i++) {
			for (j = 0; j < tags_size; j++) {
				map_tag(handle, tags[j], credential_ids[i]);
			}
		}
	} else {
		sql = calloc(100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}
		sprintf(sql, "SELECT tag FROM tags");
		ret = sqlite3_exec(handle, sql, print_tag_callback, 0, 0);
		free(sql);
	}

	sqlite3_close(handle);

fail:
	for (i = 0; i < tags_size; i++)
		free(tags[i]);
	free(tags);
	return ret;
}
