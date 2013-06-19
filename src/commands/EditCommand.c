#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>

#include <safeword.h>
#include <safeword_errno.h>
#include "EditCommand.h"
#include "dbg.h"

static int _credential_id;
static char *_username = 0;
static char *_password = 0;
static char *_message = 0;

char* editCmd_help(void)
{
	return "SYNOPSIS\n"
"	edit [-uUSERNAME | --username=USERNAME] [-pPASSWORD | --password=PASSWORD]\n"
"		[-mMESSAGE | --message=MESSAGE] ID\n"
"\n"
"DESCRIPTION\n"
"	The edit command edits credential information. With no options the credential information is\n"
"	opened in the default text editor to manually edit.\n"
"\n"
"OPTIONS\n"
"	-u, --username\n"
"	    Edit the username.\n"
"	-p, --password\n"
"	    Edit the password.\n"
"	-m, --message\n"
"	    Edit the message.\n"
"\n";
}

int editCmd_parse(int argc, char** argv)
{
	int ret = 0, c;
	struct option long_options[] = {
		{"username", required_argument, NULL, 'u'},
		{"password", required_argument, NULL, 'p'},
		{"message",  required_argument, NULL, 'm'},
	};

	while ((c = getopt_long(argc, argv, "u:p:m:", long_options, 0)) != -1) {
		switch (c) {
		case 'u':
			_username = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(_username, -ENOMEM, fail);
			_username = strcpy(_username, optarg);
			break;
		case 'p':
			_password = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(_password, -ENOMEM, fail);
			_password = strcpy(_password, optarg);
			break;
		case 'm':
			_message = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(_message, -ENOMEM, fail);
			_message = strcpy(_message, optarg);
			break;
		}
	}

	/*  must have at least one remaining argument after parsing options */
	if ((argc - optind) < 1) {
		fprintf(stderr, "no credential id specified.\n");
		ret = -ESAFEWORD_ILLEGALARG;
		goto fail;
	}

	_credential_id = strtol(argv[optind], NULL, 10);
	if (errno) {
		fprintf(stderr, "failed to parse ID: %s\n", strerror(errno));
		goto fail;
	}

	return ret;

fail:
	free(_username);
	free(_password);
	free(_message);
	_credential_id = 0; /* set to invalid id */

	return ret;
}

int editCmd_execute(void)
{
	int ret;
	struct safeword_db db;
	struct safeword_credential cred;

	memset(&cred, 0, sizeof(cred));

	if (!_credential_id) {
		ret = -ESAFEWORD_ILLEGALARG;
		goto fail;
	}

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

#if 0
	if (!_username && !_password && !_message) {
		/* TODO open text editor */
		char *env = getenv("EDITOR");
		if (!env) {
			debug("EDITOR environment variable not set");
			env = "vi";
		}
		printf("opening '%s' to edit credential %d\n", env, _credential_id);

		FILE *temp = fopen("/tmp/SAFEWORD_EDITCRED", "w+");
		safeword_check(temp, errno, fail);

		ret = safeword_credential_info(&db, &cred);
		fprintf(temp, "username=%s\n", cred.username);
		fprintf(temp, "password=%s\n", cred.password);
		fprintf(temp, "message=%s\n", cred.message);
		safeword_credential_free(&cred);

		fclose(temp);

		/* TODO parse input from text editor */
	}
#endif

	cred.id = _credential_id;
	cred.username = _username;
	cred.password = _password;
	cred.description = _message;

	ret = safeword_credential_update(&db, &cred);
	safeword_check(!ret, ret, fail);

	safeword_close(&db);

fail:
	free(_username);
	free(_password);
	free(_message);

	return ret;
}
