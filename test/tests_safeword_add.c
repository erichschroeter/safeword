#include <safeword.h>

#include "test.h"
#include "tests_safeword_add.h"

struct safeword_credential creds[] = {
	{ .username = NULL, .password = NULL, .message = NULL },
	{ .username = "", .password = "", .message = "" },
	{ .username = "testone", .password = "all lowercase", .message = "all lowercase test" },
	{ .username = "TESTTWO", .password = "ALL UPPERCASE", .message = "ALL UPPERCASE TEST" },
	{ .username = "T35Tthr33", .password = "Auv231kkj1kJaka00", .message = "A1ph4 Num3r1c t35t" },
	{ .username = "T{5T _|=0u<!", .password = "50|\\/|e M1xE[) <h4r5", .message = "|\\|0n 4lP4aNu^34!C cH4&5" },
};
const size_t CREDS_SIZE = sizeof(creds) / sizeof(creds[0]);

void test_safeword_add_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret, dummy;

	ret = safeword_credential_add(null_ptr, &dummy, "", "", "");
	CU_ASSERT(ret != 0);
}

void test_safeword_add_username_only(void)
{
	int ret, id, i;

	for (i = 0; i < CREDS_SIZE; i++) {
		ret = safeword_credential_add(db1, &id, creds[i].username, NULL, NULL);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_add_password_only(void)
{
	int ret, id, i;

	for (i = 0; i < CREDS_SIZE; i++) {
		ret = safeword_credential_add(db1, &id, NULL, creds[i].password, NULL);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_add_description_only(void)
{
	int ret, id, i;

	for (i = 0; i < CREDS_SIZE; i++) {
		ret = safeword_credential_add(db1, &id, NULL, NULL, creds[i].message);
		CU_ASSERT(ret == 0);
	}
}

void test_safeword_add_username_password_description(void)
{
	int ret, id, i;

	for (i = 0; i < CREDS_SIZE; i++) {
		ret = safeword_credential_add(db1, &id, creds[i].username, creds[i].password, creds[i].message);
		CU_ASSERT(ret == 0);
	}
}

CU_TestInfo tests_add[] = {
	{ "test_safeword_add_null_db",                       test_safeword_add_null_db },
	{ "test_safeword_add_username_only",                 test_safeword_add_username_only },
	{ "test_safeword_add_password_only",                 test_safeword_add_password_only },
	{ "test_safeword_add_description_only",              test_safeword_add_description_only },
	{ "test_safeword_add_username_password_description", test_safeword_add_username_password_description },
	CU_TEST_INFO_NULL,
};
