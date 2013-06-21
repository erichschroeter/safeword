#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <safeword.h>
#include "InfoCommand.h"

static int _credential_id;
static char* _tag;

char* infoCmd_help(void)
{
	return "SYNOPSIS\n"
"	info [ID | tag]\n"
"\n"
"DESCRIPTION\n"
"	This command displays information about a credential or tag in the safeword database.\n"
"\n";
}

int infoCmd_parse(int argc, char** argv)
{
	int ret = 0;
	char *invalid = 0;

	if (argc < 2)
		return ret;

	_credential_id = strtol(argv[1], &invalid, 10);
	if (!_credential_id) {
		_tag = calloc(strlen(argv[1]) + 1, sizeof(char));
		strcpy(_tag, argv[1]);
		/* we assume there will never be a credential id of 0 */
		_credential_id = 0;
	}

	return ret;
}

int infoCmd_execute(void)
{
	int ret;
	struct safeword_db db;
	struct safeword_credential credential;

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

	if (_credential_id) {
		credential.id = _credential_id;
		ret = safeword_credential_read(&db, &credential);
		fprintf(stdout,
			"DESCRIPTION: %s\n"
			"USERNAME   : %s\n"
			"PASSWORD   : %s\n",
			credential.description, credential.username, credential.password);
	} else {
		ret = safeword_tag_info(&db, _tag);
	}

fail:
	free(_tag);
	safeword_close(&db);
	return ret;
}
