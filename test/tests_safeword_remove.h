#ifndef TESTS_SAFEWORD_REMOVE_H
#define TESTS_SAFEWORD_REMOVE_H

#include <CUnit/Basic.h>

void test_safeword_remove_null_db(void);
void test_safeword_remove_one(void);
extern CU_TestInfo tests_remove_null[];
extern CU_TestInfo tests_remove_one[];

#endif /* TESTS_SAFEWORD_REMOVE_H */
