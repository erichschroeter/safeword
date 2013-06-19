#include <stdlib.h>
#include <unistd.h>

#include <CUnit/Basic.h>

#include <safeword.h>

#include "test.h"

const char db1_path[] = "db1.safeword";
struct safeword_db *db1;
struct safeword_credential examples[] = {
	{
		.username = "nikolatesla@gmail.com",
		.password = "My super secret password!!",
		.description = "Nikola Tesla's Gmail",
		.tags = {
			"email",
			"gmail",
		},
	},
	{ .username = "mariecurie@outlook.com", .password = "R4|)i04ctiV3", .description = "Marie Curie's Outlook" },
	{ .username = "einstein", .password = "albert", .description = "Home desktop login" },
	{ .username = "Administrator", .password = "hws772k0ajjvKKU11jLl23", .description = "Work administrator account" },
	{ .username = "testaccount", .password = "test", .description = "Test account for Blarg 1.0" },
	{ .username = "root", .password = "", .description = "The most secure root password" },
	{ .username = NULL, .password = "1234", .description = "Bank PIN" },
};
const unsigned int EXAMPLES_SIZE = sizeof(examples) / sizeof(examples[0]);

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

int suite_safeword_examples(void)
{
	int ret, id, i;

	ret = suite_safeword_init();
	if (ret)
		return -1;

	for (i = 0; i < EXAMPLES_SIZE; i++) {
		safeword_credential_add(db1, &id, examples[i].username, examples[i].password, examples[i].description);
		examples[i].id = id;
	}

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
