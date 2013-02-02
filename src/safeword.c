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

void print_usage()
{
	printf("safeword COMMAND [OPTIONS] [ARGS]\n");
}

void print_version()
{
	printf("%d.%d.%d\n", VERSION_MAJOR, VERSION_MINOR, VERSION_PATCH);
}

void print_supported_commands()
{
	int i;

	printf("supported commands:\n");
	for (i = 0; i < command_table_size; i++) {
		printf("\t%s\n", command_table[i].name);
	}
}

void print_possible_commands(const char* command_str)
{
	int i;

	printf("possible commands matching '%s':\n", command_str);
	for (i = 0; i < command_table_size; i++) {
		if (!strncmp(command_table[i].name, command_str, strlen(command_str))) {
			printf("  '%s'\n", command_table[i].name);
		}
	}
}

int main(int argc, char** argv)
{
	const char* command_str;
	int res = 0, i, command_index = 1;

	/* process any options before subcommand */
	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '-') {
			if (!strcmp("-v", argv[i]) || !strcmp("--version", argv[i])) {
				print_version();
				return 0;
			} else {
				fprintf(stderr, "unknown option '%s'\n", argv[i]);
				return 1;
			}
			command_index++;
		} else {
			break;
		}
	}

	/* minimum of 2 parameters (safeword command itself and a subcommand) */
	if (argc < 2) {
		print_usage();
		print_supported_commands();
		return 0;
	}

	command_str = argv[command_index];

	if (command_str) {
		int i, matches = 0, command_index = -1;

		for (i = 0; i < command_table_size; i++) {
			if (!strncmp(command_table[i].name, command_str, strlen(command_str))) {
				matches++;
				command_index = i;
			}
		}

		if (command_index >= 0 && matches == 1) {
			res = command_table[command_index].parse(--argc, ++argv);
			switch (res) {
			case 0:
				res = command_table[command_index].execute();
				switch (res) {
				case -ESAFEWORD_DB_NOEXIST:
					fprintf(stderr, "database does not exist. See 'safeword help init'\n");
					break;
				}
				break;
			}
		} else if (matches) {
			print_possible_commands(command_str);
		} else {
			fprintf(stderr, "'%s' is not a valid command.\n", command_str);
		}
	}

	return 0;
}
