#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>

#include <safeword.h>
#include "CopyCommand.h"

static unsigned int _seconds = 10;
static int _credential_id;
#define COPYABLE_FIELDS	2
enum field {
	PASSWORD = 1,
	USERNAME = 2
};
static enum field _copy[COPYABLE_FIELDS];

char* copyCmd_help(void)
{
	return "SYNOPSIS\n"
"	cp [-t[SECONDS] | --time[=SECONDS]] [-u | --username] [-p | --password] ID\n"
"\n"
"DESCRIPTION\n"
"	This command copies a credential to the clipboard.\n"
"\n"
"OPTIONS\n"
"	-t, --time\n"
"	    The number of seconds before the clipboard is cleared. Default is 10 seconds.\n"
"	    Specifying no argument is equivalent to entering 0 (entering 0 will not clear the\n"
"	    clipboard)\n"
"	-p, --password\n"
"	    Copies the password to the clipboard.\n"
"	-u, --username\n"
"	    Copies the username to the clipboard.\n"
"\n";
}

int copyCmd_parse(int argc, char** argv)
{
	int ret = 0, remaining_args = 0, c, i = 0;
	struct option long_options[] = {
		{"time",	optional_argument,	NULL,	't'},
		{"username",	no_argument,	NULL,	'u'},
		{"password",	no_argument,	NULL,	'p'},
	};

	while ((c = getopt_long(argc, argv, "upt::", long_options, 0)) != -1) {
		switch (c) {
		case 't':
			_seconds = optarg ? atoi(optarg) : 0;
			break;
		case 'p':
			_copy[i] = PASSWORD;
			i++;
			break;
		case 'u':
			_copy[i] = USERNAME;
			i++;
			break;
		}
	}

	remaining_args = argc - optind;
	if (remaining_args)
		_credential_id = atoi(argv[optind]);

	return ret;
}

int copyCmd_execute(void)
{
	int ret = 0, i = 0;
	struct safeword_db db;

	ret = safeword_open(&db, 0);
	if (ret)
		goto fail;

	/*
	 * If any field options were specified, we only allow pasting each once. This allows
	 * falling through to multiple fields, saving the user additional actions of running
	 * this command multiple times.
	 */
	if (_copy[i] != 0) {
		safeword_config(&db, "copy_once", "1");

		do {
			switch (_copy[i]) {
			case USERNAME:
				safeword_cp_username(&db, _credential_id, _seconds * 1000);
				break;
			case PASSWORD:
			default:
				safeword_cp_password(&db, _credential_id, _seconds * 1000);
				break;
			};
			i++;
		} while (i < COPYABLE_FIELDS && _copy[i] != 0);
	} else {
		safeword_cp_password(&db, _credential_id, _seconds * 1000);
	}

fail:
	safeword_close(&db);
	return ret;
}
