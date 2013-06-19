#ifndef TESTS_SAFEWORD_REMOVE_H
#define TESTS_SAFEWORD_REMOVE_H

#include <CUnit/Basic.h>

void test_safeword_remove_null_db(void);
void test_safeword_remove_username_only(void);
void test_safeword_remove_password_only(void);
void test_safeword_remove_description_only(void);
void test_safeword_remove_all(void);
extern CU_TestInfo tests_remove_null[];
extern CU_TestInfo tests_remove_usernames[];
extern CU_TestInfo tests_remove_passwords[];
extern CU_TestInfo tests_remove_descriptions[];
extern CU_TestInfo tests_remove_all[];

#endif /* TESTS_SAFEWORD_REMOVE_H */
