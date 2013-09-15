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
	int i, remaining_args = 0, c;
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

int listCmd_run(int argc, char** argv)
{
	int ret = 0, i;
	struct safeword_db db;
	struct safeword_credential **credentials;
	unsigned int credentials_size;

	ret = listCmd_parse(argc, argv);
	if (ret != 0) return ret;

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

	if (tags && !printAll) {
		ret = safeword_credential_list(&db, tags_size, tags, &credentials_size, &credentials);
	} else {
		if (printAll) {
			ret = safeword_credential_list(&db, UINT_MAX, 0, &credentials_size, &credentials);
		} else {
			ret = safeword_credential_list(&db, 0, 0, &credentials_size, &credentials);
		}
	}

	safeword_check(ret == 0, safeword_errno, fail);

	for (i = 0; i < credentials_size; i++) {
		printf("%d: %s\n", credentials[i]->id, credentials[i]->description);
	}

	safeword_close(&db);
fail:
	for (i = 0; i < tags_size; i++)
		free(tags[i]);
	free(tags);
	return ret;
}
