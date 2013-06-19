#ifndef TESTS_SAFEWORD_ADD_H
#define TESTS_SAFEWORD_ADD_H

#include <CUnit/Basic.h>

void test_safeword_add_null_db(void);
void test_safeword_add_username_only(void);
void test_safeword_add_password_only(void);
void test_safeword_add_description_only(void);
void test_safeword_add_all(void);
extern CU_TestInfo tests_null[];
extern CU_TestInfo tests_usernames[];
extern CU_TestInfo tests_passwords[];
extern CU_TestInfo tests_descriptions[];
extern CU_TestInfo tests_all[];

#endif /* TESTS_SAFEWORD_ADD_H */
