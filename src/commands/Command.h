#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/**
 * The command struct provides information required for a safeword
 * command to be executed.
 *
 * In order to properly use this interface as it was originally intended,
 * one must include @c Command.h in a "derived" file (e.g. HelpCommand.h),
 * define a struct command, and then specify the required functions in
 * the struct.
 */
struct command {
	char*	name;
	int	(*run)(int argc, char** argv);
};

extern struct command command_table[];
extern const size_t command_table_size;

#endif
