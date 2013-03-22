#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <getopt.h>
#include <sqlite3.h>
#include <time.h>
#include <pthread.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

#include <safeword.h>
#include "CopyCommand.h"

static unsigned int seconds_ = 10;
static sqlite3_int64 credential_id;

char* copyCmd_help(void)
{
	return "SYNOPSIS\n"
"	cp [-t[SECONDS] | --time[=SECONDS]] ID\n"
"\n"
"DESCRIPTION\n"
"	This command copies a credential to the clipboard.\n"
"\n"
"OPTIONS\n"
"	-t, --time\n"
"	    The number of seconds before the clipboard is cleared. Default is 10 seconds.\n"
"	    Specifying no argument is equivalent to entering 0 (entering 0 will not clear the\n"
"	    clipboard)\n"
"\n";
}

static int diff(struct timespec *start, struct timespec *end)
{
	return (end->tv_sec + (end->tv_nsec / 1000000000)) -
		(start->tv_sec + (start->tv_nsec / 1000000000));
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

	XSelectInput(dpy, win, StructureNotifyMask);
	XMapWindow(dpy, win);
	XSelectionRequestEvent *req;
	XEvent e, respond;
	for(;;) {
		XNextEvent(dpy, &e);
		if (e.type == MapNotify) break;
	}
	XFlush(dpy);

	XSelectInput(dpy, win, StructureNotifyMask+ExposureMask);
	XSetSelectionOwner (dpy, XA_CLIPBOARD(dpy), win, CurrentTime);
	while (*data->waiting) {
		XFlush (dpy);
		XNextEvent (dpy, &e);
		if (e.type == SelectionRequest) {
			req=&(e.xselectionrequest);
			if (req->target == XA_STRING) {
				XChangeProperty (dpy,
					req->requestor,
					req->property,
					XA_STRING,
					8,
					PropModeReplace,
					(unsigned char*) data->data,
					32);
				respond.xselection.property=req->property;
			} else {
				respond.xselection.property= None;
			}
			respond.xselection.type= SelectionNotify;
			respond.xselection.display= req->display;
			respond.xselection.requestor= req->requestor;
			respond.xselection.selection=req->selection;
			respond.xselection.target= req->target;
			respond.xselection.time = req->time;
			XSendEvent (dpy, req->requestor,0,0,&respond);
			XFlush (dpy);
		}
	}
}

static int copy_credential_callback(void* secs, int argc, char** argv, char** col_name)
{
	unsigned int *seconds = secs;
	volatile int waiting_ = 1;
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

	/* fork into the background, exit parent process */
	pid_t pid;

	pid = fork();
	/* exit the parent process */
	if (pid) {
		exit(EXIT_SUCCESS);
	}

	/* Avoid making the current directory in use, in case it will need to be umounted */
	chdir("/");

	clock_gettime(CLOCK_MONOTONIC, &start);
	clock_gettime(CLOCK_MONOTONIC, &now);

	struct __async_waiting_data waiting_data;
	waiting_data.waiting = &waiting_;
	waiting_data.dpy = dpy;
	waiting_data.win = &win;
	waiting_data.data = data;

	pthread_t tid;
	pthread_create(&tid, NULL, &wait_for_clipboard_request, &waiting_data);

	/* TODO hook up signal handler to change waiting_ */
	while (waiting_) {
		if (diff(&start, &now) > *seconds) {
			pthread_cancel(tid);
			waiting_ = 0;
		}

		clock_gettime(CLOCK_MONOTONIC, &now);
	}

	exit(EXIT_SUCCESS);
}

int copyCmd_parse(int argc, char** argv)
{
	int ret = 0, remaining_args = 0, c, backup;
	struct option long_options[] = {
		{"time",	optional_argument,	NULL,	't'},
	};

	while ((c = getopt_long(argc, argv, "t::", long_options, 0)) != -1) {
		switch (c) {
		case 't':
			seconds_ = optarg ? atoi(optarg) : 0;
			break;
		}
	}

	remaining_args = argc - optind;
	if (remaining_args)
		credential_id = atoi(argv[optind]);

fail:
	return ret;
}

int copyCmd_execute(void)
{
	int ret = 0;
	sqlite3 *handle;
	char *sql;

	ret = safeword_db_open(&handle);
	if (ret)
		goto fail;

	sql = calloc(256, sizeof(char));
	if (!sql) {
		ret = -ENOMEM;
		goto fail;
	}
	sprintf(sql, "SELECT p.password FROM passwords AS p "
		"INNER JOIN credentials AS c "
		"ON (c.passwordid = p.id) WHERE c.passwordid = %d;", credential_id);
	ret = sqlite3_exec(handle, sql, copy_credential_callback, &seconds_, 0);
	free(sql);

	sqlite3_close(handle);

fail:
	return ret;
}
