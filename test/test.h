#ifndef SAFEWORD_TEST_H
#define SAFEWORD_TEST_H

#include <safeword.h>

extern struct safeword_db *db1;
extern struct safeword_credential examples[];
extern const unsigned int EXAMPLES_SIZE;

int suite_safeword_clean(void);
int suite_safeword_init(void);

#endif /* SAFEWORD_TEST_H */
