#include <stdlib.h>

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
		"myusername", "Exr3meLY Har|) T0 Gu355!", "Example.com account", NULL);
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

void test_safeword_tag_filter(void)
{
	int i, ret;
	unsigned int tags_size;
	char **tags, **filter;
	char *personal_email_tags[] = { "email", "gmail", "google" };
	struct safeword_credential personal_email = {
		.username = "mypersonalemail@gmail.com",
		.password = "5Up3r S3cret!",
		.description = "Main email account",
		.tags_size = 3,
		.tags = personal_email_tags,
	};
	char *junk_email_tags[] = { "email", "hotmail" };
	struct safeword_credential junk_email = {
		.username = "throwaway@hotmail.com",
		.password = "password",
		.description = "Throw away email account",
		.tags_size = 2,
		.tags = junk_email_tags,
	};
	char *work_email_tags[] = { "email", "acme", "work" };
	struct safeword_credential work_email = {
		.username = "myname@acme.com",
		.password = "12jjsksiwkw4411",
		.description = "Work email account",
		.tags_size = 3,
		.tags = work_email_tags,
	};

	ret = safeword_credential_add(db1, &personal_email);
	CU_ASSERT(ret == 0);
	for (i = 0; i < personal_email.tags_size; i++) {
		ret = safeword_credential_tag(db1, 1, personal_email.tags[i]);
		CU_ASSERT(ret == 0);
	}

	ret = safeword_credential_add(db1, &junk_email);
	CU_ASSERT(ret == 0);
	for (i = 0; i < junk_email.tags_size; i++) {
		ret = safeword_credential_tag(db1, 2, junk_email.tags[i]);
		CU_ASSERT(ret == 0);
	}

	ret = safeword_credential_add(db1, &work_email);
	CU_ASSERT(ret == 0);
	for (i = 0; i < work_email.tags_size; i++) {
		ret = safeword_credential_tag(db1, 3, work_email.tags[i]);
		CU_ASSERT(ret == 0);
	}

	filter = calloc(1, sizeof(char*));
	CU_ASSERT_PTR_NOT_NULL(filter);
	filter[0] = "email";

	/* Verify that the tags are filtered correctly. */
	ret = safeword_tag_list(db1, &tags_size, &tags, 1, (const char**) filter);
	CU_ASSERT(ret == 0);
	CU_ASSERT(tags_size == 5);
	for (i = 0; i < tags_size; i++) {
		if (!strcmp(tags[i], "gmail") || !strcmp(tags[i], "google") ||
			!strcmp(tags[i], "hotmail") || !strcmp(tags[i], "acme") ||
			!strcmp(tags[i], "work")) {
			/* Should only be taged with these tags. */
		} else {
			CU_FAIL("Tag should not have been returned.");
		}
	}
}

CU_TestInfo tests_tag_null[] = {
	{ "test_safeword_tag_null_db", test_safeword_tag_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_tag_credential[] = {
	{ "test_safeword_tag_credential", test_safeword_tag_credential },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_tag_filter[] = {
	{ "test_safeword_tag_filter", test_safeword_tag_filter },
	CU_TEST_INFO_NULL,
};
