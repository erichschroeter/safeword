#include <safeword.h>

#include "test.h"
#include "tests_safeword_config.h"

void test_safeword_config_get_null_db(void)
{
	struct safeword_db *null_ptr = NULL;
	int ret;
	char *tmp;

	ret = safeword_config_get(null_ptr, NULL, NULL);
	CU_ASSERT(ret == -1);
	ret = safeword_config_get(null_ptr, "copy_once", NULL);
	CU_ASSERT(ret == -1);
	ret = safeword_config_get(null_ptr, NULL, &tmp);
	CU_ASSERT(ret == -1);
	ret = safeword_config_get(null_ptr, "copy_once", &tmp);
	CU_ASSERT(ret == -1);
}

void test_safeword_config_set_null_db(void)
{
	struct safeword_db *null_ptr = NULL;
	int ret;

	/* Try without persisting. */
	ret = safeword_config_set(null_ptr, NULL, NULL, 0);
	CU_ASSERT(ret == -1);
	ret = safeword_config_set(null_ptr, "copy_once", NULL, 0);
	CU_ASSERT(ret == -1);
	ret = safeword_config_set(null_ptr, NULL, "1", 0);
	CU_ASSERT(ret == -1);

	/* Try with persisting. */
	ret = safeword_config_set(null_ptr, NULL, NULL, 1);
	CU_ASSERT(ret == -1);
	ret = safeword_config_set(null_ptr, "copy_once", NULL, 1);
	CU_ASSERT(ret == -1);
	ret = safeword_config_set(null_ptr, NULL, "1", 1);
	CU_ASSERT(ret == -1);
}

void test_safeword_config_get_nonexistent(void)
{
	int ret;
	char *value = NULL;

	ret = safeword_config_get(db1, "this_should_not_exist", &value);
	CU_ASSERT(ret == 0);
	CU_ASSERT(value == NULL);
}

void test_safeword_config_set_nonexistent(void)
{
	int ret;
	char *value = NULL;

	ret = safeword_config_set(db1, "this_should_not_exist", "1", 0);
	CU_ASSERT(ret == 0);

	ret = safeword_config_get(db1, "this_should_not_exist", &value);
	CU_ASSERT(ret == 0);
	CU_ASSERT(value == NULL);
}

CU_TestInfo tests_config_null[] = {
	{ "test_safeword_config_get_null_db", test_safeword_config_get_null_db },
	{ "test_safeword_config_set_null_db", test_safeword_config_set_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_config_examples[] = {
	{ "test_safeword_config_get_nonexistent", test_safeword_config_get_nonexistent },
	{ "test_safeword_config_set_nonexistent", test_safeword_config_set_nonexistent },
	CU_TEST_INFO_NULL,
};
