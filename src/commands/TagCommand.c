#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <safeword.h>
#include <safeword_errno.h>
#include "TagCommand.h"

#define MAXBUFFERSIZE 16

static int _force = 0;
static int _untag = 0;
static FILE *_wiki_file;

struct array {
	unsigned int size;
	char **data;
};

struct tag_subcommand {
	int	(*execute)(struct safeword_db *db, void *data);
};
static struct tag_subcommand _subcommand;

struct update_info {
	char *tag;
	FILE *file;
};

static int *_credential_ids;
static int _credential_ids_size;
static struct array *_tags;

char* tagCmd_help(void)
{
	return "SYNOPSIS\n"
"	tag [-d | --delete] [-f | --force] [-m | --move] [-w | --wiki] [ID] TAGS ...\n"
"\n"
"DESCRIPTION\n"
"	This command serves multiple purposes dealing with tags within the safeword database. Without any\n"
"	arguments it lists all tags within the database. If the first argument is a credential ID(s) the\n"
"	remaining arguments will be mapped to the credential(s). To specify more than one credential ID can\n"
"	be a comma separated list.\n"
"\n"
"OPTIONS\n"
"	-d, --delete\n"
"	    Delete tag(s) from the safeword database. If any credentials are\n"
"	    mapped the --force option must be used.\n"
"	-f, --force\n"
"	    Used with the --delete option to force a mapped tag to be removed.\n"
"	-m, --move OLD NEW\n"
"	    Move/rename an existing tag.\n"
"	-w, --wiki FILE\n"
"	    Specify markdown wiki information about the tag. If FILE is '-' then\n"
"	    input is read from stdin.\n"
"	-u, --untag\n"
"	    Untag the specified tags from the specified credentials.\n"
"\n";
}

static int delete_tags(struct safeword_db* db, void *tags_array)
{
	int i, char_count;
	char input, input_buffer[MAXBUFFERSIZE];
	struct array *tags = (struct array*) tags_array;

	if (!tags) {
		fprintf(stderr, "no tags specified\n");
		return -ESAFEWORD_INVARG;
	}

	for (i = 0; i < tags->size; i++) {
		if (_force) {
			safeword_tag_delete(db, tags->data[i]);
			continue;
		}
		printf("delete tag '%s'? (y/N) ", tags->data[i]);
		char_count = 0;
		input = getchar();
		while ((input != '\n') && (char_count < MAXBUFFERSIZE)) {
			input_buffer[char_count++] = input;
			input = getchar();
		}
		input_buffer[char_count] = 0x00;

		/* skip comparing if just enter was pressed */
		if (!char_count)
			continue;

		if (!strncasecmp("yes", input_buffer, char_count)) {
			safeword_tag_delete(db, tags->data[i]);
		} else if (!strncasecmp("quit", input_buffer, char_count) ||
			!strncasecmp("q", input_buffer, char_count))
			break;
	}

	return 0;
}

static int rename_tag(struct safeword_db* db, void *tags_array)
{
	struct array *tags = (struct array*) tags_array;

	if (!tags) {
		fprintf(stderr, "no tags specified\n");
		return -ESAFEWORD_INVARG;
	}

	if (tags->size < 2) {
		fprintf(stderr, "%s\n", tags->size ? "no new tag name specified" : "no tags specified");
		return -ESAFEWORD_INVARG;
	}

	return safeword_tag_rename(db, tags->data[0], tags->data[1]);
}

static int update_tag(struct safeword_db* db, void *update_info)
{
	int ret = 0;
	struct update_info *info = (struct update_info*) update_info;
	long wiki_size;
	char *wiki;

	if (!info || !info->tag) {
		fprintf(stderr, "no tags specified\n");
		return -ESAFEWORD_INVARG;
	}

	if (!info->file) {
		fprintf(stderr, "no wiki file specified\n");
		return -ESAFEWORD_INVARG;
	}

	if (info->file != stdin) {
		ret = fseek(info->file, 0, SEEK_END);
		safeword_check(ret != -1, -ESAFEWORD_IO, fail);

		wiki_size = ftell(info->file);
		safeword_check(wiki_size != -1, -ESAFEWORD_IO, fail);

		ret = fseek(info->file, 0, SEEK_SET);
		safeword_check(ret != -1, -ESAFEWORD_IO, fail);

		wiki = calloc(wiki_size + 1, sizeof(char));
		safeword_check(wiki, -ENOMEM, fail);

		ret = fread(wiki, 1, wiki_size, info->file);
		safeword_check(ret == wiki_size, -ESAFEWORD_IO, fail);
	} else {
		char c;
		char buffer[1024];
		buffer[0] = '\0';
		wiki_size = 0;

		wiki = calloc(1024, sizeof(char));
		safeword_check(wiki, -ENOMEM, fail);

		while (fgets(buffer, 1024, info->file)) {
			char *old = wiki;
			wiki_size += strlen(buffer);
			wiki = realloc(wiki, wiki_size + 1);
			if (!wiki) {
				ret = -ENOMEM;
				free(old);
				goto fail;
			}
			strcat(wiki, buffer);
		}
	}

	safeword_tag_update(db, info->tag, wiki);

fail:
	free(wiki);
	return ret;
}

