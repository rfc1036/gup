#include "gup.h"

/*
 * Read the group file into memory.  This routine serves a dual purpose
 * of reading the active file too!  An exclusion list can be specified
 * when reading the active file...
 */
LIST *read_groups(int fd, LIST *exclusion_list)
{
    LIST *list;
    GROUP *gp;
    char *name;
    int not;
    int exclude;
#ifdef STDIO_READGROUP
    char *cp;
    char lbuf[MAX_LINE_SIZE];
#else
    struct stat sb;
    int length;
#endif

    list = create_list();

#ifdef STDIO_READGROUP
    while (fgets(lbuf, sizeof(lbuf) - 1, fp)) {		/* Load them all up */
	lbuf[strlen(lbuf) - 1] = '\0';	/* Zip trailing \n */

	/* Ignore leading and trailing spaces */
	for (name = lbuf; *name; name++)
	    if (!isspace(*name))
		break;

	for (cp = name; *cp; cp++)
	    if (isspace(*cp)) {
		*cp++ = '\0';
		break;
	    }

	not = (*name == '!');
	if (not)
	    name++;

	/* check that this group is not on the site's exclusion list */
	exclude = FALSE;
	TRAVERSE(exclusion_list, gp) {
	    if (wildmat(name, gp->name))
		exclude = !gp->u.not;
	}

	if (!exclude) {
	    gp = create_group(not, name);
	    add_group(list, gp);	/* Link into list */
	}
    }
#else
    if (fstat(fd, &sb) < 0) {
	logit(L_BOTH, "ERROR: read_groups: could not stat file (%s)",
		strerror(errno));
	return list;
    }
    /* grab ourselves a buffer */
    name = malloc(sb.st_size + 1);
    if (!name)
	gupout(1, "Could not malloc space for groups list");

    /* slurp it in */
    length = read(fd, name, (int) sb.st_size);
    if (length != sb.st_size)
	gupout(1, "Error reading groups file");

    /* terminate the end */
    name[length] = 0;

    /*
     * hurl through the newsgroups, assigning descriptions to active_list
     * groups as we go
     */
    while (*name) {
	char *eoln, *p;

	/*
	 * null-terminate the end of the line, to prevent anything
	 * screaming off through the entire file
	 */
	if ((eoln = strchr(name, '\n')))
	    *eoln = 0;
	else
	    logit(L_LOG, "WARNING: premature end of groups file");

	/* locate end of group name */
	if ((p = strchr(name, ' ')))
	    *p = 0;			/* found it */
	if ((not = (*name == '!')))
	    name++;

	/* check that this group is not on the site's exclusion list */
	exclude = FALSE;
	TRAVERSE(exclusion_list, gp) {
	    if (wildmat(name, gp->name))
		exclude = !gp->u.not;
	}

	if (!exclude) {
	    gp = create_group(not, name);
	    add_group(list, gp);	/* Link into list */
	}
	/* advance to next line */
	name = eoln + 1;
    }
#endif

    return list;
}

/* Check to see if a specific group is in the subscribed list */
int subscribed(LIST *sub_list, const char *gname)
{
    int unsub = TRUE;
    GROUP *gp;

    TRAVERSE(sub_list, gp) {
	if ((unsub != gp->u.not) && wildmat(gname, gp->name))
	    unsub = gp->u.not;
    }
    return !unsub;
}

LIST *create_list(void)
{
    LIST *l;

    l = malloc(sizeof(LIST));
    if (!l)
	gupout(1, "malloc of LIST failed");
    l->head = l->tail = NULL;
    l->length = 0;
    return l;
}

void add_group(LIST *list, GROUP *group)
{
    if (!list->head)
	list->head = list->tail = group;
    else {
	list->tail->next = group;
	list->tail = group;
    }
    list->length++;
    group->next = NULL;
}

void remove_group(LIST *list, GROUP *group)
{
    GROUP *prev, *tmp;

    prev = NULL;
    for (tmp = list->head; tmp; tmp = tmp->next) {
	if (tmp == group)
	    break;
	prev = tmp;
    }

    if (!tmp) {
	logit(L_BOTH, "ERROR: remove_group can't find group");
	return;
    }
    /* unlink */
    if (!prev)
	list->head = group->next;	/* the first item was deleted */
    else
	prev->next = group->next;

    if (!group->next)
	list->tail = prev;

    list->length--;

    /* don't free() the element, since TRAVERSE() will be doing naughty
       things then (if we're called from within a TRAVERSE, that is) */

    /* if this was a problem, which it isn't, we would put group onto
       a free-list for reclamation */
}

extern GROUP *create_group(int not_flag, const char *name)
{
    GROUP *gp;

    gp = malloc(sizeof(GROUP));
    if (!gp)
	gupout(1, "malloc of GROUP failed");

    gp->next = NULL;
    gp->u.not = not_flag;
    gp->name = xstrdup(name);
    gp->desc = "";

    return gp;
}

void destroy_group(GROUP *gp)
{
    free(gp);
}

char *xstrdup(const char *str)
{
    char *res;

    res = malloc(strlen(str) + 1);
    if (!res)
	gupout(1, "xstrdup of %d failed", strlen(str));
    return strcpy(res, str);
}
