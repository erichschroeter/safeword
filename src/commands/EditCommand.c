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

static int editCmd_parse(int argc, char** argv, int *credential, char **username, char **password,
	char **desc, char **note)
{
	int ret = 0, c;
	FILE *notefile = NULL;
	struct option long_options[] = {
		{"username", required_argument, NULL, 'u'},
		{"password", required_argument, NULL, 'p'},
		{"message",  required_argument, NULL, 'm'},
		{"note",     required_argument, NULL, 'n'},
	};

	while ((c = getopt_long(argc, argv, "u:p:m:n:", long_options, 0)) != -1) {
		switch (c) {
		case 'u':
			*username = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(*username, -ENOMEM, fail);
			*username = strcpy(*username, optarg);
			break;
		case 'p':
			*password = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(*password, -ENOMEM, fail);
			*password = strcpy(*password, optarg);
			break;
		case 'm':
			*desc = calloc(strlen(optarg) + 1, sizeof(char));
			safeword_check(*desc, -ENOMEM, fail);
			*desc = strcpy(*desc, optarg);
			break;
		case 'n':
			if (!strncmp(optarg, "-", 1)) {
				ret = read_file_to_string(optarg, note);
				safeword_check(ret == 0, errno, fail);
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
						safeword_check(*note != NULL, -ENOMEM, fail);
						strcpy(*note, optarg);
						break;
					}
				} else {
					fclose(notefile);
					ret = read_file_to_string(optarg, note);
					safeword_check(ret == 0, errno, fail);
				}
			}
			break;
		}
	}

	/*  must have at least one remaining argument after parsing options */
	if ((argc - optind) < 1) {
		fprintf(stderr, "no credential id specified.\n");
		ret = -ESAFEWORD_ILLEGALARG;
		goto fail;
	}

	errno = 0;
	*credential = strtol(argv[optind], NULL, 10);
	if (errno) {
		fprintf(stderr, "failed to parse ID: %s\n", strerror(errno));
		goto fail;
	}

	return 0;

fail:
	free(*username);
	free(*password);
	free(*desc);
	free(*note);
	credential = 0; /* set to invalid id */

	return -1;
}

int editCmd_run(int argc, char** argv)
{
	int ret, credential;
	struct safeword_db db;
	struct safeword_credential cred;
	char *username = NULL, *password = NULL, *desc = NULL, *note = NULL;

	ret = editCmd_parse(argc, argv, &credential, &username, &password, &desc, &note);
	if (ret != 0) return ret;

	memset(&cred, 0, sizeof(cred));

	if (!credential) {
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

	cred.id = credential;
	cred.username = username;
	cred.password = password;
	cred.description = desc;
	cred.note = note;

	ret = safeword_credential_update(&db, &cred);
	safeword_check(!ret, ret, fail);

	safeword_close(&db);

fail:
	free(username);
	free(password);
	free(desc);
	free(note);

	return ret;
}
