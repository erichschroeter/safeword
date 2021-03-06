#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <safeword.h>
#include "ShowCommand.h"

static int _credential_id;
static char* _tag;

char* showCmd_help(void)
{
	return "SYNOPSIS\n"
"	show [ID | tag]\n"
"\n"
"DESCRIPTION\n"
"	This command displays information about a credential or tag in the safeword database.\n"
"\n";
}

int showCmd_parse(int argc, char** argv)
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

int showCmd_execute(void)
{
	int i, ret;
	struct safeword_db db;
	struct safeword_credential credential;
	struct safeword_tag tag;

	memset(&tag, 0, sizeof(tag));
	memset(&credential, 0, sizeof(credential));

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

	if (_credential_id) {
		credential.id = _credential_id;
		ret = safeword_credential_read(&db, &credential);
		printf("%s\nusername:%s\npassword:%s\n",
			credential.description, credential.username, credential.password);
		for (i = 0; i < credential.tags_size; i++) {
			if (i != 0)
				printf(", ");
			printf("%s", credential.tags[i]);
		}
		if (credential.tags_size) printf("\n");
	} else {
		memset(&tag, 0, sizeof(tag));
		tag.tag = _tag;
		ret = safeword_tag_read(&db, &tag);
		if (tag.wiki)
			printf("%s\n", tag.wiki);

	}

fail:
	free(_tag);
	safeword_close(&db);
	return ret;
}
