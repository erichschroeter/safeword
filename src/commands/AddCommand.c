#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>

#include "AddCommand.h"

static char* username = NULL;
static char* password = NULL;

char* addCmd_help(void)
{
	return "SYNOPSIS\n"
"	add USERNAME PASSWORD\n"
"\n";
}

int addCmd_parse(int argc, char** argv)
{
	int ret = 0;

	if (argc < 3)
		return ret;

	username = calloc(strlen(argv[1]), sizeof(char));
	if (!username) {
		ret = -ENOMEM;
		goto fail;
	}
	username = strcpy(username, argv[1]);

	password = calloc(strlen(argv[2]), sizeof(char));
	if (!password) {
		ret = -ENOMEM;
		goto fail_password;
	}
	password = strcpy(password, argv[2]);

	return ret;

fail_password:
	free(username);
fail:
	return ret;
}

int addCmd_execute(void)
{
	int ret;
	sqlite3* handle;
	char* sql;

	if (!username || !password)
		return 0;

	ret = sqlite3_open("/home/erich/safeword.sqlite3", &handle);
	if (ret) {
		fprintf(stderr, "failed to open database\n");
		return -ret;
	}

	sql = calloc(strlen(username) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}

	sprintf(sql, "INSERT INTO usernames (username) VALUES ('%s')", username);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

	sql = calloc(strlen(password) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}

	sprintf(sql, "INSERT INTO passwords (password) VALUES ('%s')", password);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);

	sqlite3_close(handle);

fail:
	free(sql);
	return ret;
}
