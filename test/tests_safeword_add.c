#include <safeword.h>

#include "test.h"
#include "tests_safeword_add.h"

struct safeword_credential creds[] = {
	{ .username = NULL, .password = NULL, .description = NULL },
	{ .username = "", .password = "", .description = "" },
	{ .username = "testone", .password = "all lowercase", .description = "all lowercase test" },
	{ .username = "TESTTWO", .password = "ALL UPPERCASE", .description = "ALL UPPERCASE TEST" },
	{ .username = "T35Tthr33", .password = "Auv231kkj1kJaka00", .description = "A1ph4 Num3r1c t35t" },
	{ .username = "T{5T _|=0u<!", .password = "50|\\/|e M1xE[) <h4r5", .description = "|\\|0n 4lP4aNu^34!C cH4&5" },
};
const size_t CREDS_SIZE = sizeof(creds) / sizeof(creds[0]);

void test_safeword_add_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret;

	ret = safeword_credential_add(null_ptr, NULL);
	CU_ASSERT(ret != 0);
}

void test_safeword_add_username_only(void)
{
	int ret, i;
	struct safeword_credential validate;

	for (i = 0; i < CREDS_SIZE; i++) {
		memset(&validate, 0, sizeof(validate));

		ret = safeword_credential_add(db1, &creds[i]);
		CU_ASSERT(ret == 0);

		validate.id = creds[i].id;
		/* Read the credential just added and verify it is the same as expected. */
		ret = safeword_credential_read(db1, &validate);
		CU_ASSERT(ret == 0);

		/* Special check for NULL since CU_ASSERT_STRING_EQUAL segfaults if either is NULL. */
		if (validate.username && creds[i].username) {
			CU_ASSERT_STRING_EQUAL(validate.username, creds[i].username);
		} else {
			CU_ASSERT_PTR_NULL(validate.username);
			CU_ASSERT_PTR_NULL(creds[i].username);
		}

		ret = safeword_credential_free(&validate);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_add_password_only(void)
{
	int ret, i;
	struct safeword_credential validate;

	for (i = 0; i < CREDS_SIZE; i++) {
		memset(&validate, 0, sizeof(validate));

		ret = safeword_credential_add(db1, &creds[i]);
		CU_ASSERT(ret == 0);

		validate.id = creds[i].id;
		/* Read the credential just added and verify it is the same as expected. */
		ret = safeword_credential_read(db1, &validate);
		CU_ASSERT(ret == 0);

		/* Special check for NULL since CU_ASSERT_STRING_EQUAL segfaults if either is NULL. */
		if (validate.password && creds[i].password) {
			CU_ASSERT_STRING_EQUAL(validate.password, creds[i].password);
		} else {
			CU_ASSERT_PTR_NULL(validate.password);
			CU_ASSERT_PTR_NULL(creds[i].password);
		}

		ret = safeword_credential_free(&validate);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_add_description_only(void)
{
	int ret, i;
	struct safeword_credential validate;

	for (i = 0; i < CREDS_SIZE; i++) {
		memset(&validate, 0, sizeof(validate));

		ret = safeword_credential_add(db1, &creds[i]);
		CU_ASSERT(ret == 0);

		validate.id = creds[i].id;
		/* Read the credential just added and verify it is the same as expected. */
		ret = safeword_credential_read(db1, &validate);
		CU_ASSERT(ret == 0);

		/* Special check for NULL since CU_ASSERT_STRING_EQUAL segfaults if either is NULL. */
		if (validate.description && creds[i].description) {
			CU_ASSERT_STRING_EQUAL(validate.description, creds[i].description);
		} else {
			CU_ASSERT_PTR_NULL(validate.description);
			CU_ASSERT_PTR_NULL(creds[i].description);
		}

		ret = safeword_credential_free(&validate);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_add_all(void)
{
	int ret, i;

	for (i = 0; i < CREDS_SIZE; i++) {
		ret = safeword_credential_add(db1, &creds[i]);
		CU_ASSERT(ret == 0);
	}
}

CU_TestInfo tests_add_null[] = {
	{ "test_safeword_add_null_db",          test_safeword_add_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_add_usernames[] = {
	{ "test_safeword_add_username_only",    test_safeword_add_username_only },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_add_passwords[] = {
	{ "test_safeword_add_password_only",    test_safeword_add_password_only },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_add_descriptions[] = {
	{ "test_safeword_add_description_only", test_safeword_add_description_only },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_add_all[] = {
	{ "test_safeword_add_all",              test_safeword_add_all },
	CU_TEST_INFO_NULL,
};
