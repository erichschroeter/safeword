#include <safeword.h>

#include "test.h"
#include "tests_safeword_list.h"

/*
 * NOTE: changing or adding tags will most likely require the unit tests in
 * this file to be updated.
 */
char *tesla_tags[] = {
	"genius",
	"inventor",
};
char *einstein_tags[] = {
	"genius",
	"scientist",
};
char *tyson_tags[] = {
	"genius",
	"teacher",
	"scientist",
	"astrophysicist",
};
/* This should be updated if tags are added/removed above. */
const unsigned int NUM_SCIENTIST_TAGS = 5;
struct safeword_credential scientists[] = {
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

void test_safeword_list_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret;

	ret = safeword_list_tags(null_ptr, 0, NULL, 0, NULL);
	CU_ASSERT(ret != 0);
}

static void setup_list_test(void)
{
	int i, j;

	for (i = 0; i < SCIENTISTS_SIZE; i++) {
		safeword_credential_add(db1, &scientists[i]);
		for (j = 0; j < scientists[i].tags_size; j++)
			safeword_credential_tag(db1, scientists[i].id, scientists[i].tags[j]);
	}
}

void test_safeword_list_tags_all(void)
{
	int i, ret;
	unsigned int tags_size;
	char **tags;

	setup_list_test();

	ret = safeword_list_tags(db1, &tags_size, &tags, 0, 0);
	CU_ASSERT(ret == 0);
	CU_ASSERT(tags_size == NUM_SCIENTIST_TAGS);

	/* Verify the contents retrieved. */
	for (i = 0; i < NUM_SCIENTIST_TAGS; i++) {
		if (!strcmp(tags[i], "genius") || !strcmp(tags[i], "inventor") ||
		!strcmp(tags[i], "scientist") || !strcmp(tags[i], "teacher") ||
		!strcmp(tags[i], "astrophysicist")) {
			/* tag exists and matches */
		} else {
			CU_FAIL("Tag(s) do not match.");
		}
	}
}

void test_safeword_list_tags_filter(void)
{
	setup_list_test();
}

CU_TestInfo tests_list_null[] = {
	{ "test_safeword_list_null_db", test_safeword_list_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_list_tags[] = {
	{ "test_safeword_list_tags_all", test_safeword_list_tags_all },
	{ "test_safeword_list_tags_filter", test_safeword_list_tags_filter },
	CU_TEST_INFO_NULL,
};
