#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include "safeword.h"
#include "commands/Command.h"

static sqlite3_int64 tag_id;
static int _id;
static int _copy_once = 0;

int safeword_config(const char* key, const char* value)
{
	if (!strcmp(key, "copy_once")) {
		if (strlen(value) == 1 && value[0] == '1')
			_copy_once = 1;
	} else
		return -1;

	return 0;
}

static int tag_callback(void* not_used, int argc, char** argv, char** col_name)
{
	tag_id = atoi(argv[0]);
	return 0;
}

int safeword_db_open(sqlite3 **handle)
{
	int ret = 0;
	char *db = malloc(sizeof(char*));

	if (!db) {
		ret = -ENOMEM;
		goto fail;
	}
	db = getenv("SAFEWORD_DB");
	if (!db) {
		ret = -ESAFEWORD_ENV_NOT_SET;
		fprintf(stderr, "SAFEWORD_DB environment variable not set\n");
		goto fail;
	}

	// fail if file does not exist, otherwise sqlite3 will create it
	if (access(db, F_OK) == -1) {
		ret = -ESAFEWORD_DB_NOEXIST;
		fprintf(stderr, "safeword database '%s' does not exist\n", db);
		goto fail;
	}

	ret = sqlite3_open(db, handle);
	if (ret) {
		fprintf(stderr, "failed to open safeword database '%s'\n", db);
		goto fail;
	}

	/* enable foreign key support in Sqlite3 so delete cascading works. */
	ret = sqlite3_exec(*handle, "PRAGMA foreign_keys = ON;", 0, 0, 0);

fail:
	return ret;
}

static int id_callback(void* not_used, int argc, char** argv, char** col_name) { _id = atoi(argv[0]); return 0; }

static int map_to_credential(sqlite3* handle, const char* password, const char* table, const char* field,
	sqlite3_int64 credential_rowid)
{
	int ret;
	char* sql;

	_id = -1; // set to invalid value

	sql = calloc(strlen(password) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT id FROM %s WHERE %s='%s';", table, field, password);;
	ret = sqlite3_exec(handle, sql, id_callback, &credential_rowid, 0);
	free(sql);

	if (_id == -1) {
		sql = calloc(strlen(password) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}

		sprintf(sql, "INSERT INTO %s (%s) VALUES ('%s');", table, field, password);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
		_id = sqlite3_last_insert_rowid(handle);
	}

	sql = calloc(strlen(password) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}

	sprintf(sql, "UPDATE credentials SET %sid = %d WHERE id = %d;",
		field, _id, (int)credential_rowid);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int map_username(sqlite3* handle, const char* username, sqlite3_int64 credential_rowid)
{
	map_to_credential(handle, username, "usernames", "username", credential_rowid);
	return 0;
}

int map_password(sqlite3* handle, const char* password, sqlite3_int64 credential_rowid)
{
	map_to_credential(handle, password, "passwords", "password", credential_rowid);
	return 0;
}

int safeword_credential_add(sqlite3* handle, sqlite3_int64 *credential_id,
	const char *username, const char *password, const char *description)
{
	int ret = 0;
	char* sql;

	sql = calloc(100, sizeof(char));
	sprintf(sql, "INSERT INTO credentials DEFAULT VALUES;");
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);
	*credential_id = sqlite3_last_insert_rowid(handle);

	if (description) {
		sql = calloc(strlen(description) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}

		sprintf(sql, "UPDATE credentials SET description = '%s' WHERE id = %d;",
			description, *credential_id);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
	}

	map_username(handle, username, *credential_id);
	map_password(handle, password, *credential_id);
fail:
	return ret;
}

int safeword_tag_credential(sqlite3 *handle, sqlite3_int64 credential_id, const char *tag)
{
	int ret;
	char *sql;

	tag_id = -1; /* set to invalid value to represent it does not exist */

	sql = calloc(strlen(tag) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT id FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(handle, sql, tag_callback, 0, 0);
	free(sql);

	if (tag_id == -1) {
		sql = calloc(strlen(tag) + 100, sizeof(char));
		if (!sql) {
			ret = -ENOMEM;
			goto fail;
		}
		sprintf(sql, "INSERT INTO tags (tag) VALUES ('%s');", tag);
		ret = sqlite3_exec(handle, sql, 0, 0, 0);
		free(sql);
		tag_id = sqlite3_last_insert_rowid(handle);
	}

	sql = calloc(strlen(tag) + 256, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "INSERT OR REPLACE INTO tagged_credentials "
		"(credentialid, tagid) VALUES (%d, %d);",
		credential_id, tag_id);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);

fail:
	return ret;
}

int safeword_delete_tag(sqlite3* handle, const char *tag)
{
	int ret;
	char *sql;

	sql = calloc(strlen(tag) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "DELETE FROM tags WHERE tag='%s';", tag);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

int safeword_rename_tag(sqlite3* handle, const char *old, const char *new)
{
	int ret;
	char *sql;

	sql = calloc(strlen(old) + strlen(new) + 100, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "UPDATE tags SET tag = '%s' WHERE tag = '%s';", new, old);
	ret = sqlite3_exec(handle, sql, 0, 0, 0);
	free(sql);
fail:
	return ret;
}

/* returns the difference in milliseconds */
static unsigned int diff(struct timespec *start, struct timespec *end)
{
	return ((end->tv_sec * 1000) + (end->tv_nsec / 1000000)) -
		((start->tv_sec * 1000) + (start->tv_nsec / 1000000));
}

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

	XSelectInput(dpy, win, PropertyChangeMask);
	XSelectInput(dpy, win, StructureNotifyMask+ExposureMask);
	XSetSelectionOwner(dpy, XA_CLIPBOARD(dpy), win, CurrentTime);

	while (*data->waiting) {
		XFlush(dpy);
		XNextEvent(dpy, &e);
		if (e.type == SelectionRequest) {
			req=&(e.xselectionrequest);
			if (req->target == XA_STRING) {
				XChangeProperty(dpy,
					req->requestor,
					req->property,
					XA_STRING,
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

static int copy_credential_callback(void* millis, int argc, char** argv, char** col_name)
{
	unsigned int *ms = millis;
	volatile int waiting = 1;
	unsigned char *data;
	struct timespec start, now;
	Display *dpy;
	Window win;

	/* copy the data to local variable that will stay in scope for waiting thread */
	data = calloc(strlen(argv[0]) + 1, sizeof(char));
	if (!data)
		return -ENOMEM;
	strcpy(data, argv[0]);

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "could not open display\n");
		return;
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

	return 0;
}

static int safeword_cp(sqlite3* handle, sqlite3_int64 credential_id, unsigned int ms,
	const char* table, const char* field)
{
	int ret;
	char *sql;

	sql = calloc(256, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT t.%s FROM %s AS t "
		"INNER JOIN credentials AS c "
		"ON (c.%sid = t.id) WHERE c.id = %d;", field, table, field, credential_id);
	ret = sqlite3_exec(handle, sql, copy_credential_callback, &ms, 0);
	free(sql);
fail:
	return ret;
}

int safeword_cp_username(sqlite3* handle, sqlite3_int64 credential_id, unsigned int ms)
{
	return safeword_cp(handle, credential_id, ms, "usernames", "username");
}

int safeword_cp_password(sqlite3* handle, sqlite3_int64 credential_id, unsigned int ms)
{
	return safeword_cp(handle, credential_id, ms, "passwords", "password");
}
