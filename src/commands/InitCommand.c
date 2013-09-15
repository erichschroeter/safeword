#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include <safeword.h>
#include <safeword_errno.h>
#include "InitCommand.h"

static int _force;

static char* _file = NULL;

char* initCmd_help(void)
{
	return "SYNOPSIS\n"
"	init [-f | --force] FILE\n"
"OPTIONS\n"
"	-f, --force\n"
"	    Force the database file initialization. This will overwrite the\n"
"	    existing file.\n"
"\n";
}

int initCmd_parse(int argc, char** argv)
{
	int ret = 0;
	int c;
	struct option long_options[] = {
		{"force",	no_argument,	NULL,	'f'},
	};

	while ((c = getopt_long(argc, argv, "f", long_options, 0)) != -1) {
		switch (c) {
		case 'f':
			_force = 1;
			break;
		}
	}

	/*  must have at least one remaining argument after parsing options */
	if ((argc - optind) < 1)
		return ret;

	if (!access(argv[optind], F_OK) && !_force) {
		fprintf(stderr,
			"'%s' already exists. Use --force to overwrite.\n",
			argv[optind]);
		ret = -EEXIST;
		goto fail;
	}

	_file = calloc(strlen(argv[optind]), sizeof(char));
	if (!_file) {
		ret = -ENOMEM;
		goto fail;
	}
	_file = strcpy(_file, argv[optind]);

fail:
	return ret;
}

int initCmd_run(int argc, char** argv)
{
	int ret;

	ret = initCmd_parse(argc, argv);
	if (ret != 0) return ret;

	if (!_file) {
		fprintf(stderr, "no path specified\n");
		ret = -ESAFEWORD_DBEXIST;
		goto fail;
	}

	if (!access(_file, F_OK)) {
		if (_force) {
			if  (remove(_file) != 0) {
				ret = -ESAFEWORD_IO;
				goto fail;
			}
		} else {
			ret = -ESAFEWORD_DBEXIST;
			goto fail;
		}
	}

	ret = safeword_init(_file);

fail:
	return ret;
}
