/*
 * A triv program to parse a mail and accept simple commands.
 *
 * Date         Who     What
 * -------      ----    ----------------------------------------
 * 19 Jul 93    MarkD   Initial coding.
 * 20 Jul 93    MarkD   Incorporated AHerb's marvellous changes
 *
 * Mail comes in on stdin.
 *
 * Valid commands are:
 *
 *      site            name            passwd
 *      include         <group-pattern>
 *      exclude         <group-pattern>
 *      list
 *      newsgroups      <group-pattern>
 *      delete          <group-pattern>
 *      quit
 *
 * Case is irrelevant.
 * group can be a wildmat pattern.
 *
 * If any command has changed the current grouplists for a given site
 * then the list is pushed through prune.c which does a fairly thorough
 * test to ensure that the list makes sense. Anything non-sensical is
 * removed.
 *
 * The delete group is checked against the currently subscribed list
 * rather than the active.
 */

#define	MAIN

#include "gup.h"
#include "rfc822.h"


const char *progname = "gup";	/* GC */

static const char *usage =
"\n\
Usage: %s\t[-hvP] -a active_path [-d home_directory] [-l log_path]\n\
\t\t[-m reply_headers] [-n newsgroups_path] [-s sites_directory]\n\
[-M Mail_command] [-G maxgroups]\n\
\n\
Group Update Program: mail-server for remote newfeed changes.\n\
\n\
-a\tThe news system's 'active' file - used to validate newsgroups\n\
-d\tgup's home directory (default: .)\n\
-h\tPrint usage then exit\n\
-l\tLog file for writing significant events (default: ./log)\n\
-m\tfile containing rfc822 headers to send to the recipient\n\n\
-n\tThe newsgroups file - used for newsgroups descriptions\n\
-M\tResultant mail piped to this command\n\
-P\tDo not prune newsgroup selections (prune is CP intensive)\n\
-s\tLocation of the site directories (default: ./sites)\n\
-v\tPrint version and build options, then exit.\n\
-G\tMaximum number of subscriptions allowed\n\
\n";


typedef struct command_s COMMAND;

struct command_s {
    const char *name;
    int (*handler) (const char **);	/* function to handle it */
    int args;
    int needs_site;
    int sets_quit;
};


static void parse_options(int argc, char **argv);
static void parse_headers(void);
static int parse_commands(void);
static void load_active(LIST * exclusion_list);
static void write_groups(void);
static void print_version(void);
static void log_mail_headers(void);
static void item_print(int indent, int *column, const char *str);

static int getoneline(FILE * fp);
static int tokenize(char *cp, const char **tokens, int max_tokens);
static int command_cmp(const char *str, const char *cmd);
static int site(const char **tokens);
static int include(const char **tokens);
static int exclude(const char **tokens);
static int insert_group(int not_flag, const char *gname);
static int delete(const char **tokens);
static int list(const char **tokens);
static int quit(const char **tokens);
static int newsgroups(const char **tokens);


/* Here's the command list from the body of the mail */
static COMMAND C_list[] =
{
    {"inc*lude", include, 2, TRUE, FALSE},
    {"+", include, 2, TRUE, FALSE},
    {"exc*lude", exclude, 2, TRUE, FALSE},
    {"-", exclude, 2, TRUE, FALSE},
    {"l*ist", list, 0, TRUE, FALSE},
    {"he*lp", help, 0, FALSE, TRUE},
    {"del*ete", delete, 2, TRUE, FALSE},
    {"s*ite", site, 3, FALSE, FALSE},
    {"o*pen", site, 3, FALSE, FALSE},
    {"host", site, 3, FALSE, FALSE},
    {"news*groups", newsgroups, 2, TRUE, FALSE},
    {"q*uit", quit, 0, TRUE, TRUE},
    {"--", quit, 0, TRUE, TRUE},
    {"c*lose", quit, 0, TRUE, TRUE},
    {NULL}
};


static int seen_site = FALSE;	/* If site command accepted */
static int do_prune = TRUE;
static char *sitename;

static const char *site_directory = SITE_DIRECTORY;

static char curr_line[MAX_LINE_SIZE];

static int quit_flag;

/* Variables used to control mailed results */
static char *h_from = NULL;
static char *h_reply_to = NULL;
static char *h_date = NULL;
static char *h_subject = NULL;
static char *h_message_id = NULL;
static char *h_return_path = NULL;

