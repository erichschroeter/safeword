#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sqlite3.h>

#include "InitCommand.h"

static char* file = NULL;

char* initCmd_help(void)
{
	return NULL;
}

int initCmd_parse(int argc, char** argv)
{
	int ret = 0;

	if (argc < 2)
		return ret;

	if (!access(argv[1], F_OK)) {
		fprintf(stderr, "'%s' already exists\n", argv[1]);
		ret = -EEXIST;
		goto fail;
	}

	file = calloc(strlen(argv[1]), sizeof(char));
	if (!file) {
		ret = -ENOMEM;
		goto fail;
	}
	file = strcpy(file, argv[1]);

fail:
	return ret;
}

int initCmd_execute(void)
{
	int ret;
	sqlite3* handle;

	if (!file)
		return 0;

	ret = sqlite3_open(file, &handle);
	if (ret) {
		fprintf(stderr, "failed to open database\n");
		return -ret;
	}
	free(file);

	sqlite3_close(handle);

	return 0;
}
