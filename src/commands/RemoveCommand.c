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
}

int removeCmd_run(int argc, char** argv)
{
	int ret;
	struct safeword_db db;

	ret = removeCmd_parse(argc, argv);
	if (ret != 0) return ret;

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

	ret = safeword_credential_delete(&db, _credential_id);
	safeword_check(!ret, ret, fail_remove);

fail_remove:
	safeword_close(&db);
fail:
	return ret;
}
