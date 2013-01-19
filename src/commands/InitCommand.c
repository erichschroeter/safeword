#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
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
	int force = 0;
	int c;
	struct option long_options[] = {
		{"force",	no_argument,	NULL,	'f'},
	};

	while ((c = getopt_long(argc, argv, "f", long_options, 0)) != -1) {
		switch (c) {
		case 'f':
			force++;
			break;
		}
	}

	if (argc < 2)
		return ret;

	if (!access(argv[1], F_OK) && !force) {
		fprintf(stderr,
			"'%s' already exists. Use --force to overwrite.\n",
			argv[1]);
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
	char* query;

	if (!file)
		return 0;

	ret = sqlite3_open(file, &handle);
	if (ret) {
		fprintf(stderr, "failed to open database\n");
		return -ret;
	}
	free(file);

	query = "CREATE TABLE IF NOT EXISTS tags "
		"(id INTEGER PRIMARY KEY, tag TEXT)";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	query = "CREATE TABLE IF NOT EXISTS usernames "
		"(id INTEGER PRIMARY KEY, username TEXT)";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	query = "CREATE TABLE IF NOT EXISTS passwords "
		"(id INTEGER PRIMARY KEY, password TEXT)";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	query = "CREATE TABLE IF NOT EXISTS credentials ("
		"id INTEGER PRIMARY KEY,"
		"usernameid INTEGER,"
		"passwordid INTEGER,"
		"description TEXT,"
		"FOREIGN KEY(usernameid) REFERENCES usernames(id)"
		"FOREIGN KEY(passwordid) REFERENCES passwords(id)"
		")";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	query = "CREATE TABLE IF NOT EXISTS tagged_credentials ("
		"credentialid INTEGER,"
		"tagid INTEGER,"
		"FOREIGN KEY(credentialid) REFERENCES credentials(id)"
		"FOREIGN KEY(tagid) REFERENCES tags(id)"
		")";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	sqlite3_close(handle);

	return 0;
}