/*
 * The major note for this program is that calls to logit() should be
 * avoided as much as possible prior to the 'site' command. The reason
 * for this is that we have a "less-than-ideal" mail recipient before
 * that time.
 *
 * You see the mail recipient starts out as the backstop mailid
 * (typically news or root or some local admin name) as the inbound
 * headers are cracked this changes to the FROM: address then the
 * REPLY-TO: address. However, it's not until a valid site command is
 * parsed do we know the mailid of the 'defined' admin.
 *
 * So, the net effect is that to earlier you put in logging calls the
 * more change you have of irritating the person on the end of the
 * admin address un-necessarily ;>
 */

int main(int argc, char **argv)
{
    int changed;

    umask(UMASK);

    parse_options(argc, argv);	/* Decode our command line options */

    quit_flag = FALSE;
    parse_headers();		/* Crack the inbound mail headers */
    changed = parse_commands();	/* Process the body of the mail */

    if (!seen_site)
	gupout(0, "No 'site' command found");

    if (changed) {		/* Only sort & prune if changed */
	group_list = sort_groups(group_list);
	if (do_prune) {
	    logit(L_MAIL, "");
	    logit(L_BOTH | L_TIMESTAMP, "Checking new group list");
	    prune(active_list, group_list);
	}
	logit(L_BOTH | L_TIMESTAMP, "Writing updated group list");
	write_groups();
    }
    gupout(0, NULL);
    /* NOTREACHED */
}

/* Parse the command line options */
static void parse_options(int argc, char **argv)
{
    int cc;

    active_path = ACTIVE_PATH;
    newsgroups_path = NEWSGROUPS_PATH;
    mail_fp = mail_open(FALSE, BACKSTOP_MAILID, MAIL_COMMAND, NULL);
    maxgroups = 32767;

    while ((cc = getopt(argc, argv, "a:d:hs:l:n:m:M:PvG:")) != -1) {
	switch (cc) {
	case 'a':
	    active_path = optarg;
	    break;
	case 'd':
	    if (optarg)
		if (chdir(optarg))
		    gupout(1, "Could not chdir to %s", optarg);
	    break;
	case 'h':
	    fprintf(stderr, usage, progname);
	    exit(0);
	case 's':
	    site_directory = optarg;
	    break;
	case 'l':
	    if (optarg) {
		if (!(log_fp = fopen(optarg, "a")))
		    gupout(1, "%s: Could not open log file: %s\n",
			    progname, optarg);
		if (dup2(fileno(log_fp), fileno(stderr)) < 0)
		    gupout(1, "dup2() failed");
	    }
	    break;
	case 'm':
	    mail_fp = mail_open(FALSE, NULL, NULL, optarg);
	    break;
	case 'M':
	    mail_fp = mail_open(FALSE, NULL, optarg, NULL);
	    break;
	case 'P':
	    do_prune = FALSE;
	    break;
	case 'n':
	    newsgroups_path = optarg;
	    break;
	case 'v':
	    print_version();
	    exit(0);
	case 'G':
	    maxgroups = atoi(optarg);
	    break;
	default:
	    gupout(1, "Installation problem. Invalid option used");
	}
    }


    /* Must have an active file that can be opened */
    if (!active_path || !*active_path) {
	fprintf(stderr, "%s: Must nominate the active file with -a\n",
		progname);
	fprintf(stderr, usage, progname);
	gupout(1, NULL);
    }
}

static void print_version(void)
{
    fprintf(stderr, "%s: Version: %s.\n", progname, VERSION);
    fprintf(stderr, "CONFIG_FILENAME:  '%s'\n", CONFIG_FILENAME);
    fprintf(stderr, "ACTIVE_PATH:      '%s'\n", ACTIVE_PATH);
    fprintf(stderr, "NEWSGROUPS_PATH:  '%s'\n", NEWSGROUPS_PATH);
    fprintf(stderr, "MAIL_COMMAND:     '%s'\n", MAIL_COMMAND);
    fprintf(stderr, "BACKSTOP_MAILID:  '%s'\n", BACKSTOP_MAILID);
    fprintf(stderr, "UMASK:            %03o\n", UMASK);
#ifdef USE_FLOCK
    fprintf(stderr, "LOCKING uses:     flock(2)\n");
#endif
#ifdef USE_DOTLOCK
    fprintf(stderr, "LOCKING uses:     dot locking\n");
#endif
}

