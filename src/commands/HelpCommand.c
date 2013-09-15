#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>

#include "Command.h"
#include "HelpCommand.h"

enum help_format {
	HELP_FORMAT_NONE,
	HELP_FORMAT_MAN,
	HELP_FORMAT_HTML
};

char* helpCmd_help(void)
{
	return "usage: safeword [--version] [--help]\n"
"\tCOMMAND [ARGS]\n"
"\n"
"See 'safeword help COMMAND' for more information on a specific command.\n"
"\n";
}

int helpCmd_run(int argc, char** argv)
{
	int i, c, command_index = -1;
	enum help_format format = HELP_FORMAT_NONE;
	struct option long_options[] = {
		{"man",  no_argument, NULL, 'm'},
		{"html", no_argument, NULL, 'h'},
	};
	char *helpcmd = NULL;

	while ((c = getopt_long(argc, argv, "mh", long_options, 0)) != -1) {
		switch (c) {
		case 'm':
			format = HELP_FORMAT_MAN;
			break;
		case 'h':
			format = HELP_FORMAT_HTML;
			break;
		}
	}

	/* We need a command to show help for. */
	if (argc - optind < 1) {
		printf("%s", helpCmd_help());
		goto fail;
	}

	helpcmd = argv[optind];

	/* Find the command to show help for. */
	for (i = 0; i < command_table_size; i++) {
		if (!strncmp(command_table[i].name, helpcmd, strlen(helpcmd))) {
			command_index = i;
		}
	}

	if (command_index < 0) {
		fprintf(stderr, "no help for '%s' command.\n", helpcmd);
		goto fail;
	}

	char cmd[256];
	cmd[0] = 0;
	switch (format) {
	case HELP_FORMAT_HTML:
		sprintf(cmd, "google-chrome https://raw.github.com/erichschroeter/safeword/master/doc/asciidoc/safeword-%s.txt", command_table[command_index].name);
		break;
	case HELP_FORMAT_MAN:
	case HELP_FORMAT_NONE:
	default:
		sprintf(cmd, "man safeword-%s", command_table[command_index].name);
		break;
	}
	system(cmd);

	return 0;
fail:
	return -1;
}