int tagCmd_parse(int argc, char** argv)
{
	int ret = 0, remaining_args = 0, i;
	struct option long_options[] = {
		{"delete", no_argument,       NULL, 'd'},
		{"force",  no_argument,       NULL, 'f'},
		{"move",   no_argument,       NULL, 'm'},
		{"wiki",   required_argument, NULL, 'w'},
		{"untag",  no_argument,       NULL, 'u'},
	};

	_subcommand.execute = NULL;

	while ((i = getopt_long(argc, argv, "w:dfmu", long_options, 0)) != -1) {
		switch (i) {
		case 'd':
			safeword_check(!_subcommand.execute, -ESAFEWORD_STATE, fail);
			_subcommand.execute = &delete_tags;
			break;
		case 'f':
			_force = 1;
			break;
		case 'm':
			safeword_check(!_subcommand.execute, -ESAFEWORD_STATE, fail);
			_subcommand.execute = &rename_tag;
			break;
		case 'w':
			if (!strncmp(optarg, "-", 1)) {
				_wiki_file = stdin;
			} else {
				_wiki_file = fopen(optarg, "r");
				safeword_check(_wiki_file, -ESAFEWORD_IO, fail);
			}
			_subcommand.execute = &update_tag;
			break;
		case 'u':
			_untag = 1;
			break;
		}
	}

	remaining_args = argc - optind;

	/* START of parsing credential ids */
	if (!_subcommand.execute && remaining_args > 0) {
		int i = 0, id, tags_size = 0;
		char *id_str, *ids_backup, **tags;

		ids_backup = calloc(strlen(argv[optind]) + 1, sizeof(char));
		safeword_check(ids_backup, -ENOMEM, fail);

		tags = malloc(remaining_args * sizeof(*tags));
		safeword_check(tags, -ENOMEM, fail);

		strcpy(ids_backup, argv[optind]);

		/* get number of credential ids in string */
		id_str = strtok(ids_backup, ",");
		while (id_str != NULL) {
			id_str = strtok(NULL, ",");
			_credential_ids_size++;
		}

		_credential_ids = calloc(_credential_ids_size, sizeof(*_credential_ids));
		safeword_check(_credential_ids, -ENOMEM, fail);

		ids_backup = argv[optind];
		id_str = strtok(ids_backup, ",");
		while (id_str != NULL) {
			id = atoi(id_str);
			_credential_ids[i] = id;
			id_str = strtok(NULL, ",");
			i++;
		}
		optind++;
		remaining_args--;
	}
	/* END of parsing credential ids */

	/* START of parsing tags */
	_tags = malloc(sizeof(*_tags));
	safeword_check(_tags, -ENOMEM, fail);

	_tags->size = 0;
	_tags->data = malloc(remaining_args * sizeof(*_tags->data));
	safeword_check(_tags->data, -ENOMEM, fail);

	/* copy tags to the tags array */
	for (i = 0; i < remaining_args; i++) {
		_tags->data[i] = calloc(strlen(argv[optind]) + 1, sizeof(char));
		safeword_check(_tags->data[i], -ENOMEM, fail);

		strcpy(_tags->data[i], argv[optind]);
		_tags->size++;
		optind++;
	}
	/* END of parsing tags */

fail:
	return ret;
}

int tagCmd_execute(void)
{
	int ret = 0, i, j;
	struct safeword_db db;
	char *sql;

	ret = safeword_db_open(&db, 0);
	safeword_check(!ret, ret, fail);

	if (_subcommand.execute == &delete_tags ||
		_subcommand.execute == &rename_tag) {
		_subcommand.execute(&db, _tags);
	} else if (_subcommand.execute == &update_tag) {
		struct update_info info;
		info.tag = _tags->data[0];
		info.file = _wiki_file;
		_subcommand.execute(&db, &info);
	} else if (_tags && _credential_ids) {
		if (_tags->size < 1) {
			safeword_list_tags(&db, _credential_ids[0], 0, 0);
		} else {
			for (i = 0; i < _credential_ids_size; i++) {
				for (j = 0; j < _tags->size; j++) {
					if (_untag)
						safeword_credential_untag(&db, _credential_ids[i], _tags->data[j]);
					else
						safeword_tag_credential(&db, _credential_ids[i], _tags->data[j]);
				}
			}
		}
	} else {
		safeword_list_tags(&db, 0, 0, 0);
	}

fail:
	for (i = 0; i < _tags->size; i++)
		free(_tags->data[i]);
	free(_tags);
	safeword_close(&db);
	return ret;
}
