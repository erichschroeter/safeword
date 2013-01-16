#ifndef COMMAND_H
#define COMMAND_H

#include <stddef.h>

/**
 * The Command struct provides the methods required for a <b>safeword</b>
 * command to be executed. The init() function is called before the execute()
 * method in order to allow the command's state to be initialized.
 * <p>
 * In order to properly use this interface as it was originally intended, you
 * must include <code>Command.h</code> in a "derived" file (e.g. HelpCommand.h),
 * define a <code>struct command</code>, and then specify the required functions
 * in the struct. This is the equivalent of implementing an interface in C.
 */
struct command {
	char*	name;
	char*	(*help)(void);
	int	(*parse)(int argc, char** argv);
	int	(*execute)(void);
};

extern struct command command_table[];
extern const size_t command_table_size;

#endif
