/*
 * Sort a list of wildmat groupname in a sorted ASCII order. Well,
 * it's sortof sorted. The basic idea is that only the characters
 * upto the first wildmat special character are compared.
 *
 * It is *not safe, as you might assume to accept matching wildmat
 * chars and continue. Consider:
 *
 * A.*.A3
 * A.[.A2
 * A.*.A1
 *
 * This routine uses qsort() and does not rely on it being stable
 * (which it's not) to maintain order. Pity really, a stable qsort()
 * would make my life a bit easier. Instead they made qsort()'s life a
 * bit easier - sigh.
 *
 * Note that the inbound list is worthless after this routine.
 */

#include "gup.h"

static int mat_compare(const GROUP **g1, const GROUP **g2);

extern LIST *sort_groups(LIST *glist)
{
    LIST *slist;
    GROUP **stab;
    GROUP *gp;
    int st_ix;

    /* Construct the sort table */
    stab = calloc(LIST_LENGTH(glist), sizeof(GROUP *));
    if (!stab)
	gupout(1, "Malloc of sort table failed");

    st_ix = 0;
    TRAVERSE(glist, gp) {
	gp->order = st_ix;	/* Maintain order for exact matches */
	stab[st_ix++] = gp;
    }

    /* Do the sort */
    qsort(stab, LIST_LENGTH(glist), sizeof(GROUP *), mat_compare);

    /* Build the results list */
    slist = create_list();

    for (st_ix = 0; st_ix < LIST_LENGTH(glist); st_ix++)
	add_group(slist, stab[st_ix]);

    free(stab);
    free(glist);

    return slist;
}


static int mat_compare(const GROUP **g1, const GROUP **g2)
{
    register char c1, c2;
    const char *cp1, *cp2;

    for (cp1 = (*g1)->name, cp2 = (*g2)->name;; cp1++, cp2++) {
	c1 = *cp1;
	c2 = *cp2;

	if (!c1 || !c2)
	    return c1 - c2;	/* Hit end of string */

	/* If either character is a meta character, return original order */
	if ((c1 == '\\') || (c1 == '?') || (c1 == '*') || (c1 == '[') ||
	    (c2 == '\\') || (c2 == '?') || (c2 == '*') || (c2 == '['))
	    return (*g1)->order - (*g2)->order;
	if (c1 != c2)
	    return c1 - c2;
    }
}
