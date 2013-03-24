#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

#include <safeword.h>
#include "RemoveCommand.h"

static int _credential_id;

char* removeCmd_help(void)
{
	return "SYNOPSIS\n"
"	remove ID\n"
"\n"
"DESCRIPTION\n"
"	This command removes credentials from the safeword database.\n"
"\n";
}

int removeCmd_parse(int argc, char** argv)
{
	int ret = 0;

	if (argc < 2)
		return ret;

	_credential_id = atoi(argv[1]);

	return ret;

fail:
	return ret;
}

int removeCmd_execute(void)
{
	int ret;
	sqlite3* handle;
	char* sql;

	ret = safeword_db_open(&handle);
	if (ret) goto fail;

	ret = safeword_credential_remove(handle, _credential_id);
	if (ret) goto fail;

	sqlite3_close(handle);

fail:
	return ret;
}
