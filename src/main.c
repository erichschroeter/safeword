#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "main.h"
#include "safeword.h"
#include "commands/Command.h"

void print_usage()
{
	printf("safeword COMMAND [OPTIONS] [ARGS]\n");
}

void print_version()
{
	printf("%s safeword-%s\n", SAFEWORD_CLI_VERSION, SAFEWORD_VERSION);
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
				case 0:
					break;
				case -ESAFEWORD_DBEXIST:
					fprintf(stderr, "database does not exist. See 'safeword help init'\n");
					break;
				default:
					fprintf(stderr, "safeword execute error: '%s'\n", safeword_strerror(res));
				}
				break;
			default:
				fprintf(stderr, "safeword parsing error: '%s'\n", safeword_strerror(res));
			}
		} else if (matches) {
			print_possible_commands(command_str);
		} else {
			fprintf(stderr, "'%s' is not a valid command.\n", command_str);
		}
	}

	return 0;
}
