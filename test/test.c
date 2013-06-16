#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <safeword.h>

static struct safeword_db *db1;

void test_safeword_no_overwrite(void)
{
	const char path[] = "no_overwrite.safeword";
	int ret;

	/* Verify file does not exist. */
	CU_ASSERT(access(path, F_OK) == -1);

	/* Create the safeword database. */
	ret = safeword_init(path);
	CU_ASSERT(ret == 0);

	/* Verify file now exists. */
	CU_ASSERT(access(path, F_OK) == 0);

	/* Verify that we get correct error return value. */
	ret = safeword_init(path);
	CU_ASSERT(ret == -ESAFEWORD_DBEXIST);

	/* Remove the safeword database created. */
	ret = remove(path);
	CU_ASSERT(ret == 0);
}

static CU_TestInfo tests_init[] = {
	{ "test_safeword_no_overwrite", test_safeword_no_overwrite },
	CU_TEST_INFO_NULL,
};

static int suite_safeword_init(void)
{
	return 0;
}

static int suite_safeword_clean(void)
{
	return 0;
}

static CU_SuiteInfo suites[] = {
	{ "suite_init",  suite_safeword_init, suite_safeword_clean, tests_init },
	CU_SUITE_INFO_NULL,
};

int main(int argc, char **argv)
{
	if (CUE_SUCCESS != CU_initialize_registry())
		return CU_get_error();

	if (CU_register_suites(suites) != CUE_SUCCESS) {
		fprintf(stderr, "suite registration failed: %s\n", CU_get_error_msg());
		exit(EXIT_FAILURE);
	}

	CU_basic_set_mode(CU_BRM_VERBOSE);
	/*CU_set_error_action(CUEA_IGNORE);*/
	CU_basic_run_tests();

fail:
	CU_cleanup_registry();
	return CU_get_error();
}
