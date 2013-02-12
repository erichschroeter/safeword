#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "safeword.h"
#include "commands/Command.h"

int safeword_db_open(sqlite3 **handle)
{
	int ret = 0;
	char *db = malloc(sizeof(char*));

	if (!db) {
		ret = -ENOMEM;
		goto fail;
	}
	db = getenv("SAFEWORD_DB");
	if (!db) {
		ret = -ESAFEWORD_ENV_NOT_SET;
		fprintf(stderr, "SAFEWORD_DB environment variable not set\n");
		goto fail;
	}

	// fail if file does not exist, otherwise sqlite3 will create it
	if (access(db, F_OK) == -1) {
		ret = -ESAFEWORD_DB_NOEXIST;
		fprintf(stderr, "safeword database '%s' does not exist\n", db);
		goto fail;
	}

	ret = sqlite3_open(db, handle);
	if (ret) {
		fprintf(stderr, "failed to open safeword database '%s'\n", db);
		goto fail;
	}

fail:
	return ret;
}
