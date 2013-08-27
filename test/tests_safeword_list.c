#include <safeword.h>

#include "test.h"
#include "tests_safeword_list.h"

/* This should be updated if tags are added/removed above. */
#define SAFEWORD_TAG_NUM_SCIENTIST_TAGS 5

void test_safeword_list_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret;

	ret = safeword_tag_list(null_ptr, 0, NULL, 0, NULL);
	CU_ASSERT(ret != 0);
}


void test_safeword_list_tags_all(void)
{
	int i, ret;
	unsigned int tags_size;
	char **tags;

	ret = safeword_tag_list(db1, &tags_size, &tags, 0, 0);
	CU_ASSERT(ret == 0);
	/* NOTE: update if more tags are added to suite init. */
	CU_ASSERT(tags_size == SAFEWORD_TAG_NUM_SCIENTIST_TAGS);

	/* Verify the contents retrieved. */
	for (i = 0; i < SAFEWORD_TAG_NUM_SCIENTIST_TAGS; i++) {
		if (!strcmp(tags[i], "genius") || !strcmp(tags[i], "inventor") ||
		!strcmp(tags[i], "scientist") || !strcmp(tags[i], "teacher") ||
		!strcmp(tags[i], "astrophysicist")) {
			/* tag exists and matches */
		} else {
			CU_FAIL("We got a tage that we shouldn't have.");
		}
	}
}

void test_safeword_list_tags_filter(void)
{
	int i, ret;
	unsigned int tags_size;
	char **tags;
	/*
	 * Filtering the tags around 'scientist' should result in the tags for Einstein and
	 * Niel deGrasse Tyson (assuming 'scientist' is not added to Tesla's tags).
	 */
	const char *filter[] = { "scientist" };

	ret = safeword_tag_list(db1, &tags_size, &tags, 1, filter);
	CU_ASSERT(ret == 0);
	CU_ASSERT(tags_size == 3);

	for (i = 0; i < tags_size; i++) {
		if (!strcmp(tags[i], "genius") || !strcmp(tags[i], "teacher") ||
		!strcmp(tags[i], "astrophysicist")) {
			/* tag exists and matches */
		} else {
			CU_FAIL("We got a tag that we shouldn't have.");
		}
	}
}

CU_TestInfo tests_list_null[] = {
	{ "test_safeword_list_null_db", test_safeword_list_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_list_tags_all[] = {
	{ "test_safeword_list_tags_all", test_safeword_list_tags_all },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_list_tags_filter[] = {
	{ "test_safeword_list_tags_filter", test_safeword_list_tags_filter },
	CU_TEST_INFO_NULL,
};

int suite_safeword_list_init(void)
{
	int i, j, ret;
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

	ret = suite_safeword_init();
	if (ret) return -1;

	for (i = 0; i < SCIENTISTS_SIZE; i++) {
		safeword_credential_add(db1, &scientists[i]);
		for (j = 0; j < scientists[i].tags_size; j++)
			safeword_credential_tag(db1, scientists[i].id, scientists[i].tags[j]);
	}
	return 0;
}