/*
 * Parse the mail headers and note the interesting ones. Note that we
 * stash all of the interesting headers for later logging and use.
 * Note that this routine avoids calling logit() because we are so
 * early on in the process that our mailid is bound to be the backstop
 * mailid.
 */
static void parse_headers(void)
{
    while (getoneline(stdin)) {
	if (!curr_line[0])
	    return;

	if (strncasecmp(curr_line, "FROM:", 5) == 0) {
	    char buf1[LG_SIZE], buf2[LG_SIZE];

	    CrackFrom(buf1, buf2, curr_line + 5);
	    h_from = xstrdup(buf1);
	    if (!h_reply_to)
		mail_fp = mail_open(FALSE, h_from, NULL, NULL);
	    continue;
	}
	if (strncasecmp(curr_line, "REPLY-TO:", 9) == 0) {
	    char buf1[LG_SIZE], buf2[LG_SIZE];

	    CrackFrom(buf1, buf2, curr_line + 9);
	    h_reply_to = xstrdup(buf1);
	    mail_fp = mail_open(FALSE, h_reply_to, NULL, NULL);
	    continue;
	}
	if (strncasecmp(curr_line, "DATE:", 5) == 0) {
	    h_date = xstrdup(curr_line);
	    continue;
	}
	if (strncasecmp(curr_line, "SUBJECT:", 8) == 0) {
	    h_subject = xstrdup(curr_line);
	    continue;
	}
	if (strncasecmp(curr_line, "MESSAGE-ID:", 11) == 0) {
	    h_message_id = xstrdup(curr_line);
	    continue;
	}
	if (strncasecmp(curr_line, "RETURN-PATH:", 12) == 0) {
	    h_return_path = xstrdup(curr_line);
	    continue;
	}
    }
}

/* Process the commands in the body of the mail */

static int parse_commands(void)
{
    const char *command;
    COMMAND *cmdp;
    const char *tokens[10];
    int tcount;
    int changed;

    changed = FALSE;

    while (!quit_flag && getoneline(stdin)) {
    /* It's safe to log commands after we've seen the site as mail is right */
	if (!*curr_line)
	    continue;

	if (seen_site) {
	    logit(L_MAIL, "");
	    logit(L_BOTH, "%s", curr_line);
	}
	tcount = tokenize(curr_line, tokens, 10);
	if (tcount == 0)
	    continue;

	/* Search for a matching command */
	command = tokens[0];
	for (cmdp = C_list; cmdp->name; cmdp++)
	    if (command_cmp(command, cmdp->name) == 0)
		break;

	/* Find it? */
	if (!cmdp->name) {	/* No */
	    if (!seen_site)	/* What a hack - sigh */
		logit(L_BOTH, "\n%s %s %s", tokens[0],tokens[1],tokens[2]);
	    logit(L_BOTH, "\nERROR: Unrecognized command '%s' - try 'help'\n"
		    "NOTE: No changes made to subscription list.", command);
	    gupout(0, NULL);
	}

	/* Does this command require a preceeding site command? */
	if (cmdp->needs_site && !seen_site)
	    gupout(0, "'site' command must preceed the '%s' command",
		    cmdp->name);

	/* Only check # of args if defined as > 0 */
	if ((cmdp->args > 0) && (tcount != cmdp->args)) {
	    logit(L_BOTH, "WARNING: %s requires %d parameters, not %d\n",
		    command, cmdp->args, tcount);
	    continue;
	}

	/* Dispatch to handler */
	if (cmdp->handler(tokens + 1))
	    changed = TRUE;

	/* Auto quit? */
	if (cmdp->sets_quit)
	    quit_flag = TRUE;
    }

    return changed;
}

