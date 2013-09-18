#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <safeword.h>
#include "AddCommand.h"

static int read_file_to_string(char *file, char **strptr)
{
	int ret = 0;
	long file_size = 0;
	FILE *fd = NULL;

	/* Special case: use stdin if file is hyphen "-". */
	if (!strcmp(file, "-")) {
		fd = stdin;
	} else {
		fd = fopen(file, "r");
		if (fd == NULL) return errno;
	}

	if (fd == stdin) {
		/* Read from standard input into a string. */
		char buffer[1024];
		buffer[0] = '\0';
		file_size = 0;

		*strptr = calloc(1024, sizeof(char));
		safeword_check(*strptr, -ENOMEM, fail);

		while (fgets(buffer, 1024, fd)) {
			char *old = *strptr;
			file_size += strlen(buffer);
			*strptr = realloc(*strptr, file_size + 1);
			if (!*strptr) {
				ret = -ENOMEM;
				free(old);
				goto fail;
			}
			strcat(*strptr, buffer);
		}
	} else {
		/* Read from specified file into a string. */
		ret = fseek(fd, 0, SEEK_END);
		safeword_check(ret != -1, errno, fail);

		file_size = ftell(fd);
		safeword_check(file_size != -1, errno, fail);

		ret = fseek(fd, 0, SEEK_SET);
		safeword_check(ret != -1, errno, fail);

		*strptr = calloc(file_size + 1, sizeof(char));
		safeword_check(*strptr, -ENOMEM, fail);

		ret = fread(*strptr, 1, file_size, fd);
		safeword_check(ret == file_size, errno, fail);
	}

	return 0;
fail:
	free(*strptr);
	return -1;
}

int addCmd_parse(int argc, char** argv, char **username, char **password,
	char **desc, char **note, char **tags)
{
	int ret = 0, c;
	FILE *notefile = NULL;
	struct option long_options[] = {
		{"message", required_argument, NULL, 'm'},
		{"tag",     required_argument, NULL, 't'},
		{"note",    required_argument, NULL, 'n'},
	};

	while ((c = getopt_long(argc, argv, "m:t:n:", long_options, 0)) != -1) {
		switch (c) {
		case 'm':
			*desc = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(*desc != NULL, -ENOMEM, fail_options);
			strcpy(*desc, optarg);
			break;
		case 't':
			*tags = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(*tags != NULL, -ENOMEM, fail_options);
			strcpy(*tags, optarg);
			break;
		case 'n':
			if (!strncmp(optarg, "-", 1)) {
				ret = read_file_to_string(optarg, note);
				safeword_check(ret == 0, errno, fail_options);
			} else {
				notefile = fopen(optarg, "r");
				if (notefile == NULL) {
					/*
					 * Assume if the file doesn't exist that a
					 * string was specified.
					 */
					switch (errno) {
					case EISDIR:
					case EACCES:
						return 3;
					case ENOENT:
					default:
						*note = calloc(strlen(optarg) + 1, sizeof(char));
						safeword_check(*note != NULL, -ENOMEM, fail_options);
						strcpy(*note, optarg);
						break;
					}
				} else {
					fclose(notefile);
					ret = read_file_to_string(optarg, note);
					safeword_check(ret == 0, errno, fail_options);
				}
			}
			break;
		}
	}

	*username = calloc(strlen(argv[optind]) + 1, sizeof(char));
	safeword_check(*username != NULL, -ENOMEM, fail_username);
	*username = strcpy(*username, argv[optind]);
	optind++;

	*password = calloc(strlen(argv[optind]) + 1, sizeof(char));
	safeword_check(*password != NULL, -ENOMEM, fail_password);
	*password = strcpy(*password, argv[optind]);

	return ret;

fail_password:
	free(*username);
fail_username:
fail_options:
	free(*tags);
	free(*desc);
	return ret;
}

int addCmd_run(int argc, char** argv)
{
	int ret;
	struct safeword_db db;
	struct safeword_credential *cred;
	char *username = NULL, *password = NULL, *desc = NULL, *note = NULL, *tags = NULL;

	ret = addCmd_parse(argc, argv, &username, &password, &desc, &note, &tags);
	if (ret != 0) return ret;

	if (!username) {
		fprintf(stderr, "no username specified\n");
		ret = -ESAFEWORD_INVARG;
		goto fail;
	}
	if (!password) {
		fprintf(stderr, "no password specified\n");
		ret = -ESAFEWORD_INVARG;
		goto fail;
	}

	ret = safeword_open(&db, 0);
	safeword_check(!ret, ret, fail);

	cred = safeword_credential_create(username, password, desc, note);
	if (!cred) goto fail;

	ret = safeword_credential_add(&db, cred);
	safeword_check(ret == 0, ret, fail);

	if (tags) {
		char *tag;

		tag = strtok(tags, ",");
		while (tag != NULL) {
			safeword_credential_tag(&db, cred->id, tag);
			tag = strtok(NULL, ",");
		}
	}

	safeword_close(&db);
fail:
	if (username)
		free(username);
	if (password)
		free(password);
	if (desc)
		free(desc);
	if (note)
		free(note);
	if (tags)
		free(tags);
	return ret;
}
