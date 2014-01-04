#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <safeword.h>

#include "tests_safeword_init.h"

void test_safeword_no_overwrite(void)
{
	const char path[] = "no_overwrite.safeword";
	int ret;

	/* Verify file does not exist. */
	CU_ASSERT(access(path, F_OK) == -1);

	/* Create the safeword database. */
	ret = safeword_init(path);
	CU_ASSERT(ret == 0);

	/* Verify file now exists. */
	CU_ASSERT(access(path, F_OK) == 0);

	/* Verify that we get correct error return value. */
	ret = safeword_init(path);
	CU_ASSERT(ret == -1);
	CU_ASSERT(safeword_errno == ESAFEWORD_DBEXIST);

	/* Remove the safeword database created. */
	ret = remove(path);
	CU_ASSERT(ret == 0);
}

CU_TestInfo tests_init[] = {
	{ "test_safeword_no_overwrite", test_safeword_no_overwrite },
	CU_TEST_INFO_NULL,
};
