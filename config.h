/* ACTIVE_PATH is the full pathname for your news active file. This
 * file is used to validate newgroup names. */
#define ACTIVE_PATH	"/var/lib/news/active"

/* NEWSGROUPS_PATH is the fill pathname for your newsgroups file. This
 * file is used to provide descriptions for each newsgroup. */
#define NEWSGROUPS_PATH	"/var/lib/news/newsgroups"

/* BACKSTOP_MAILID is used if gup cannot find a FROM: or a REPLY-TO: or
 * a valid site command, this is where the mail goes to. */
#define BACKSTOP_MAILID	"news"

/* MAIL_COMMAND is the mailer that accepts a mail with rfc822 headers and
 * body from stdin. */
#define MAIL_COMMAND	"/usr/sbin/sendmail -t"

/* With a command like "include alt.*" the resultant list is, long.
 * LOG_MATCH_LIMIT, defines the upper bound on the number of pattern
 * matches that will be printed out for the include, exclude and delete
 * commands. */
#define LOG_MATCH_LIMIT	50

/* The UMASK to use when creating files and directories. */
#define UMASK	022

#define	CONFIG_FILENAME		"config"
#define	SITE_DIRECTORY		"sites"
#define	EXCLUSIONS_FILENAME	"exclude"
#define	BODY_FILENAME		"groups"
#define	NEWBODY_FILENAME	"groups.new"
#define	OLDBODY_FILENAME	"groups.old"

/* I'll enable it. I hope it will not break anything. -- Md */
/* No luck, it needs more code. */
/* #define STDIO_READGROUP */

