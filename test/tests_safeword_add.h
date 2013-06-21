#ifndef TESTS_SAFEWORD_ADD_H
#define TESTS_SAFEWORD_ADD_H

#include <CUnit/Basic.h>

void test_safeword_add_null_db(void);
void test_safeword_add_username_only(void);
void test_safeword_add_password_only(void);
void test_safeword_add_description_only(void);
void test_safeword_add_all(void);
extern CU_TestInfo tests_add_null[];
extern CU_TestInfo tests_add_usernames[];
extern CU_TestInfo tests_add_passwords[];
extern CU_TestInfo tests_add_descriptions[];
extern CU_TestInfo tests_add_all[];

#endif /* TESTS_SAFEWORD_ADD_H */
