#ifndef __SAFEWORD_ERRNO_H
#define __SAFEWORD_ERRNO_H

/*
 * This file is specifically for the safeword cli application error codes. Error codes
 * for the safeword library are located in safeword.h
 */

#define ESAFEWORD_IO         512 /* general purpose I/O error */
#define ESAFEWORD_COMMAND    513 /* invalid command */
#define ESAFEWORD_STATE      514 /* invalid state */
#define ESAFEWORD_ILLEGALARG 515 /* illegal argument */

#endif // __SAFEWORD_ERRNO_H