/*      site    sitename        passwd */
static int site(const char **tokens)
{
    FILE *fp;
    int fd;
    const char *hf_tokens[4];
    int found_site;
    int tcount;
    LIST *exclusion_list = NULL;
    char lbuf[MAX_LINE_SIZE];
    char *cryptstr;

    if (seen_site)
	gupout(0, "Multiple 'site' commands not allowed");

    /* Search the config file for a valid sitename */
    if (!(fp = fopen(CONFIG_FILENAME, "r")))
	gupout(1, "Install error. Open of '%s' failed", CONFIG_FILENAME);

    /* Search for matching site */
    found_site = FALSE;
    while (fgets(lbuf, sizeof(lbuf), fp)) {
	tcount = tokenize(lbuf, hf_tokens, 4);
	if (tcount == 0)
	    continue;
	if (*hf_tokens[0] == '#')
	    continue;		/* skip comments -- Md */
	if (tcount != 3)
	    gupout(1, "%s format incorrect for %s. Need 3 tokens",
		    CONFIG_FILENAME, tokens[0]);
	if (strcasecmp(tokens[0], hf_tokens[0]) == 0) {
	    found_site = TRUE;
	    break;
	}
    }

    fclose(fp);

    if (!found_site)
	gupout(0, "Unknown site '%s'", tokens[0]);

/*
 * Ok. We now have the *correct* admin mail ID. Start the mail output.
 * We set up even before completing the site command so that password
 * cracking attempts go to the site admin, not the sender.
 */
    mail_fp = mail_open(TRUE, hf_tokens[2], NULL, NULL);

/*
 * Right.  Now mail_fp is guaranteed to be open, and stdout points there as
 * well.  Let's catch up on all that good log stuff we want to tell them.
 */
    logit(L_LOG, "");
    logit(L_BOTH | L_TIMESTAMP, "BEGIN: %s %s\n", progname, VERSION);

    log_mail_headers();

    logit(L_BOTH, "site %s", tokens[0]);

    /* Found the site how does the password look? */
    cryptstr = crypt(tokens[1], hf_tokens[1]);

    if (strcmp(cryptstr, hf_tokens[1]) != 0		/* crypted pwd */
#ifdef CLEARTEXT_PWD
	&& strcmp(tokens[1], hf_tokens[1]) != 0)	/* cleartext pwd */
#endif
	gupout(0, "Password incorrect");

    sitename = xstrdup(hf_tokens[0]);
    seen_site = TRUE;

    /* It's all good, accept this site as the directory name and cd to it */
    while (chdir(site_directory))
	if (errno == ENOENT) {
	    logit(L_LOG, "MKDIR: %s", site_directory);
	    if (mkdir(site_directory, 0777))
		gupout(1, "Install error: could not MKDIR %s (%s)",
			site_directory, strerror(errno));
	} else
	    gupout(1, "Install error: could not CHDIR to %s (%s)",
		    site_directory, strerror(errno));

    while (chdir(sitename))
	if (errno == ENOENT) {
	    logit(L_LOG, "MKDIR: %s", sitename);
	    if (mkdir(sitename, 0777))
		gupout(1, "Install error: could not MKDIR %s (%s)",
			sitename, strerror(errno));
	} else
	    gupout(1, "Install error: could not CHDIR to %s (%s)",
		    sitename, strerror(errno));

    if (!lockit())
	gupout(1, "Failed to take lock in %s", lbuf);
    if ((fd = open(EXCLUSIONS_FILENAME, O_RDONLY)) >= 0) {
	exclusion_list = read_groups(fd, (LIST *) NULL);
	close(fd);
    }
    load_active(exclusion_list);

    /* Open the sites' groups file - it's ok if it's not there btw */
    if ((fd = open(BODY_FILENAME, O_RDONLY)) >= 0) {
	group_list = read_groups(fd, (LIST *) NULL);
	close(fd);
    } else {
	group_list = create_list();	/* we need something... */
	logit(L_LOG, "WARNING: File '%s' not found for %s", BODY_FILENAME,
		sitename);
    }

    return FALSE;
}

static int quit(const char **tokens)
{
    return 0;
}

/* Check and load the active file */
static void load_active(LIST *exclusion_list)
{
    int fd;

    if ((fd = open(active_path, O_RDONLY)) < 0)
	gupout(1, "Could not open active file '%s'", active_path);
    active_list = read_groups(fd, exclusion_list);
    close(fd);

    if (!active_list->length)
	gupout(1, "No groups found in active file '%s'", active_path);
}

/*
 * Writes the new group list back to disk moderately safely.
 */
