#ifndef TESTS_SAFEWORD_LIST_H
#define TESTS_SAFEWORD_LIST_H

#include <CUnit/Basic.h>

int suite_safeword_list_init(void);
void test_safeword_list_null_db(void);
void test_safeword_list_tags_all(void);
void test_safeword_list_tags_filter(void);
extern CU_TestInfo tests_list_null[];
extern CU_TestInfo tests_list_tags[];

#endif /* TESTS_SAFEWORD_LIST_H */
