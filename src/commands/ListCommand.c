#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sqlite3.h>

#include "ListCommand.h"

char* listCmd_help(void)
{
	return "SYNOPSIS\n"
"	list\n"
"\n";
}

int listCmd_parse(int argc, char** argv)
{
	return 0;
}

int credentials_callback(void* not_used, int argc, char** argv, char** col_name)
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
	int ret;
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

	sql = "SELECT id,description FROM credentials;";
	ret = sqlite3_exec(handle, sql, credentials_callback, 0, 0);

	sqlite3_close(handle);

fail:
	return 0;
}
