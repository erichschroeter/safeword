#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <limits.h>

#ifdef WIN32
#include "windows.h"
#else
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>
#endif

#include "dbg.h"
#include "safeword.h"
#include "commands/Command.h"

int errno_safeword = 0;
static long int _tag_id;
static int _id;
static int _copy_once = 0;

char* strerror_safeword(int errnum)
{
	switch (errnum) {
	case ESAFEWORD_DBEXIST:
	case -ESAFEWORD_DBEXIST:
		return "safeword db does not exist";
	case ESAFEWORD_INVARG:
	case -ESAFEWORD_INVARG:
		return "invalid argument";
	case ESAFEWORD_FIELDEXIST:
	case -ESAFEWORD_FIELDEXIST:
		return "field does not exist";
	case ESAFEWORD_INTERNAL:
	case -ESAFEWORD_INTERNAL:
		return "internal";
	default:
		return strerror(errnum);
	}
}

void perror_safeword(const char *string)
{
	if (string)
		fprintf(stderr, "%s: ", string);

	fprintf(stderr, "%s", strerror_safeword(errno_safeword));
}

int safeword_config(const char* key, const char* value)
{
	if (!strcmp(key, "copy_once")) {
		if (strlen(value) == 1 && value[0] == '1')
			_copy_once = 1;
	} else
		return -1;

	return 0;
}

/* #region safeword init function */

