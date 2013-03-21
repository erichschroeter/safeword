#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <sqlite3.h>
#include <time.h>

#include <safeword.h>
#include "CopyCommand.h"

static unsigned int seconds_ = 10;
static sqlite3_int64 credential_id;

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

static int diff(struct timespec *start, struct timespec *end)
{
	return (end->tv_sec + (end->tv_nsec / 1000000000)) -
		(start->tv_sec + (start->tv_nsec / 1000000000));
}

static int copy_credential_callback(void* secs, int argc, char** argv, char** col_name)
{
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
			seconds_ = optarg ? atoi(optarg) : 0;
			break;
		}
	}

	remaining_args = argc - optind;
	if (remaining_args)
		credential_id = atoi(argv[optind]);

fail:
	return ret;
}

int copyCmd_execute(void)
{
	int ret = 0;
	sqlite3 *handle;
	char *sql;

	ret = safeword_db_open(&handle);
	if (ret)
		goto fail;

	sql = calloc(256, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT p.password FROM passwords AS p "
		"INNER JOIN credentials AS c "
		"ON (c.passwordid = p.id) WHERE c.passwordid = %d;", credential_id);
	ret = sqlite3_exec(handle, sql, copy_credential_callback, &seconds_, 0);
	free(sql);

	sqlite3_close(handle);

fail:
	return ret;
}
