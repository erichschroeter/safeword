#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <getopt.h>
#include <limits.h>

#include <safeword.h>
#include <safeword_errno.h>
#include "ExportCommand.h"
#include "dbg.h"

struct export_data {
	unsigned int ids_size;
	long int *ids;
	unsigned int tags_size;
	char **tags;
};

char* exportCmd_help(void)
{
	return NULL;
}

static struct export_data *pdata = NULL;
static int _all = 0;

int exportCmd_parse(int argc, char** argv)
{
	int i, c, backup, arrays_counted = 0, id_idx = 0, tags_idx = 0;
	struct option long_options[] = {
		{"all", no_argument, NULL, 'a'},
	};

	while ((c = getopt_long(argc, argv, "a", long_options, 0)) != -1) {
		switch (c) {
		case 'a':
			_all = 1;
			break;
		default:
			break;
		}
	}

	pdata = calloc(1, sizeof(*pdata));
	if (pdata == NULL) goto fail;

	/*
	 * Keep a backup of the optind so we can go back to it
	 * after finding number of ids and tags
	 */
	backup = optind;

parse_tags_and_ids:
	while (optind < argc) {
		long int id = strtol(argv[optind], NULL, 10);
		if (id == 0) {
			/* It's a tag */
			if (arrays_counted) {
				pdata->tags[tags_idx] = calloc(strlen(argv[optind]) + 1, sizeof(char));
				if (pdata->tags[tags_idx] == NULL) goto fail_tag_copy;
				strcpy(pdata->tags[tags_idx], argv[optind]);
				tags_idx++;
			} else {
				pdata->tags_size++;
			}
		} else if (id == LONG_MIN || id == LONG_MAX) {
			perror("strtol");
			goto fail;
		} else {
			/* It's an id */
			if (arrays_counted) {
				pdata->ids[id_idx] = id;
				id_idx++;
			} else {
				pdata->ids_size++;
			}
		}
		optind++;
	}

	if (!arrays_counted) {
		/* Resotre the original optind as we go back and copy the data. */
		optind = backup;
		pdata->ids = calloc(pdata->ids_size, sizeof(long int));
		if (pdata->ids == NULL) goto fail_ids;
		pdata->tags = calloc(pdata->tags_size, sizeof(char*));
		if (pdata->tags == NULL) goto fail_tags;
		arrays_counted = 1;
		goto parse_tags_and_ids;
	}

	return 0;
fail_tag_copy:
	for (i = tags_idx; i > 0; i--) {
		free(pdata->tags[i]);
	}
fail_tags:
	free(pdata->ids);
fail_ids:
	free(pdata);
fail:
	return -1;
}

int exportCmd_run(int argc, char** argv)
{
	int ret, i;
	unsigned int creds_size = 0;
	struct safeword_credential **creds, *cred;
	struct safeword_db db;
	char *json;

	ret = exportCmd_parse(argc, argv);
	if (ret != 0) return ret;

	/* If _all is not specified, make sure we have ids or tags to export. */
	if (!_all && (pdata == NULL ||
		(pdata->ids_size < 1 && pdata->tags_size < 1))) return 0;

	ret = safeword_open(&db, 0);
	safeword_check(ret == 0, ret, fail);

	cred = safeword_credential_create(NULL, NULL, NULL, NULL);
	if (cred == NULL) goto fail;

	if (_all) {
		ret = safeword_credential_list(&db, UINT_MAX, NULL, &creds_size, &creds);
		safeword_check(ret == 0, ret, fail);
	} else {
		for (i = 0; i < pdata->ids_size; i++) {
			cred->id = pdata->ids[i];
			ret = safeword_credential_read(&db, cred);
			safeword_check(ret == 0, ret, fail);

			json = safeword_credential_tostring(cred);
			printf("%s\n", json);
			free(json);
		}

		ret = safeword_credential_list(&db, pdata->tags_size, pdata->tags, &creds_size, &creds);
		safeword_check(ret == 0, ret, fail);
	}

	for (i = 0; i < creds_size; i++) {
		cred->id = creds[i]->id;
		ret = safeword_credential_read(&db, cred);
		safeword_check(ret == 0, ret, fail);

		json = safeword_credential_tostring(cred);
		printf("%s\n", json);
		free(json);
	}

	free(cred);

	safeword_close(&db);

	return 0;
fail:

	return -1;
}
