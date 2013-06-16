#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <safeword.h>

#include "tests_safeword_init.h"

struct safeword_db *db1;

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
