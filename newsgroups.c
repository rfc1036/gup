#include "gup.h"


static int newsgroups_loaded = FALSE;


static void load_newsgroups(void);

/* List out the active file entries that match the pattern */

extern void print_newsgroups(FILE *fp, const char *gname)
{
    GROUP *gp;
    int glen;
    int subflag;
    int gcount = 0;
    int subcount = 0;

    if (!newsgroups_loaded)
	load_newsgroups();

/* Print all matching active entries */

    fprintf(fp, "\n%-*s Sub Description\n",
	    GROUP_OVERFLOW_LP, "Name");
    fprintf(fp, "%-*s --- ----------------------------------------\n",
	    GROUP_OVERFLOW_LP, "---------------");

    TRAVERSE(active_list, gp) {
	if (wildmat(gp->name, gname)) {
	    gcount++;
	    subflag = subscribed(group_list, gp->name);
	    if (subflag)
		subcount++;

	    fputs(gp->name, fp);

/*
 * Try and fit the description nicely on the current line.
 * Either:      groupname       DESC
 * or:          very-long-group-name
 *                              DESC
 */

	    glen = strlen(gp->name);
	    if (glen > GROUP_OVERFLOW_LP) {
		glen = 0;
		putc('\n', fp);
	    }
	    glen = (GROUP_OVERFLOW_LP - glen) + 1;
	    while (glen-- > 0)
		putc(' ', fp);

	    fprintf(fp, "%s %s\n", subflag ? "Yes" : "No ", gp->desc);
	}
    }
    fprintf(fp, "%-*s --- ----------------------------------------\n",
	    GROUP_OVERFLOW_LP, "---------------");
    fprintf(fp, "\nGroups listed: %d, of which you subscribe to %d.\n\n",
	    gcount, subcount);
}


/* Try and load the newsgroups file and add descriptions to the active */

static void load_newsgroups(void)
{
#ifdef STDIO_LOADFILE
    FILE *fp;
    char lbuf[MAX_LINE_SIZE];
    char *cp;
    char *name;
#else
    int fd;
    struct stat sb;
    char *desc;
    int length;
#endif
    GROUP *gp = NULL;

    newsgroups_loaded = TRUE;
    if (!newsgroups_path || !*newsgroups_path) {
	logit(L_BOTH, "WARNING",
	      "No newsgroups file defined - needed for descriptions");
	return;
    }
#ifdef STDIO_LOADFILE
    if (!(fp = fopen(newsgroups_path, "r"))) {
	sprintf(msg, "Could not open newsgroups file '%s'",
		newsgroups_path);
	logit(L_BOTH, "WARNING", msg);
	return;
    }
    while (fgets(lbuf, sizeof(lbuf) - 1, fp)) {		/* Load them all up */
	lbuf[strlen(lbuf) - 1] = '\0';	/* Zip trailing \n */

/* Crack it into a name and a description */

	for (name = lbuf; *name; name++) {	/* Ignore leading spaces */
	    if (!isspace(*name))
		break;
	}

	for (cp = name; *cp; cp++) {
	    if (isspace(*cp)) {
		*cp++ = '\0';
		break;
	    }
	}

	while (*cp && isspace(*cp))
	    cp++;

/*
 * name points to name and cp points to the description. Find the matching
 * active entry. Gen a warning if not found - useful for annoying the 
 * news admins.
 */
	TRAVERSE(active_list, gp) {
	    if ((strcasecmp(gp->name, name) == 0) && !*gp->desc) {
		gp->desc = xstrdup(cp);
		break;
	    }
	}
    }

    fclose(fp);
#else
    if ((fd = open(newsgroups_path, O_RDONLY)) < 0) {
	sprintf(msg, "Could not open newsgroups file '%s'",
		newsgroups_path);
	logit(L_BOTH, "WARNING", msg);
	return;
    }
    /* see how big it is */
    if (fstat(fd, &sb) < 0) {
	sprintf(msg, "Could not stat newsgroups file '%s'",
		newsgroups_path);
	logit(L_BOTH, "WARNING", msg);
	return;
    }
    /* grab ourselves a buffer */
    desc = malloc(sb.st_size + 1);
    if (!desc) {
	logit(L_BOTH, "WARNING", "Could not malloc space for newsgroups");
	return;
    }
    /* slurp it in */
    length = read(fd, desc, (int) sb.st_size);
    if (length != sb.st_size) {
	logit(L_BOTH, "WARNING", "Error reading newsgroups");
	return;
    }
    /* terminate the end */
    desc[length] = 0;

    /*
     * hurl through the newsgroups, assigning descriptions to active_list
     * groups as we go
     */
    while (*desc) {
	char *eoln, *p;

	/*
	 * terminate the end of the description, to prevent anything
	 * screaming off through the entire file
	 */
	if ((eoln = strchr(desc, '\n')))
	    *eoln = 0;
	else
	    logit(L_LOG, "WARNING", "premature end of newsgroups file");

	/* locate end of group name */
	if ((p = strchr(desc, '\t'))) {
	    /* found it */
	    *p = 0;

	    /* gobble whitespace */
	    while (isspace(*++p));

	    /* check whether there's still a description to add */
	    if (*p) {
		/*
		 * check if we know about it; first try the current group we're
		 * up to, to make it fast for sorted active & newsgroups
		 */
		if (gp && !strcmp(gp->name, desc)) {
		    /* add description */
		    gp->desc = p;
		} else {
		    TRAVERSE(active_list, gp) {
			if (!strcmp(gp->name, desc) && !*gp->desc) {
			    /* OK, we've found it */
			    gp->desc = p;

			    /* jump to the next group for next time around */
			    NEXT(active_list, gp);
			    break;
			}
		    }
		}
	    }
	}
	/* advance to next line */
	desc = eoln + 1;
    }
#endif
}
