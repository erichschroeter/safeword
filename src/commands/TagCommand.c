#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>

#include <safeword.h>
#include "TagCommand.h"

#define MAXBUFFERSIZE 16
#define EMULTIPLE_SUBCOMMANDS 257

static int force;
static FILE *_wiki_file;

struct array {
	unsigned int size;
	char **data;
};

struct tag_subcommand {
	int	(*execute)(struct safeword_db *db, void *data);
};
static struct tag_subcommand tag_subcommand_;

struct update_info {
	char *tag;
	FILE *file;
};

static int *credential_ids;
static int credential_ids_size;
static struct array *tags_;

char* tagCmd_help(void)
{
	return "SYNOPSIS\n"
"	tag [-d | --delete] [-f | --force] [-m | --move] [-w | --wiki] [ID] TAGS ...\n"
"\n"
"DESCRIPTION\n"
"	This command maps tags to credentials within the safeword database.\n"
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
"\n";
}

static int delete_tags(struct safeword_db* db, void *tags_array)
{
	int ret = 0, i, char_count;
	char input, input_buffer[MAXBUFFERSIZE];
	struct array *tags = (struct array*) tags_array;

	if (!tags) {
		fprintf(stderr, "no tags specified\n");
		return -1;
	}

	for (i = 0; i < tags->size; i++) {
		if (force) {
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

fail:
	return ret;
}

static int rename_tag(struct safeword_db* db, void *tags_array)
{
	struct array *tags = (struct array*) tags_array;

	if (!tags) {
		fprintf(stderr, "no tags specified\n");
		return -1;
	}

	if (tags->size < 2) {
		fprintf(stderr, "%s\n", tags->size ? "no new tag name specified" : "no tags specified");
		return -1;
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
		return -1;
	}

	if (!info->file) {
		fprintf(stderr, "no wiki file specified\n");
		return -1;
	}

	if (info->file != stdin) {
		if ((ret = fseek(info->file, 0, SEEK_END)) == -1) {
			fprintf(stderr, "error seek end: %s\n", strerror(errno));
			goto fail;
		}
		if ((wiki_size = ftell(info->file)) == -1) {
			ret = errno;
			fprintf(stderr, "ftell failed: %s\n", strerror(errno));
			goto fail;
		}
		if ((ret = fseek(info->file, 0, SEEK_SET)) == -1) {
			fprintf(stderr, "error seek set: %s\n", strerror(errno));
			goto fail;
		}
		wiki = calloc(wiki_size + 1, sizeof(char));
		if (!wiki) {
			ret = -ENOMEM;
			goto fail;
		}
		ret = fread(wiki, 1, wiki_size, info->file);
	} else {
		char c;
		char buffer[1024];
		wiki = calloc(1024, sizeof(char));
		buffer[0] = '\0';
		wiki_size = 0;
		while (fgets(buffer, 1024, info->file)) {
			char *old = wiki;
			wiki_size += strlen(buffer);
			wiki = realloc(wiki, wiki_size);
			if (!wiki) {
				ret = -ENOMEM;
				goto fail;
			}
			strcat(wiki, buffer);
		}
	}

	safeword_tag_update(db, info->tag, wiki);

fail:
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
	};

	tag_subcommand_.execute = NULL;

	while ((i = getopt_long(argc, argv, "w:dfm", long_options, 0)) != -1) {
		switch (i) {
		case 'd':
			if (tag_subcommand_.execute) {
				ret = -EMULTIPLE_SUBCOMMANDS;
				goto fail;
			}
			tag_subcommand_.execute = &delete_tags;
			break;
		case 'f':
			force = 1;
			break;
		case 'm':
			if (tag_subcommand_.execute) {
				ret = -EMULTIPLE_SUBCOMMANDS;
				goto fail;
			}
			tag_subcommand_.execute = &rename_tag;
			break;
		case 'w':
			if (!strncmp(optarg, "-", 1)) {
				_wiki_file = stdin;
			} else {
				_wiki_file = fopen(optarg, "r");
				if (!_wiki_file) {
					ret = -EEXIST;
					goto fail;
				}
			}
			tag_subcommand_.execute = &update_tag;
			break;
		}
	}

	remaining_args = argc - optind;

	/* START of parsing credential ids */
	if (!tag_subcommand_.execute && remaining_args > 0) {
		int i = 0, id;
		char *id_str;
		char *ids_backup = calloc(strlen(argv[optind]), sizeof(char));
		int tags_size = 0;
		char **tags = malloc(remaining_args * sizeof(*tags));
		if (!tags) {
			ret = -ENOMEM;
			goto fail;
		}
		strcpy(ids_backup, argv[optind]);

		/* get number of credential ids in string */
		id_str = strtok(ids_backup, ",");
		while (id_str != NULL) {
			id_str = strtok(NULL, ",");
			credential_ids_size++;
		}

		credential_ids = calloc(credential_ids_size, sizeof(*credential_ids));
		if (!credential_ids) {
			ret = -ENOMEM;
			goto fail;
		}
		ids_backup = argv[optind];
		id_str = strtok(ids_backup, ",");
		while (id_str != NULL) {
			id = atoi(id_str);
			credential_ids[i] = id;
			id_str = strtok(NULL, ",");
			i++;
		}
		optind++;
		remaining_args--;
	}
	/* END of parsing credential ids */

	/* START of parsing tags */
	tags_ = malloc(sizeof(*tags_));
	if (!tags_) {
		ret = -ENOMEM;
		goto fail;
	}
	tags_->size = 0;
	tags_->data = malloc(remaining_args * sizeof(*tags_->data));
	if (!tags_->data) {
		ret = -ENOMEM;
		goto fail;
	}
	/* copy tags to the tags array */
	for (i = 0; i < remaining_args; i++) {
		tags_->data[i] = calloc(strlen(argv[optind]), sizeof(char));
		if (!tags_->data[i]) {
			ret = -ENOMEM;
			goto fail;
		}
		strcpy(tags_->data[i], argv[optind]);
		tags_->size++;
		optind++;
	}
	/* END of parsing tags */

fail:
	return ret;
}

int tagCmd_execute(void)
{
	int ret = 0, i, j, char_count;
	struct safeword_db db;
	char *sql, input, input_buffer[MAXBUFFERSIZE];

	ret = safeword_db_open(&db, 0);
	if (ret)
		goto fail;

	if (tag_subcommand_.execute == &delete_tags ||
		tag_subcommand_.execute == &rename_tag) {
		tag_subcommand_.execute(&db, tags_);
	} else if (tag_subcommand_.execute == &update_tag) {
		struct update_info info;
		info.tag = tags_->data[0];
		info.file = _wiki_file;
		tag_subcommand_.execute(&db, &info);
	} else if (tags_ && credential_ids) {
		for (i = 0; i < credential_ids_size; i++) {
			for (j = 0; j < tags_->size; j++) {
				safeword_tag_credential(&db, credential_ids[i], tags_->data[j]);
			}
		}
	} else {
		safeword_list_tags(&db, 0, 0, 0);
	}

fail:
	for (i = 0; i < tags_->size; i++)
		free(tags_->data[i]);
	free(tags_);
	return ret;
}
