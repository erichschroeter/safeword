#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <safeword.h>
#include "AddCommand.h"

static char* _username = NULL;
static char* _password = NULL;
static char* _description = NULL;
static char* _tags = NULL;

char* addCmd_help(void)
{
	return "SYNOPSIS\n"
"	add [-m | --message MESSAGE] [-t | --tag TAG,...] USERNAME PASSWORD\n"
"\n"
"DESCRIPTION\n"
"	This command adds credentials to the safeword database. The message\n"
"	option is helpful when listing credentials to see a short description\n"
"	of what the credential's intended purpose is.\n"
"\n"
"OPTIONS\n"
"	-m, --message\n"
"	    Set a description for what the credential is for.\n"
"	-t, --tag\n"
"	    Tag the credential with the comma-separated list of tags.\n"
"\n";
}

int addCmd_parse(int argc, char** argv)
{
	int ret = 0, c;
	struct option long_options[] = {
		{"message", required_argument, NULL, 'm'},
		{"tag",     required_argument, NULL, 't'},
	};

	while ((c = getopt_long(argc, argv, "m:t:", long_options, 0)) != -1) {
		switch (c) {
		case 'm':
			_description = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(_description, -ENOMEM, fail_options);
			_description = strcpy(_description, optarg);
			break;
		case 't':
			_tags = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(_tags, -ENOMEM, fail_options);
			strcpy(_tags, optarg);
			break;
		}
	}

	/* not enough args */
	safeword_check(((argc - optind) > 1), -ESAFEWORD_INVARG, fail_options);

	_username = calloc(strlen(argv[optind]) + 1, sizeof(char));
	safeword_check(_username, -ENOMEM, fail_username);
	_username = strcpy(_username, argv[optind]);
	optind++;

	_password = calloc(strlen(argv[optind]) + 1, sizeof(char));
	safeword_check(_password, -ENOMEM, fail_password);
	_password = strcpy(_password, argv[optind]);

	return ret;

fail_password:
	free(_username);
fail_username:
fail_options:
	free(_tags);
	free(_description);
	return ret;
}

int addCmd_execute(void)
{
	int ret;
	int credential_id;
	struct safeword_db db;

	if (!_username) {
		fprintf(stderr, "no username specified\n");
		ret = -ESAFEWORD_INVARG;
		goto fail;
	}
	if (!_password) {
		fprintf(stderr, "no password specified\n");
		ret = -ESAFEWORD_INVARG;
		goto fail;
	}

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

	ret = safeword_credential_add(&db, &credential_id, _username, _password, _description);
	safeword_check(!ret, ret, fail);

	if (_tags) {
		char *tag;

		tag = strtok(_tags, ",");
		while (tag != NULL) {
			safeword_credential_tag(&db, credential_id, tag);
			tag = strtok(NULL, ",");
		}
	}

	safeword_close(&db);
fail:
	free(_username);
	free(_password);
	free(_description);
	free(_tags);
	return ret;
}
