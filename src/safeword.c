#include <stdio.h>
#include <string.h>

#include "commands/Command.h"

void print_usage()
{
	printf("safeword command [OPTIONS] args\n");
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
	int res = 0;

	// minimum of 2 parameters (safeword command itself and a subcommand)
	if (argc < 2) {
		print_usage();
		return 0;
	}

	command_str = argv[1];

	if (command_str) {
		int i, matches = 0, command_index = -1;

		for (i = 0; i < command_table_size; i++) {
			if (!strncmp(command_table[i].name, command_str, strlen(command_str))) {
				matches++;
				command_index = i;
			}
		}

		if (command_index >= 0 && matches == 1) {
			res = command_table[command_index].parse(argc, argv);
			if (!res)
				command_table[command_index].execute();
		} else if (matches) {
			print_possible_commands(command_str);
		} else {
			fprintf(stderr, "'%s' is not a valid command.\n", command_str);
		}
	}

	return 0;
}
