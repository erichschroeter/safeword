#ifndef TESTS_SAFEWORD_READ_H
#define TESTS_SAFEWORD_READ_H

#include <CUnit/Basic.h>

void test_safeword_read_null_db(void);
void test_safeword_read_invalid_id(void);
void test_safeword_read_examples(void);
extern CU_TestInfo tests_read_null[];
extern CU_TestInfo tests_read_examples[];

#endif /* TESTS_SAFEWORD_READ_H */
