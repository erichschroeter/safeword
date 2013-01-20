#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>

#include "AddCommand.h"

static char* username = NULL;
static char* password = NULL;
static char* description = NULL;

static int username_id;

char* addCmd_help(void)
{
	return "SYNOPSIS\n"
"	add [-m | --message MESSAGE] USERNAME PASSWORD\n"
"\n";
}

int addCmd_parse(int argc, char** argv)
{
	int ret = 0, c;
	struct option long_options[] = {
		{"message",	required_argument,	NULL,	'm'},
	};

	while ((c = getopt_long(argc, argv, "m:", long_options, 0)) != -1) {
		switch (c) {
		case 'm':
			description = calloc(strlen(optarg), sizeof(char));
			if (!description) {
				ret = -ENOMEM;
				goto fail;
			}
			description = strcpy(description, optarg);
			break;
		}
	}

	if ((argc - optind) < 2)
		return ret;

	username = calloc(strlen(argv[optind]), sizeof(char));
	if (!username) {
		ret = -ENOMEM;
		goto fail;
	}
	username = strcpy(username, argv[optind]);
	optind++;

	password = calloc(strlen(argv[optind]), sizeof(char));
	if (!password) {
		ret = -ENOMEM;
		goto fail_password;
	}
	password = strcpy(password, argv[optind]);

	return ret;

fail_password:
	free(username);
fail:
	return ret;
}

static int username_callback(void* not_used, int argc, char** argv, char** col_name)
{
	username_id = atoi(argv[0]);

	return 0;
}

int map_username(sqlite3* handle, const char* username, sqlite3_int64 credential_rowid)
{
	int ret;
	char* sql;

	username_id = -1; // set to invalid value

	sql = calloc(strlen(username) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT id FROM usernames WHERE username='%s'", username);;
	ret = sqlite3_exec(handle, sql, username_callback, &credential_rowid, 0);
	free(sql);

	if (username_id == -1) {
		sql = calloc(strlen(username) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}

		sprintf(sql, "INSERT INTO usernames (username) VALUES ('%s')", username);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
		username_id = sqlite3_last_insert_rowid(handle);
	}

	sql = calloc(strlen(username) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}

	sprintf(sql, "UPDATE credentials SET usernameid = %d WHERE "
		"id = %d",
		username_id, (int)credential_rowid);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int addCmd_execute(void)
{
	int ret;
	sqlite3* handle;
	sqlite3_int64 credentials_rowid;
	char* sql;

	if (!username || !password)
		return 0;

	ret = sqlite3_open("/home/erich/safeword.sqlite3", &handle);
	if (ret) {
		fprintf(stderr, "failed to open database\n");
		return -ret;
	}

	sql = calloc(100, sizeof(char));
	sprintf(sql, "INSERT INTO credentials DEFAULT VALUES");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);
	credentials_rowid = sqlite3_last_insert_rowid(handle);
	if (description) {
		sql = calloc(strlen(description) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}

		sprintf(sql, "UPDATE credentials SET description = '%s' WHERE "
			"id = %d",
			description, credentials_rowid);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
	}

	map_username(handle, username, credentials_rowid);

	sqlite3_close(handle);

fail:
	free(username);
	free(password);
	free(description);
	return ret;
}
