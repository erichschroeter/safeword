#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <getopt.h>
#include <sqlite3.h>

#include "InitCommand.h"

static int force;

static char* file = NULL;

char* initCmd_help(void)
{
	return "SYNOPSIS\n"
"	init [-f | --force] FILE\n"
"OPTIONS\n"
"	-f, --force\n"
"	    Force the database file initialization. This will overwrite the\n"
"	    existing file.\n"
"\n";
}

int initCmd_parse(int argc, char** argv)
{
	int ret = 0;
	int c;
	struct option long_options[] = {
		{"force",	no_argument,	NULL,	'f'},
	};

	while ((c = getopt_long(argc, argv, "f", long_options, 0)) != -1) {
		switch (c) {
		case 'f':
			force = 1;
			break;
		}
	}

	/*  must have at least one remaining argument after parsing options */
	if ((argc - optind) < 1)
		return ret;

	if (!access(argv[optind], F_OK) && !force) {
		fprintf(stderr,
			"'%s' already exists. Use --force to overwrite.\n",
			argv[optind]);
		ret = -EEXIST;
		goto fail;
	}

	file = calloc(strlen(argv[optind]), sizeof(char));
	if (!file) {
		ret = -ENOMEM;
		goto fail;
	}
	file = strcpy(file, argv[optind]);

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

	if (!access(file, F_OK)) {
		if (force) {
			if  (remove(file) != 0) {
				ret = -EIO;
				goto fail;
			}
			printf("removed '%s'\n", file);
		} else {
			ret = -EEXIST;
			goto fail;
		}
	}

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
		"id INTEGER PRIMARY KEY, "
		"usernameid INTEGER REFERENCES usernames(id), "
		"passwordid INTEGER REFERENCES passwords(id), "
		"description TEXT "
		")";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	query = "CREATE TABLE IF NOT EXISTS tagged_credentials ("
		"credentialid INTEGER NOT NULL REFERENCES credentials(id), "
		"tagid INTEGER NOT NULL REFERENCES tags(id), "
		"PRIMARY KEY (credentialid, tagid) "
		")";
	ret = sqlite3_exec(handle, query, 0, 0, 0);

	sqlite3_close(handle);

fail:
	return 0;
}
