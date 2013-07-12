#include <safeword.h>

#include "test.h"
#include "tests_safeword_read.h"

void test_safeword_read_null_db(void)
{
	struct safeword_db *null_ptr = 0;
	struct safeword_credential credential;
	int ret;

	/* Set to invalid id. */
	credential.id = 0;

	/* In case logic ever changes precedence checking for invalid credential id. */
	ret = safeword_credential_read(null_ptr, &credential);
	CU_ASSERT(ret != 0);

	/* Set to valid id. */
	credential.id = 1;

	ret = safeword_credential_read(null_ptr, &credential);
	CU_ASSERT(ret != 0);
}

void test_safeword_read_invalid_id(void)
{
	int ret;
	struct safeword_credential credential;

	/* Init memory to known state. Also sets id to invalid value. */
	memset(&credential, 0, sizeof(credential));

	ret = safeword_credential_read(db1, &credential);
	CU_ASSERT(ret == 0);

	/* Ensure nothing was modified in credential passed in. */
	CU_ASSERT(credential.id == 0);
	CU_ASSERT(credential.username == 0);
	CU_ASSERT(credential.password == 0);
	CU_ASSERT(credential.description == 0);
	CU_ASSERT(credential.tags == 0);
	CU_ASSERT(credential.tags_size == 0);
}

void test_safeword_read_examples(void)
{
	int i, j, ret;
	struct safeword_credential credential;

	for (i = 0; i < EXAMPLES_SIZE; i++) {
		/* Init memory to known state. Also sets id to invalid value. */
		memset(&credential, 0, sizeof(credential));
		/* NOTE: this assumes the credential IDs match sequentially when input into the database. */
		credential.id = i + 1;

		ret = safeword_credential_read(db1, &credential);
		CU_ASSERT(ret == 0);

		/* Ensure nothing was modified in credential passed in. */
		CU_ASSERT(credential.id == examples[i].id);
		if (!credential.username || !examples[i].username) {
			CU_ASSERT(examples[i].username == NULL);
			CU_ASSERT(credential.username == NULL);
		} else
			CU_ASSERT_STRING_EQUAL(credential.username, examples[i].username);
		if (!credential.password || !examples[i].password) {
			CU_ASSERT(examples[i].password == NULL);
			CU_ASSERT(credential.password == NULL);
		} else
			CU_ASSERT_STRING_EQUAL(credential.password, examples[i].password);
		if (!credential.description || !examples[i].description) {
			CU_ASSERT(examples[i].description == NULL);
			CU_ASSERT(credential.description == NULL);
		} else
			CU_ASSERT_STRING_EQUAL(credential.description, examples[i].description);
		CU_ASSERT(credential.tags_size == examples[i].tags_size);
		for (j = 0; j < examples[i].tags_size; j++) {
			CU_ASSERT_STRING_EQUAL(credential.tags[j], examples[i].tags[j]);
		}
	}
}

CU_TestInfo tests_read_null[] = {
	{ "test_safeword_read_null_db", test_safeword_read_null_db },
	CU_TEST_INFO_NULL,
};
CU_TestInfo tests_read_examples[] = {
	{ "test_safeword_read_invalid_id", test_safeword_read_invalid_id },
	{ "test_safeword_read_examples", test_safeword_read_examples },
	CU_TEST_INFO_NULL,
};
