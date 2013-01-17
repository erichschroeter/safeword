#include "HelpCommand.h"

Command HELP_COMMAND = {
	.help = helpCmd_help,
	.init = helpCmd_parse,
	.execute = helpCmd_execute
};

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
	return 0;
}

int helpCmd_execute(void)
{
	return 0;
}
