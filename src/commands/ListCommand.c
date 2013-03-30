#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <string.h>
#include <limits.h>

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
		safeword_check(tags, -ENOMEM, fail);

		for (i = 0; i < remaining_args; i++) {
			tags[i] = calloc(strlen(argv[optind]), sizeof(char));
			safeword_check(tags[i], -ENOMEM, fail);

			strcpy(tags[i], argv[optind]);
			tags_size++;
			optind++;
		}
	}

fail:
	return 0;
}

int listCmd_execute(void)
{
	int ret = 0, i;
	struct safeword_db db;

	ret = safeword_db_open(&db, 0);
	safeword_check(!ret, ret, fail);

	if (tags && !printAll) {
		safeword_list_credentials(&db, tags_size, tags);
	} else {
		if (printAll) {
			safeword_list_credentials(&db, UINT_MAX, 0);
		} else {
			safeword_list_credentials(&db, 0, 0);
		}
	}

	safeword_close(&db);
fail:
	for (i = 0; i < tags_size; i++)
		free(tags[i]);
	free(tags);
	return ret;
}
