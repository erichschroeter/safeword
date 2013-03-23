#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>

#include <safeword.h>
#include "CopyCommand.h"

static unsigned int _seconds = 10;
static sqlite3_int64 _credential_id;

char* copyCmd_help(void)
{
	return "SYNOPSIS\n"
"	cp [-t[SECONDS] | --time[=SECONDS]] ID\n"
"\n"
"DESCRIPTION\n"
"	This command copies a credential to the clipboard.\n"
"\n"
"OPTIONS\n"
"	-t, --time\n"
"	    The number of seconds before the clipboard is cleared. Default is 10 seconds.\n"
"	    Specifying no argument is equivalent to entering 0 (entering 0 will not clear the\n"
"	    clipboard)\n"
"\n";
}

int copyCmd_parse(int argc, char** argv)
{
	int ret = 0, remaining_args = 0, c, backup;
	struct option long_options[] = {
		{"time",	optional_argument,	NULL,	't'},
	};

	while ((c = getopt_long(argc, argv, "t::", long_options, 0)) != -1) {
		switch (c) {
		case 't':
			_seconds = optarg ? atoi(optarg) : 0;
			break;
		}
	}

	remaining_args = argc - optind;
	if (remaining_args)
		_credential_id = atoi(argv[optind]);

fail:
	return ret;
}

int copyCmd_execute(void)
{
	int ret = 0;
	sqlite3 *handle;

	ret = safeword_db_open(&handle);
	if (ret)
		goto fail;

	safeword_cp_password(handle, _credential_id, _seconds * 1000);

	sqlite3_close(handle);

fail:
	return ret;
}
