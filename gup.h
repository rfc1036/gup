#ifdef linux
#define _GNU_SOURCE
#else
#define _XOPEN_SOURCE
#define _BSD_SOURCE
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>		/* strftime */
#include <unistd.h>		/* Flock flags */
#include <ctype.h>		/* isspace */
#include <errno.h>		/* Needed for errno */
#include <stdarg.h>

#include <sys/file.h>
#include <sys/stat.h>		/* umask */

#include "config.h"


#define	VERSION			"0.5.7"


#define	TRUE		1
#define	FALSE		0

#define	GROUP_OVERFLOW_LP	25	/* Point at which newsgroup newlines */

#define	MAX_LINE_SIZE		512
#define	MODEST_SIZE		128

#ifdef FILE_LOCK
#define LOCK_SLEEP		10	/* time to sleep between lock attempts*/
#define MAX_LOCK_SLEEP_COUNT	10	/* maximum number of tries */
#endif

/* Logit flags */
#define	L_LOG		(1<<0)
#define	L_MAIL		(1<<1)
#define	L_BOTH		(L_LOG | L_MAIL)
#define	L_TIMESTAMP	(1<<2)


typedef struct group_s GROUP;

typedef struct tag_s TAG;

struct tag_s {
    TAG *next;
    GROUP *group;
};

struct group_s {
    GROUP *next;
    const char *name;
    const char *desc;
    int order;			/* Needed for sorting */
    union {
	int not;
	TAG *tag;
    } u;
};

typedef struct {
    GROUP *head;
    GROUP *tail;
    unsigned length;
} LIST;

#define TRAVERSE(list, elt) \
	if (list) \
	    for (elt = list->head; elt; elt = elt->next)

#define NEXT(list, elt) \
	elt = elt->next

#define LIST_LENGTH(list) \
	(list->length)

#ifdef __GNUC__
#define __NORETURN __attribute__((__noreturn__))
#else
#define __NORETURN
#endif

/* Protos */
extern int wildmat(const char *, const char *);
extern void logit(int, const char *, ...);
extern void vlogit(int, const char *, const char *, va_list AP);
extern void __NORETURN gupout(int, const char *, ...);
extern void prune(LIST *, LIST *);
extern int help(const char **);
extern void print_newsgroups(FILE *, const char *);

/* lock.c */
extern int lockit(void);
extern void unlockit(void);

/* misc.c */
extern LIST *read_groups(int, LIST *);
extern int subscribed(LIST *, const char *);
extern LIST *create_list(void);
extern void add_group(LIST *, GROUP *);
extern void remove_group(LIST *, GROUP *);
extern GROUP *create_group(int, const char *);
extern void destroy_group(GROUP *);
extern char *xstrdup(const char *);

/* mail.c */
extern FILE *mail_open(int, const char *, const char *, const char *);
extern void mail_close(FILE *);

/* sort.c */
extern LIST *sort_groups(LIST *);

/* Define external variables */
#ifdef MAIN
#define	EXTERN
#else
#define	EXTERN	extern
#endif

extern const char *progname;

EXTERN FILE *log_fp;
EXTERN FILE *mail_fp;

EXTERN const char *active_path;
EXTERN const char *newsgroups_path;

EXTERN LIST *active_list;
EXTERN LIST *group_list;

EXTERN int maxgroups;

