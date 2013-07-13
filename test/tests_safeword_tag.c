#include <safeword.h>

#include "test.h"
#include "tests_safeword_tag.h"

void test_safeword_tag_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	int ret;

	ret = safeword_tag_read(null_ptr, NULL);
	CU_ASSERT(ret == -1);
}

void test_safeword_tag_credential(void)
{
	int i, ret;
	struct safeword_credential *example = safeword_credential_create(
		"myusername", "Exr3meLY Har|) T0 Gu355!", "Example.com account");
	CU_ASSERT_PTR_NOT_NULL(example);

	ret = safeword_credential_add(db1, example);
	CU_ASSERT(ret == 0);

	/* Verify there are no tags for a clean test. */
	ret = safeword_credential_read(db1, example);
	CU_ASSERT(ret == 0);
	CU_ASSERT(example->tags_size == 0);
	CU_ASSERT(example->tags == NULL);

	/* Tag Tesla as genius and inventor. */
	ret = safeword_credential_tag(db1, 1, "www");
	CU_ASSERT(ret == 0);
	ret = safeword_credential_tag(db1, 1, "example.com");
	CU_ASSERT(ret == 0);

	/* Verify it has the tags. */
	ret = safeword_credential_read(db1, example);
	CU_ASSERT(ret == 0);

	for (i = 0; i < example->tags_size; i++) {
		if (!strcmp(example->tags[i], "www") || !strcmp(example->tags[i], "example.com")) {
			/* Should be tagged with ONLY www and example.com. */
		} else {
			CU_FAIL("example is tagged with something it shouldn't be.");
		}
	}
}

CU_TestInfo tests_tag_null[] = {
	{ "test_safeword_tag_null_db", test_safeword_tag_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_tags[] = {
	{ "test_safeword_tag_credential", test_safeword_tag_credential },
	CU_TEST_INFO_NULL,
};

int suite_safeword_tag_init(void)
{
	int i, ret;
	struct safeword_credential scientists[] = {
		{
			.username = "nikola",
			.password = "tesla",
			.description = "Nikola Tesla",
		},
		{
			.username = "albert",
			.password = "einstein",
			.description = "Albert Einstein",
		},
		{
			.username = "neil",
			.password = "tyson",
			.description = "Neil deGrasse Tyson",
		},
	};
	const unsigned int SCIENTISTS_SIZE = sizeof(scientists) / sizeof(scientists[0]);

	ret = suite_safeword_init();
	if (ret) return -1;

	for (i = 0; i < SCIENTISTS_SIZE; i++) {
		safeword_credential_add(db1, &scientists[i]);
	}
	return 0;
}
