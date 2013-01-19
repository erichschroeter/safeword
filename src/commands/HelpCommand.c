#include <stdio.h>
#include <string.h>

#include "Command.h"
#include "HelpCommand.h"

static int command_index = -1;

char* helpCmd_help(void)
{
	return "usage: safeword [--version] [--help]\n"
"\tCOMMAND [ARGS]\n"
"\n"
"See 'safeword help COMMAND' for more information on a specific command.\n"
"\n";
}

int helpCmd_parse(int argc, char** argv)
{
	int i, ret = 0;

	if (argc < 2) {
		printf("%s", helpCmd_help());
		goto fail;
	}

	for (i = 0; i < command_table_size; i++) {
		if (!strncmp(command_table[i].name, argv[1], strlen(argv[1]))) {
			command_index = i;
		}
	}

fail:
	return ret;
}

int helpCmd_execute(void)
{
	if (command_index >= 0)
		printf("%s", command_table[command_index].help());

	return 0;
}
