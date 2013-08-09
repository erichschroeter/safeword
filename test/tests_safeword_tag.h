#ifndef TESTS_SAFEWORD_TAG_H
#define TESTS_SAFEWORD_TAG_H

#include <CUnit/Basic.h>

int suite_safeword_tag_init(void);
void test_safeword_tag_null_db(void);
void test_safeword_tag_credential(void);
extern CU_TestInfo tests_tag_null[];
extern CU_TestInfo tests_tag_credential[];
extern CU_TestInfo tests_tag_filter[];

#endif /* TESTS_SAFEWORD_TAG_H */
