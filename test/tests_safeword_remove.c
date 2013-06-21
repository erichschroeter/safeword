#include <safeword.h>

#include "test.h"
#include "tests_safeword_remove.h"

void test_safeword_remove_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret;

	/* In case logic ever changes precedence checking for invalid credential id. */
	ret = safeword_credential_remove(null_ptr, 0);
	CU_ASSERT(ret != 0);

	ret = safeword_credential_remove(null_ptr, 1);
	CU_ASSERT(ret != 0);
}

void test_safeword_remove_one(void)
{
	int ret, id1 = 1, id2 = 2;

	/* Verify two known credentials exist. */
	ret = safeword_credential_exists(db1, id1);
	CU_ASSERT_EQUAL(ret, 1);
	ret = safeword_credential_exists(db1, id2);
	CU_ASSERT_EQUAL(ret, 1);

	/* Delete one of the existing credentials. */
	ret = safeword_credential_remove(db1, id1);
	CU_ASSERT(ret == 0);

	/* Verify deleted credential no longer exists. */
	ret = safeword_credential_exists(db1, id1);
	CU_ASSERT_EQUAL(ret, 0);

	/* Verify non-deleted credential still exists. */
	ret = safeword_credential_exists(db1, id2);
	CU_ASSERT_EQUAL(ret, 1);
}

CU_TestInfo tests_remove_null[] = {
	{ "test_safeword_remove_null_db", test_safeword_remove_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_remove_one[] = {
	{ "test_safeword_remove_one", test_safeword_remove_one },
	CU_TEST_INFO_NULL,
};