int safeword_init(const char *path)
{
	int ret = 0;
	sqlite3* handle;
	char sql[512];

	safeword_check(path, -ESAFEWORD_INVARG, fail);

	/* ensure that path does not already exist */
	safeword_check(access(path, F_OK) == -1, -ESAFEWORD_DBEXIST, fail);

	ret = sqlite3_open(path, &handle);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS tags "
		"(id INTEGER PRIMARY KEY, "
		"tag TEXT NOT NULL, "
		"wiki TEXT, "
		"UNIQUE (tag) ON CONFLICT ABORT, "
		"CONSTRAINT no_empty_tag CHECK (tag != '')"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS usernames "
		"(id INTEGER PRIMARY KEY, "
		"username TEXT, "
		"UNIQUE (username) ON CONFLICT ABORT"
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS passwords "
		"(id INTEGER PRIMARY KEY, "
		"password TEXT, "
		"UNIQUE (password) ON CONFLICT ABORT "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS credentials ("
		"id INTEGER PRIMARY KEY, "
		"usernameid INTEGER REFERENCES usernames(id), "
		"passwordid INTEGER REFERENCES passwords(id), "
		"description TEXT "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

	sprintf(sql, "CREATE TABLE IF NOT EXISTS tagged_credentials ("
		"credentialid INTEGER NOT NULL REFERENCES credentials(id) ON DELETE CASCADE, "
		"tagid INTEGER NOT NULL REFERENCES tags(id) ON DELETE CASCADE, "
		"PRIMARY KEY (credentialid, tagid) "
		");");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

	sqlite3_close(handle);

	return 0;
fail:
	return -1;
}

/* #endregion safeword init function */

/* #region safeword open & close */

int safeword_open(struct safeword_db *db, const char *path)
{
	int ret = 0;

	if (path) {
		db->path = calloc(strlen(path) + 1, sizeof(char));
		safeword_check(db->path, -ENOMEM, fail);
		strcpy(db->path, path);
	} else {
		char *env = getenv("SAFEWORD_DB");
		if (!env) {
			ret = -ESAFEWORD_DBEXIST;
			debug("SAFEWORD_DB environment variable not set");
			goto fail;
		}
		db->path = calloc(strlen(env) + 1, sizeof(char));
		safeword_check(db->path, -ENOMEM, fail);
		strcpy(db->path, env);
	}

	// fail if file does not exist, otherwise sqlite3 will create it
	if (access(db->path, F_OK) == -1) {
		ret = -ESAFEWORD_DBEXIST;
		debug("safeword database '%s' does not exist", db->path);
		goto fail;
	}

	ret = sqlite3_open(db->path, &(db->handle));
	if (ret) {
		ret = -ESAFEWORD_DBEXIST;
		debug("failed to open safeword database '%s'", db->path);
		goto fail;
	}

	/* enable foreign key support in Sqlite3 so delete cascading works. */
	ret = sqlite3_exec(db->handle, "PRAGMA foreign_keys = ON;", 0, 0, 0);
	safeword_check(!ret, -ESAFEWORD_INTERNAL, fail);

fail:
	return ret;
}

int safeword_close(struct safeword_db *db)
{
	int ret = 0;

	safeword_check(db, -ESAFEWORD_DBEXIST, fail);

	free(db->path);
	sqlite3_close(db->handle);

fail:
	return ret;
}

/* #endregion safeword open & close */

/* #region safeword list functions */

static int credentials_callback(void* not_used, int argc, char** argv, char** col_name)
{
	char* description = argv[1];
	long int credential_id = atoi(argv[0]);

	if (!description)
		description = "";

	printf("%ld : %s\n", credential_id, description);
	return 0;
}

int safeword_list_credentials(struct safeword_db *db, unsigned int tags_size, char **tags)
{
	int ret = 0, i;
	char *sql;

	if (tags) {
		char *tags_concat;
		/* init to include the commas & null terminator */
		int tags_concat_size = tags_size + 1;

		for (i = 0; i < tags_size; i++) {
			/* add 2 for wrapping in single quotes */
			tags_concat_size += strlen(tags[i]) + 2;
		}
		tags_concat = calloc(tags_concat_size, sizeof(char));
		safeword_check(tags_concat, -ENOMEM, fail);

		for (i = 0; i < tags_size; i++) {
			strcat(tags_concat, "'");
			strcat(tags_concat, tags[i]);
			strcat(tags_concat, "'");
			if (i != tags_size - 1)
				strcat(tags_concat, ",");
		}

		sql = calloc(strlen(tags_concat) + 256, sizeof(char));
		safeword_check(sql, -ENOMEM, fail);

		/* Find credentials with the specified tags */
		sprintf(sql, "SELECT c.id,c.description FROM credentials AS c "
			"INNER JOIN tagged_credentials AS tc "
			"INNER JOIN tags AS t ON "
			"(c.id = tc.credentialid AND tc.tagid = t.id) WHERE "
			"t.tag IN (%s) GROUP BY c.id HAVING COUNT(c.id) = %d;",
			tags_concat, tags_size);
		ret = sqlite3_exec(db->handle, sql, credentials_callback, 0, 0);
		free(tags_concat);
		free(sql);
	} else {
		if (tags_size == UINT_MAX) {
			sql = calloc(100, sizeof(char));
			safeword_check(sql, -ENOMEM, fail);

			/* Find all credentials */
			sprintf(sql, "SELECT id,description FROM credentials;");
			ret = sqlite3_exec(db->handle, sql, credentials_callback, 0, 0);
			free(sql);
		} else {
			sql = calloc(256, sizeof(char));
			safeword_check(sql, -ENOMEM, fail);

			/* Find credentials that have no tags */
			sprintf(sql, "SELECT id,description FROM credentials "
			"WHERE id NOT IN ("
			"SELECT credentialid FROM tagged_credentials "
			"WHERE tagid IN ("
			"SELECT id FROM tags));");
			ret = sqlite3_exec(db->handle, sql, credentials_callback, 0, 0);
			free(sql);
		}
	}

fail:
	return ret;
}

/* #endregion safeword list functions */

/* #region safeword info functions */

static int credential_info_callback(void* credential, int argc, char** argv, char** col_name)
{
	struct safeword_credential *cred = (struct safeword_credential*) credential;
	char *field;
	int ret;

	if (argc < 3)
		return -1;
	if (!cred)
		return -1;

	field = calloc(strlen(argv[0]) + 1, sizeof(char));
	safeword_check(field, -ENOMEM, fail);
	strcpy(field, argv[0]);
	cred->username = field;

	field = calloc(strlen(argv[1]) + 1, sizeof(char));
	safeword_check(field, -ENOMEM, fail);
	strcpy(field, argv[1]);
	cred->password = field;

	field = calloc(strlen(argv[2]) + 1, sizeof(char));
	safeword_check(field, -ENOMEM, fail);
	strcpy(field, argv[2]);
	cred->message = field;

	return 0;

fail:
	return ret;
}

int safeword_credential_info(struct safeword_db *db, struct safeword_credential *credential)
{
	int ret;
	char* sql;

	sql = calloc(256, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT u.username,p.password,c.description FROM credentials AS c "
		"INNER JOIN usernames AS u ON (c.usernameid = u.id) "
		"INNER JOIN passwords AS p ON (c.passwordid = p.id) "
		"WHERE c.id = '%d';", credential->id);
	ret = sqlite3_exec(db->handle, sql, credential_info_callback, credential, 0);
	free(sql);
fail:
	return ret;
}

int safeword_credential_free(struct safeword_credential *credential)
{
	int ret = 0, i;

	if (!credential)
		return 0;

	free(credential->username);
	free(credential->password);
	free(credential->message);

	for (i = 0; i < credential->tags_size; i++)
		free(credential->tags[i]);
	free(credential->tags);

fail:
	return ret;
}

static int tag_info_callback(void* not_used, int argc, char** argv, char** col_name)
{
	if (argc < 1)
		return -1;

	printf("TAG:\t%s\n", argv[0]);

	return 0;
}

int safeword_tag_info(struct safeword_db *db, const char *tag)
{
	int ret;
	char* sql;

	/* TODO print the wiki/markdown text for the specified tag */
	/* TODO print the number of mapped credentials for the specified tag */
	sql = calloc(256, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT tag FROM tags "
		"WHERE tag = '%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_info_callback, 0, 0);
	free(sql);
fail:
	return ret;
}

/* #endregion safeword info functions */

/* #region safeword credential functions */

static int id_callback(void* not_used, int argc, char** argv, char** col_name) { _id = atoi(argv[0]); return 0; }

static int map_to_credential(sqlite3* handle, const char* value, const char* table, const char* field,
	sqlite3_int64 credential_rowid)
{
	int ret;
	char* sql;

	_id = 0; // set to invalid value

	sql = calloc(strlen(value) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT id FROM %s WHERE %s='%s';", table, field, value);;
	ret = sqlite3_exec(handle, sql, id_callback, &credential_rowid, 0);
	free(sql);

	/* if the field does not exist create it, so we can map it */
	if (!_id) {
		sql = calloc(strlen(value) + 100, sizeof(char));
		safeword_check(sql, -ENOMEM, fail);

		sprintf(sql, "INSERT INTO %s (%s) VALUES ('%s');", table, field, value);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
		_id = sqlite3_last_insert_rowid(handle);
	}

	sql = calloc(strlen(value) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "UPDATE credentials SET %sid = %d WHERE id = %d;",
		field, _id, (int)credential_rowid);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int map_username(sqlite3* handle, const char* username, sqlite3_int64 credential_rowid)
{
	return map_to_credential(handle, username, "usernames", "username", credential_rowid);
}

int map_password(sqlite3* handle, const char* password, sqlite3_int64 credential_rowid)
{
	return map_to_credential(handle, password, "passwords", "password", credential_rowid);
}

int safeword_credential_add(struct safeword_db *db, int *credential_id,
	const char *username, const char *password, const char *description)
{
	int ret = 0;
	char* sql;

	if (!db)
		return -1;

	sql = calloc(100, sizeof(char));
	sprintf(sql, "INSERT INTO credentials DEFAULT VALUES;");
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);
	*credential_id = sqlite3_last_insert_rowid(db->handle);

	if (description) {
		sql = calloc(strlen(description) + 100, sizeof(char));
		safeword_check(sql, -ENOMEM, fail);

		sprintf(sql, "UPDATE credentials SET description = '%s' WHERE id = %d;",
			description, *credential_id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		free(sql);
	}

	if (username)
		map_username(db->handle, username, *credential_id);
	if (password)
		map_password(db->handle, password, *credential_id);
fail:
	return ret;
}

int safeword_credential_remove(struct safeword_db *db, int credential_id)
{
	int ret;
	char *sql;

	sql = calloc(100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "DELETE FROM credentials WHERE id = '%d';", credential_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_credential_update(struct safeword_db *db, struct safeword_credential *credential)
{
	int ret = 0;
	char *sql;

	if (!credential->id)
		goto fail;

	sql = calloc(512, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	if (credential->username) {
		/* Add the new username to the database if it does not already exist. */
		sprintf(sql, "INSERT OR IGNORE INTO usernames (username) VALUES ('%s');", credential->username);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		/* Update the usernameid to the id of the username if it exists. */
		sprintf(sql, "UPDATE OR ABORT credentials SET usernameid = COALESCE("
			"(SELECT id FROM usernames WHERE username = '%s'), usernameid) WHERE id = %d;",
			credential->username, credential->id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	}

	if (credential->password) {
		/* Add the new password to the database if it does not already exist. */
		sprintf(sql, "INSERT OR IGNORE INTO passwords (password) VALUES ('%s');", credential->password);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		/* Update the passwordid to the id of the password if it exists. */
		sprintf(sql, "UPDATE OR ABORT credentials SET passwordid = COALESCE("
			"(SELECT id FROM passwords WHERE password = '%s'), passwordid) WHERE id = %d;",
			credential->password, credential->id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	}

	if (credential->message) {
		/* Update the message if it exists. */
		sprintf(sql, "UPDATE OR ABORT credentials SET description = '%s' WHERE id = %d;",
			credential->message, credential->id);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	}

fail:
	free(sql);
	return ret;
}

/* #endregion safeword credential functions */

/* #region safeword tag functions */

static int tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	_tag_id = atoi(argv[0]);
	return 0;
}

int safeword_credential_tag(struct safeword_db *db, long int credential_id, const char *tag)
{
	int ret;
	char *sql;
	_tag_id = 0; /* set to invalid value to represent it does not exist */

	safeword_check(credential_id, -ESAFEWORD_INVARG, fail);
	safeword_check(tag, -ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_callback, 0, 0);
	free(sql);

	if (!_tag_id) {
		sql = calloc(strlen(tag) + 100, sizeof(char));
		safeword_check(sql, -ENOMEM, fail);

		sprintf(sql, "INSERT INTO tags (tag) VALUES ('%s');", tag);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		free(sql);
		_tag_id = sqlite3_last_insert_rowid(db->handle);
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	/* Either create or overwrite the mapping between credential and tag */
	sprintf(sql, "INSERT OR REPLACE INTO tagged_credentials "
		"(credentialid, tagid) VALUES (%ld, %ld);",
		credential_id, _tag_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_credential_untag(struct safeword_db *db, long int credential_id, const char *tag)
{
	int ret;
	char *sql;
	_tag_id = 0; /* set to invalid value to represent it does not exist */

	safeword_check(credential_id, -ESAFEWORD_INVARG, fail);
	safeword_check(tag, -ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_callback, 0, 0);
	free(sql);

	if (!_tag_id) {
		ret = -ESAFEWORD_INVARG;
		goto fail;
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	/* Delete the mapping between the credential and tag */
	sprintf(sql, "DELETE FROM tagged_credentials WHERE "
		"(credentialid = %ld AND tagid = %ld);",
		credential_id, _tag_id);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_tag_update(struct safeword_db *db, const char *tag, const char *wiki)
{
	int ret;
	char *sql;

	safeword_check(tag, -ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, tag_callback, 0, 0);
	free(sql);

	/* make sure the tag exists */
	safeword_check(_tag_id, -ESAFEWORD_FIELDEXIST, fail);

	if (wiki) {
		sql = calloc(strlen(wiki) + strlen(tag) + 100, sizeof(char));
		safeword_check(sql, -ENOMEM, fail);

		/* Replace the existing wiki column value with the new value */
		sprintf(sql, "UPDATE OR ABORT tags SET wiki = '%s' WHERE tag = '%s';", wiki, tag);
		ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
		free(sql);
	}
fail:
	return ret;
}

static int print_tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	printf("%s\n", argv[0]);
	return 0;
}

int safeword_list_tags(struct safeword_db *db, int credential_id, unsigned int *tags_size, const char ***tags)
{
	int ret = 0;
	char *sql;

	sql = calloc(512, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	if (!credential_id) {
		/* Find all tags */
		sprintf(sql, "SELECT tag FROM tags;");
	} else {
		/* Find all tags for the specified credential ID */
		sprintf(sql, "SELECT t.tag FROM tags AS t INNER JOIN tagged_credentials AS c "
			"ON (c.tagid = t.id) WHERE c.credentialid = %d;", credential_id);
	}
	ret = sqlite3_exec(db->handle, sql, print_tag_callback, 0, 0);

	free(sql);
fail:
	return ret;
}

int safeword_tag_delete(struct safeword_db *db, const char *tag)
{
	int ret;
	char *sql;

	safeword_check(tag, -ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(tag) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "DELETE FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

int safeword_tag_rename(struct safeword_db *db, const char *old, const char *new)
{
	int ret;
	char *sql;

	safeword_check(old, -ESAFEWORD_INVARG, fail);
	safeword_check(new, -ESAFEWORD_INVARG, fail);

	sql = calloc(strlen(old) + strlen(new) + 100, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "UPDATE tags SET tag = '%s' WHERE tag = '%s';", new, old);
	ret = sqlite3_exec(db->handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

/* #endregion safeword tag functions */

/* #region safeword cp functions */

/* returns the difference in milliseconds */
static unsigned int diff(struct timespec *start, struct timespec *end)
{
	return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
		((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}

#ifndef WIN32
struct __async_waiting_data {
	volatile int *waiting;
	Display *dpy;
	Window *win;
	unsigned char *data;
};

static void* wait_for_clipboard_request(void *waiting_data)
{
	struct __async_waiting_data *data = (struct __async_waiting_data*) waiting_data;
	Display *dpy = data->dpy;
	Window win = *data->win;
	XSelectionRequestEvent *req;
	XEvent e, respond;
	static Atom targets;

	if (!targets)
		targets = XInternAtom(dpy, "TARGETS", False);

	XSelectInput(dpy, win, PropertyChangeMask);
	XSelectInput(dpy, win, StructureNotifyMask+ExposureMask);
	XSetSelectionOwner(dpy, XA_CLIPBOARD(dpy), win, CurrentTime);

	while (*data->waiting) {
		XFlush(dpy);
		XNextEvent(dpy, &e);
		if (e.type == SelectionRequest) {
			req=&(e.xselectionrequest);
			if (req->target == targets) {
				Atom supported[] = {
					XA_UTF8_STRING(dpy),
					XA_STRING
				};
				XChangeProperty(dpy,
					req->requestor,
					req->property,
					XA_ATOM,
					32,
					PropModeReplace,
					(unsigned char*) supported,
					(int) (sizeof(supported) / sizeof(Atom)));
				respond.xselection.property=req->property;
			} else if (req->target == XA_UTF8_STRING(dpy) ||
				req->target == XA_STRING) {
				XChangeProperty(dpy,
					req->requestor,
					req->property,
					req->target,
					8,
					PropModeReplace,
					(unsigned char*) data->data,
					strlen(data->data));
				respond.xselection.property=req->property;

				if (_copy_once) {
					*data->waiting = 0;
				}
			} else {
				respond.xselection.property= None;
			}
			respond.xselection.type= SelectionNotify;
			respond.xselection.display= req->display;
			respond.xselection.requestor= req->requestor;
			respond.xselection.selection=req->selection;
			respond.xselection.target= req->target;
			respond.xselection.time = req->time;
			XSendEvent(dpy, req->requestor,0,0,&respond);
			XFlush(dpy);
		}
	}
}
#endif

static int copy_credential_callback(void* millis, int argc, char** argv, char** col_name)
{
#ifdef WIN32
	DWORD len = strlen(argv[0]);
	HGLOBAL lock;
	LPWSTR data;

	lock = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, (len + 1) * sizeof(char));
	data = (LPWSTR)GlobalLock(lock);
	memcpy(data, argv[0], len * sizeof(char));
	data[len] = 0;
	GlobalUnlock(lock);

	// Set clipboard data
	if (!OpenClipboard(NULL)) return GetLastError();
	EmptyClipboard();
	if (!SetClipboardData(CF_TEXT, lock)) return GetLastError();
	CloseClipboard();

	return 0;
#else
	int ret = 0;
	unsigned int *ms = millis;
	volatile int waiting = 1;
	unsigned char *data;
	struct timespec start, now;
	Display *dpy;
	Window win;

	/* copy the data to local variable that will stay in scope for waiting thread */
	data = calloc(strlen(argv[0]) + 1, sizeof(char));
	safeword_check(data, -ENOMEM, fail);

	strcpy(data, argv[0]);

	if (!(dpy = XOpenDisplay(NULL))) {
		debug("could not open display\n");
		ret = -ESAFEWORD_INTERNAL;
		goto fail;
	}

	/* create a window to trap events */
	win = XCreateSimpleWindow(dpy, DefaultRootWindow(dpy), 0, 0, 1, 1, 0, 0, 0);

	clock_gettime(CLOCK_MONOTONIC, &start);
	clock_gettime(CLOCK_MONOTONIC, &now);

	struct __async_waiting_data waiting_data;
	waiting_data.waiting = &waiting;
	waiting_data.dpy = dpy;
	waiting_data.win = &win;
	waiting_data.data = data;

	pthread_t tid;
	pthread_create(&tid, NULL, &wait_for_clipboard_request, &waiting_data);

	while (waiting) {
		if (diff(&start, &now) > *ms) {
			pthread_cancel(tid);
			waiting = 0;
		}

		clock_gettime(CLOCK_MONOTONIC, &now);
	}

fail:
	free(data);
	return ret;
#endif
}

static int safeword_cp(struct safeword_db *db, int credential_id, unsigned int ms,
	const char* table, const char* field)
{
	int ret;
	char *sql;

	sql = calloc(256, sizeof(char));
	safeword_check(sql, -ENOMEM, fail);

	sprintf(sql, "SELECT t.%s FROM %s AS t "
		"INNER JOIN credentials AS c "
		"ON (c.%sid = t.id) WHERE c.id = %d;", field, table, field, credential_id);
	ret = sqlite3_exec(db->handle, sql, copy_credential_callback, &ms, 0);
	free(sql);
fail:
	return ret;
}

int safeword_cp_username(struct safeword_db *db, int credential_id, unsigned int ms)
{
	return safeword_cp(db, credential_id, ms, "usernames", "username");
}

int safeword_cp_password(struct safeword_db *db, int credential_id, unsigned int ms)
{
	return safeword_cp(db, credential_id, ms, "passwords", "password");
}

/* #endregion safeword cp functions */

