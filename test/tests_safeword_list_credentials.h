#ifndef TESTS_SAFEWORD_LIST_CREDENTIALS_H
#define TESTS_SAFEWORD_LIST_CREDENTIALS_H

#include <CUnit/Basic.h>

void test_safeword_list_credentials_null_db(void);
void test_safeword_list_credentials_all(void);
void test_safeword_list_credentials_examples(void);
extern CU_TestInfo tests_list_credentials_null[];
extern CU_TestInfo tests_list_credentials_all[];
extern CU_TestInfo tests_list_credentials_examples[];

#endif /* TESTS_SAFEWORD_LIST_CREDENTIALS_H */
