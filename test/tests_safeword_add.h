#ifndef TESTS_SAFEWORD_ADD_H
#define TESTS_SAFEWORD_ADD_H

#include <CUnit/Basic.h>

void test_safeword_add_null_db(void);
void test_safeword_add_username_only(void);
void test_safeword_add_password_only(void);
void test_safeword_add_description_only(void);
void test_safeword_add_username_password_description(void);
extern CU_TestInfo tests_add[];

#endif /* TESTS_SAFEWORD_ADD_H */
