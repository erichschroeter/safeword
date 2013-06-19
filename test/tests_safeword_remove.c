#include <safeword.h>

#include "test.h"
#include "tests_safeword_remove.h"

void test_safeword_remove_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret, dummy;

	/* In case logic ever changes precedence checking for invalid credential id. */
	ret = safeword_credential_remove(null_ptr, 0);
	CU_ASSERT(ret != 0);

	ret = safeword_credential_remove(null_ptr, 1);
	CU_ASSERT(ret != 0);
}

void test_safeword_remove_username_only(void)
{
	int ret, i;

	for (i = 0; i < EXAMPLES_SIZE; i++) {
		ret = safeword_credential_remove(db1, examples[i].id);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_remove_password_only(void)
{
	int ret, i;

	for (i = 0; i < EXAMPLES_SIZE; i++) {
		ret = safeword_credential_remove(db1, examples[i].id);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_remove_description_only(void)
{
	int ret, i;

	for (i = 0; i < EXAMPLES_SIZE; i++) {
		ret = safeword_credential_remove(db1, examples[i].id);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_remove_all(void)
{
	int ret, i;

	for (i = 0; i < EXAMPLES_SIZE; i++) {
		ret = safeword_credential_remove(db1, examples[i].id);
		CU_ASSERT(ret == 0);
	}
}

CU_TestInfo tests_remove_null[] = {
	{ "test_safeword_remove_null_db",          test_safeword_remove_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_remove_usernames[] = {
	{ "test_safeword_remove_username_only",    test_safeword_remove_username_only },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_remove_passwords[] = {
	{ "test_safeword_remove_password_only",    test_safeword_remove_password_only },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_remove_descriptions[] = {
	{ "test_safeword_remove_description_only", test_safeword_remove_description_only },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_remove_all[] = {
	{ "test_safeword_remove_all",              test_safeword_remove_all },
	CU_TEST_INFO_NULL,
};

static CU_SuiteInfo suites_remove[] = {
	{ "suite_safeword_remove_null_db",                       suite_safeword_init, suite_safeword_clean, tests_remove_null },
	{ "suite_safeword_remove_username_only",                 suite_safeword_init, suite_safeword_clean, tests_remove_usernames },
	{ "suite_safeword_remove_password_only",                 suite_safeword_init, suite_safeword_clean, tests_remove_passwords },
	{ "suite_safeword_remove_description_only",              suite_safeword_init, suite_safeword_clean, tests_remove_descriptions },
	{ "suite_safeword_remove_username_password_description", suite_safeword_init, suite_safeword_clean, tests_remove_all },
	CU_SUITE_INFO_NULL,
};