static void write_groups(void)
{
    FILE *fp;
    GROUP *gp;
    int write_count;

    if (!(fp = fopen(NEWBODY_FILENAME, "w")))
	gupout(1, "Could not open %s for writing", NEWBODY_FILENAME);
    write_count = 0;
    TRAVERSE(group_list, gp) {
	write_count++;
	if (gp->u.not)
	    putc('!', fp);
	fputs(gp->name, fp);
	putc('\n', fp);
    }

    fclose(fp);

    logit(L_BOTH | L_TIMESTAMP, "%s: %s: group patterns: %d",
	    progname, sitename, write_count);

/*
 * In the directory is:       groups, groups.new and groups.old
 *
 * rename(groups,old)
 * rename(new,groups)
 */

    if ((rename(BODY_FILENAME, OLDBODY_FILENAME) == -1) && (errno != ENOENT))
	gupout(1, "rename(%s,%s) failed for %s. Errno=%d",
		BODY_FILENAME, OLDBODY_FILENAME, sitename, errno);
    if (rename(NEWBODY_FILENAME, BODY_FILENAME) == -1)
	gupout(1, "rename(%s,%s) failed for %s. Errno=%d",
		NEWBODY_FILENAME, BODY_FILENAME, sitename, errno);
}

/* Include a pattern into the site's group list */
static int include(const char **tokens)
{
    return insert_group(FALSE, tokens[0]);
}

static int exclude(const char **tokens)
{
    return insert_group(TRUE, tokens[0]);
}

static int insert_group(int not_flag, const char *gname)
{
    GROUP *gp, *new_gp;
    int match_count;
    int column, indent;

    /* Be optimistic, create the group now */
    new_gp = create_group(not_flag, gname);

    /* See if the add pattern matches anything in active */
    fputs("  => ", mail_fp);
    column = indent = 5;

    match_count = 0;
    TRAVERSE(active_list, gp) {
	if (wildmat(gp->name, new_gp->name)) {
	    match_count++;
	    if (match_count <= LOG_MATCH_LIMIT) {
		item_print(indent, &column, gp->name);
		if (match_count == LOG_MATCH_LIMIT)
		    item_print(indent, &column, "...etc...");
	    }
	}
    }

    if (match_count == 0) {
	destroy_group(new_gp);
	logit(L_MAIL, "WARNING: %s pattern not in active - ignored", gname);
	return FALSE;
    }
    logit(L_MAIL, "");
    if (match_count > 1)
	logit(L_MAIL, "  [ %d groups ]", match_count);

    /* Link the new add pattern into the end */
    add_group(group_list, new_gp);

    return TRUE;
}

/* Delete matches on the group list, not the active list */
static int delete(const char **tokens)
{
    const char *gname;
    GROUP *gp;
    int match_count;

    gname = tokens[0];

    /* See if the delete pattern matches anything in the current groups */
    match_count = 0;
    TRAVERSE(group_list, gp) {
	if (wildmat(gp->name, gname)) {
	    match_count++;
	    if (match_count <= LOG_MATCH_LIMIT)
		logit(L_MAIL, "\tMatches: %s %s",
			gp->u.not ? "exclude" : "include", gp->name);
	    if (match_count == LOG_MATCH_LIMIT)
		logit(L_MAIL, "\tMatches: ...etc...");
	    remove_group(group_list, gp);
	}
    }

    logit(L_BOTH, " delete: Match count %d", match_count);
    if (match_count == 0)
	logit(L_MAIL, "WARNING: %s pattern not in %s - ignored",
		gname, BODY_FILENAME);
    return (match_count > 0);
}

static void item_print(int indent, int *column, const char *str)
{
#define MAX_COL	77	/* number of columns in a line, with room for a ", " */
    int len = strlen(str);

    if (*column + len > MAX_COL) {
	int col = indent;
	char ch;

	putc(',', mail_fp);
	putc('\n', mail_fp);

	if (col % 8)
	    ch = ' ';
	else {
	    ch = '\t';
	    col /= 8;
	}
	while (col--)
	    putc(ch, mail_fp);

	*column = indent;
    }
    if (*column != indent) {
	putc(',', mail_fp);
	putc(' ', mail_fp);
    }
    fputs(str, mail_fp);

    *column += len + 2;
}

/* List out the currently selected groups */
static int list(const char **tokens)
{
    GROUP *gp;

    group_list = sort_groups(group_list);
    TRAVERSE(group_list, gp) {
	logit(L_MAIL, "  %s  %s\n", gp->u.not ? "  exclude" : "include",
		gp->name);
    }
    return 0;
}

