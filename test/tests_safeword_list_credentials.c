#include <limits.h>
#include <safeword.h>

#include "test.h"
#include "tests_safeword_list_credentials.h"

static int suite_safeword_list_credentials_init(void)
{
	int i, j;
	/*
	 * NOTE: changing or adding tags will most likely require the unit tests in
	 * this file to be updated.
	 */
	static char *tesla_tags[] = {
		"genius",
		"inventor",
	};
	static char *einstein_tags[] = {
		"genius",
		"scientist",
	};
	static char *tyson_tags[] = {
		"genius",
		"teacher",
		"scientist",
		"astrophysicist",
	};
	static struct safeword_credential scientists[] = {
		{
			.username = "nikola",
			.password = "tesla",
			.description = "Nikola Tesla",
			.tags_size = 2,
			.tags = tesla_tags,
		},
		{
			.username = "albert",
			.password = "einstein",
			.description = "Albert Einstein",
			.tags_size = 2,
			.tags = einstein_tags,
		},
		{
			.username = "neil",
			.password = "tyson",
			.description = "Neil deGrasse Tyson",
			.tags_size = 4,
			.tags = tyson_tags,
		},
	};
	const unsigned int SCIENTISTS_SIZE = sizeof(scientists) / sizeof(scientists[0]);

	for (i = 0; i < SCIENTISTS_SIZE; i++) {
		safeword_credential_add(db1, &scientists[i]);
		for (j = 0; j < scientists[i].tags_size; j++)
			safeword_credential_tag(db1, scientists[i].id, scientists[i].tags[j]);
	}
	return 0;
}

void test_safeword_list_credentials_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret;
	char **tmp = NULL;
	unsigned int creds_size;
	struct safeword_credential **creds;

	ret = safeword_credential_list(null_ptr, 0, NULL, 0, NULL);
	CU_ASSERT(ret == -1);
	ret = safeword_credential_list(null_ptr, 0, tmp, 0, NULL);
	CU_ASSERT(ret == -1);
	ret = safeword_credential_list(null_ptr, 0, tmp, &creds_size, NULL);
	CU_ASSERT(ret == -1);
	ret = safeword_credential_list(null_ptr, 0, tmp, &creds_size, &creds);
	CU_ASSERT(ret == -1);
}


void test_safeword_list_credentials_all(void)
{
	int i, ret;
	unsigned int creds_size;
	struct safeword_credential **creds;

	ret = suite_safeword_list_credentials_init();
	CU_ASSERT(ret == 0);

	ret = safeword_credential_list(db1, UINT_MAX, NULL, &creds_size, &creds);
	CU_ASSERT(ret == 0);
	CU_ASSERT(creds_size == 3);

	/* Verify the contents retrieved. */
	for (i = 0; i < creds_size; i++) {
		if (!strcmp(creds[i]->description, "Nikola Tesla") ||
		!strcmp(creds[i]->description, "Albert Einstein") ||
		!strcmp(creds[i]->description, "Neil deGrasse Tyson")) {
			/* credential was returned */
		} else {
			CU_FAIL("We got a credential that we shouldn't have.");
		}
	}
}

void test_safeword_list_credentials_examples(void)
{
	int i, ret;
	unsigned int creds_size;
	struct safeword_credential **creds;
	/*
	 * Filtering the credentials around 'scientist' should result in Einstein and
	 * Niel deGrasse Tyson (assuming 'scientist' is not added to Tesla's tags).
	 */
	char *tags[] = { "scientist" };
	unsigned int tags_size = 1;

	ret = suite_safeword_list_credentials_init();
	CU_ASSERT(ret == 0);

	ret = safeword_credential_list(db1, tags_size, tags, &creds_size, &creds);
	CU_ASSERT(ret == 0);
	CU_ASSERT(creds_size == 2);

	for (i = 0; i < creds_size; i++) {
		if (!strcmp(creds[i]->description, "Albert Einstein") ||
		!strcmp(creds[i]->description, "Neil deGrasse Tyson")) {
			/* credential was returned */
		} else {
			CU_FAIL("We got a credential that we shouldn't have.");
		}
	}
}

CU_TestInfo tests_list_credentials_null[] = {
	{ "test_safeword_list_credentials_null_db", test_safeword_list_credentials_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_list_credentials_all[] = {
	{ "test_safeword_list_credentials_all", test_safeword_list_credentials_all },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_list_credentials_examples[] = {
	{ "test_safeword_list_credentials_examples", test_safeword_list_credentials_examples },
	CU_TEST_INFO_NULL,
};
