#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <safeword.h>

#include "test.h"

const char db1_path[] = "db1.safeword";
struct safeword_db *db1;

#include "tests_safeword_init.h"
#include "tests_safeword_add.h"

int suite_safeword_init(void)
{
	int ret;

	ret = safeword_init(db1_path);
	if (ret != 0)
		return -1;

	db1 = calloc(1, sizeof(*db1));
	if (!db1)
		return -1;

	ret = safeword_open(db1, db1_path);
	if (ret != 0)
		return -1;

	return 0;
}

int suite_safeword_clean(void)
{
	int ret;

	ret = safeword_close(db1);
	if (ret != 0)
		return -1;

	ret = remove(db1_path);
	if (ret != 0)
		return -1;

	return 0;
}

static CU_SuiteInfo suites[] = {
	{ "suite_init",                                       NULL,                NULL,                 tests_init },
	{ "suite_safeword_add_null_db",                       suite_safeword_init, suite_safeword_clean, tests_null },
	{ "suite_safeword_add_username_only",                 suite_safeword_init, suite_safeword_clean, tests_usernames },
	{ "suite_safeword_add_password_only",                 suite_safeword_init, suite_safeword_clean, tests_passwords },
	{ "suite_safeword_add_description_only",              suite_safeword_init, suite_safeword_clean, tests_descriptions },
	{ "suite_safeword_add_username_password_description", suite_safeword_init, suite_safeword_clean, tests_all },
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
