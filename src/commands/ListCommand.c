#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <sqlite3.h>

#include <safeword.h>
#include "ListCommand.h"

int printAll;
static char** tags;
static int tags_size;

char* listCmd_help(void)
{
	return "SYNOPSIS\n"
"	list [-a | --all] [ TAGS ... ]\n"
"DESCRIPTION\n"
"	This command lists the credentials stored in the safeword database.\n"
"	Without any arguments only credentials with tags are displayed,\n"
"	otherwise only credentials with the specified tags are displayed.\n"
"\n"
"OPTIONS\n"
"	-a, --all\n"
"	    list all credentials\n"
"\n";
}

int listCmd_parse(int argc, char** argv)
{
	int ret, i, remaining_args = 0, c;
	struct option long_options[] = {
		{"all",	no_argument,	NULL,	'a'},
	};

	while ((c = getopt_long(argc, argv, "a", long_options, 0)) != -1) {
		switch (c) {
		case 'a':
			printAll = 1;
			break;
		}
	}

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
	int ret = 0, i;
	sqlite3 *handle;
	char *sql;

	ret = safeword_db_open(&handle);
	if (ret)
		goto fail;

	if (tags) {
		char *tags_concat;
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
		sprintf(sql, "SELECT c.id,c.description FROM credentials AS c "
			"INNER JOIN tagged_credentials AS tc "
			"INNER JOIN tags AS t ON "
			"(c.id = tc.credentialid AND tc.tagid = t.id) WHERE "
			"t.tag IN (%s) GROUP BY c.id HAVING COUNT(c.id) = %d;",
			tags_concat, tags_size);
		ret = sqlite3_exec(handle, sql, credentials_callback, 0, 0);
		free(tags_concat);
		free(sql);
	} else {
		if (printAll) {
			sql = calloc(100, sizeof(char));
			if (!sql) {
				ret = -ENOMEM;
				goto fail;
			}
			sprintf(sql, "SELECT id,description FROM credentials;");
			ret = sqlite3_exec(handle, sql, credentials_callback, 0, 0);
			free(sql);
		} else {
			sql = calloc(256, sizeof(char));
			if (!sql) {
				ret = -ENOMEM;
				goto fail;
			}
			sprintf(sql, "SELECT id,description FROM credentials "
			"WHERE id NOT IN ("
			"SELECT credentialid FROM tagged_credentials "
			"WHERE tagid IN ("
			"SELECT id FROM tags));");
			ret = sqlite3_exec(handle, sql, credentials_callback, 0, 0);
			free(sql);
		}
	}

	sqlite3_close(handle);

fail:
	for (i = 0; i < tags_size; i++)
		free(tags[i]);
	free(tags);
	return ret;
}
