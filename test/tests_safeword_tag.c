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
	struct safeword_credential tesla;

	memset(&tesla, 0, sizeof(tesla));

	/* Verify there are no tags for a clean test. */
	ret = safeword_credential_read(db1, &tesla);
	CU_ASSERT(ret == 0);
	CU_ASSERT(tesla.tags_size == 0);
	CU_ASSERT(tesla.tags == NULL);

	/* Tag Tesla as genius and inventor. */
	ret = safeword_credential_tag(db1, 1, "genius");
	CU_ASSERT(ret == 0);
	ret = safeword_credential_tag(db1, 1, "inventor");
	CU_ASSERT(ret == 0);

	/* Verify he has the tags. */
	ret = safeword_credential_read(db1, &tesla);
	CU_ASSERT(ret == 0);

	for (i = 0; i < tesla.tags_size; i++) {
		if (!strcmp(tesla.tags[i], "genius") || !strcmp(tesla.tags[i], "inventor")) {
			/* Tesla should be tagged with ONLY genius and inventor. */
		} else {
			CU_FAIL("Tesla is tagged with something he shouldn't be.");
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
