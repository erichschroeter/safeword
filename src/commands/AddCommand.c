#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>

#include <safeword.h>
#include "AddCommand.h"

static char* username = NULL;
static char* password = NULL;
static char* description = NULL;
static char* tags = NULL;

static int username_id;
static int password_id;

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
		{"message",	required_argument,	NULL,	'm'},
		{"tag",	required_argument,	NULL,	't'},
	};

	while ((c = getopt_long(argc, argv, "m:t:", long_options, 0)) != -1) {
		switch (c) {
		case 'm':
			description = calloc(strlen(optarg), sizeof(char));
			if (!description) {
				ret = -ENOMEM;
				goto fail;
			}
			description = strcpy(description, optarg);
			break;
		case 't':
			tags = calloc(strlen(optarg), sizeof(char));
			if (!tags) {
				ret = -ENOMEM;
				goto fail;
			}
			strcpy(tags, optarg);
			break;
		}
	}

	if ((argc - optind) < 2)
		return ret;

	username = calloc(strlen(argv[optind]), sizeof(char));
	if (!username) {
		ret = -ENOMEM;
		goto fail;
	}
	username = strcpy(username, argv[optind]);
	optind++;

	password = calloc(strlen(argv[optind]), sizeof(char));
	if (!password) {
		ret = -ENOMEM;
		goto fail_password;
	}
	password = strcpy(password, argv[optind]);

	return ret;

fail_password:
	free(username);
fail:
	return ret;
}

int addCmd_execute(void)
{
	int ret;
	sqlite3* handle;
	sqlite3_int64 credentials_rowid;
	char* sql;

	if (!username || !password)
		return 0;

	ret = safeword_db_open(&handle);
	if (ret) goto fail;

	ret = safeword_credential_add(handle, &credentials_rowid, username, password, description);
	if (ret) goto fail;

	if (tags) {
		char *tag;

		tag = strtok(tags, ",");
		while (tag != NULL) {
			safeword_tag_credential(handle, credentials_rowid, tag);
			tag = strtok(NULL, ",");
		}
	}

	sqlite3_close(handle);

fail:
	free(username);
	free(password);
	free(description);
	free(tags);
	return ret;
}
