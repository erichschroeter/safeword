#ifndef TESTS_SAFEWORD_CONFIG_H
#define TESTS_SAFEWORD_CONFIG_H

#include <CUnit/Basic.h>

void test_safeword_config_null_db(void);
void test_safeword_config_nonexistent(void);
void test_safeword_config_examples(void);
extern CU_TestInfo tests_config_null[];
extern CU_TestInfo tests_config_examples[];

#endif /* TESTS_SAFEWORD_CONFIG_H */