/* List out the active file entries that match the pattern */
static int newsgroups(const char **tokens)
{
    print_newsgroups(mail_fp, tokens[0]);
    return 0;
}

/*
 * Read the next line into curr_line. Trim trailing whitespace.
 * Return TRUE if a line was read, otherwise return FALSE.
 */
static int getoneline(FILE *fp)
{
    int len;

    if (!fgets(curr_line, sizeof(curr_line) - 1, fp)) {
	quit_flag = TRUE;
	return FALSE;
    }
    curr_line[sizeof(curr_line) - 1] = '\0';

    /* Trim trailing whitespace */
    len = strlen(curr_line);
    while ((len > 0) && isspace(curr_line[len - 1]))
	len--;
    curr_line[len] = '\0';

    return TRUE;
}

/*
 * A simple whitespace tokenizer - no continutation, no quotes, no
 * nuthin' Does accept comments '#' and empty lines.
 */
static int tokenize(char *cp, const char **tokens, int max_tokens)
{
    int tix;
    char *com_cp;

    /* Find comment */
    for (com_cp = cp; *com_cp; com_cp++)
	if (*com_cp == '#') {
	    *com_cp = '\0';
	    break;
	}

    /* Why don't I use scanf? Good question... */
    for (tix = 0; tix < max_tokens; tix++)
	tokens[tix] = "";

    for (tix = 0; tix < max_tokens; tix++) {
	while (*cp && isspace(*cp))
	    cp++;		/* Skip leading whitespace */
	tokens[tix] = cp;

	while (*cp && !isspace(*cp))
	    cp++;		/* Skip data */

	if (cp == tokens[tix])
	    break;		/* End of data stream */
	if (*cp)
	    *cp++ = '\0';
    }

    return tix;
}

static int command_cmp(const char *str, const char *cmd)
{
    int base = 0;		/* have we matched the base of the cmd? */

    while (*str && *cmd) {
	if (tolower(*str) != *cmd)
	    return TRUE;	/* match failed */
	str++;
	cmd++;
	if (*cmd == '*') {
	    base = TRUE;
	    cmd++;
	}
    }
    return !(!*str && (base || !*cmd));
}

void gupout(int val, const char *out_msg, ...)
{
    va_list ap;

    unlockit();			/* release our lock */

    if (out_msg) {
	va_start(ap, out_msg);
	vlogit(L_BOTH, "ERROR", out_msg, ap);
	va_end(ap);
    }

    log_mail_headers();
    logit(L_BOTH, "");
    logit(L_BOTH | L_TIMESTAMP, "END: %s", progname);

    if (log_fp)
	fclose(log_fp);
    if (mail_fp)
	mail_close(mail_fp);

    /* keep the MTA happy - "don't worry, it's all under control  - honest!" */
/*   exit(val) */

    /* but only if it's not a system error -- Md*/
    if (val > 0)
	exit(75);		/* EX_TEMPFAIL */
    else
	exit(0);
}

/*
 * The mail headers are stashed up by the parse because we don't really
 * want to log them until we absolutely have to. This routine logs them
 * if they haven't already been logged. I don't know that freeing is
 * doing anything much for anyone, but...
 */
static void log_mail_headers(void)
{
    int log_count = 0;

    if (h_from) {
	logit(L_BOTH, "Mail Header: FROM: %s", h_from);
	free(h_from);
	h_from = NULL;
	log_count++;
    }
    if (h_reply_to) {
	logit(L_BOTH, "Mail Header: REPLY-TO: %s", h_reply_to);
	free(h_reply_to);
	h_reply_to = NULL;
	log_count++;
    }
    if (h_subject) {
	logit(L_BOTH, "Mail Header: %s", h_subject);
	free(h_subject);
	h_subject = NULL;
	log_count++;
    }
    if (h_date) {
	logit(L_BOTH, "Mail Header: %s", h_date);
	free(h_date);
	h_date = NULL;
	log_count++;
    }
    if (h_message_id) {
	logit(L_BOTH, "Mail Header: %s", h_message_id);
	free(h_message_id);
	h_message_id = NULL;
	log_count++;
    }
    if (h_return_path) {
	logit(L_BOTH, "Mail Header: %s", h_return_path);
	free(h_return_path);
	h_return_path = NULL;
	log_count++;
    }
    if (log_count)
	logit(L_BOTH, "");
}
