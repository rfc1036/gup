#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>		/* strftime */
#include <unistd.h>		/* Flock flags */
#include <ctype.h>		/* isspace */
#include <errno.h>		/* Needed for errno */

#include <sys/file.h>
#include <sys/stat.h>		/* umask */

#include "config.h"


#define	VERSION			"0.5"


#define	TRUE		1
#define	FALSE		0

#define	GROUP_OVERFLOW_LP	25	/* Point at which newsgroup newlines */

#define	MAX_LINE_SIZE		512
#define	MODEST_SIZE		128

#define FILE_LOCK		/* this should always be OK */
/*but is not NFS safe! -- Md */

#ifdef FILE_LOCK
#define LOCK_SLEEP		10	/* time to sleep between lock attempts */
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

/* Protos */

extern int wildmat(const char *text, const char *p);
extern void logit(int lflags, const char *prefix, const char *msg);
extern int lockit(void);
extern void unlockit(void);
extern void gupout(int val, const char *msg)
#ifdef __GNUC__
 __attribute__((__noreturn__))
#endif
;
extern void prune(LIST * active_list, LIST * group_list);
extern int help(const char **tokens);
extern void print_newsgroups(FILE *, const char *gname);
extern LIST *read_groups(int fd, LIST * exclusion_list);
extern LIST *create_list(void);
extern GROUP *create_group(int not_flag, const char *name);
extern void destroy_group(GROUP * gp);
extern void add_group(LIST * list, GROUP * group);
extern void remove_group(LIST * list, GROUP * group);

extern char *xstrdup(const char *str);
extern int subscribed(LIST * gp, const char *gname);

/* mail.c */

extern FILE *mail_open(int, const char *, const char *, const char *);
extern void mail_close(FILE *);

/* sort.c */

extern LIST *sort_groups(LIST * list);

/* Define external variables */

#ifdef	MAIN
#define	EXTERN
#else
#define	EXTERN	extern
#endif

extern const char *progname;

EXTERN const char *log_filename;
EXTERN FILE *log_fp;

EXTERN FILE *mail_fp;

EXTERN const char *active_path;
EXTERN const char *newsgroups_path;

EXTERN char msg[MAX_LINE_SIZE * 2];

EXTERN LIST *active_list;
EXTERN LIST *group_list;
