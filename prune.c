/*
 * This routine does an exhaustive check on the groups file.
 * It weeds out:
 *
 * all redundant additions.
 * all !groups that don't filter anything.
 * all groups that don't match any of the actives.
 *
 * Any such groups are deleted.
 *
 * CP is not considered an issue...
 */

#include "gup.h"

/* Remove all superfluous groups */
void prune(LIST *active_l, LIST *group_l)
{
    GROUP *gp;
    GROUP *ap;
    int groupcount;

    groupcount = 0;
    /* tag the active list */
    TRAVERSE(active_l, ap) {
	ap->u.tag = NULL;

	TRAVERSE(group_l, gp) {
	    if (wildmat(ap->name, gp->name)) {
		TAG *t = (TAG *) malloc(sizeof(TAG));
		if (!t) {
		    logit(L_BOTH, "WARNING: insufficient memory to prune!");
		    return;
		}
		/* add the group to the front of this ap's tag list */
		t->group = gp;
		t->next = ap->u.tag;
		ap->u.tag = t;
	    }
	}
	if (ap && ap->u.tag && ap->u.tag->group)
	    if (ap->u.tag->group->u.not == GUP_INCLUDE)
		groupcount++;
    }

    /* see if too many groups in the list */
    if (groupcount > maxgroups)
	gupout(1, "Too many groups (%d, only %d allowed)",
		groupcount, maxgroups);

    /* iterate the groups list to see if each group any effect on the active */
    TRAVERSE(group_l, gp) {
	int effect = FALSE;

	TRAVERSE(active_l, ap) {
	    TAG *tag = ap->u.tag;

	    /* does gp affect ap? */
	    if (tag && tag->group == gp && ((!tag->next && !gp->u.not) ||
		(tag->next && tag->next->group->u.not != gp->u.not))) {
		effect = TRUE;
		break;
	    }
	}

	if (!effect && gp->u.not != GUP_POISON) {
	    /* group doesn't do anything - clobber it */

	    /*
	     * remove it from any tags in which it appears - makes the
	     * tag-check faster.  We *could* have done an active tag list
	     * for each group to speed this, but it at the expense of the
	     * initial tagging process.  As implemented here, we assume
	     * pruning is a relatively rare occurrence.
	     */
	    TRAVERSE(active_l, ap) {
		TAG *t, *prev;

		for (prev = NULL, t = ap->u.tag; t; prev = t, t = t->next) {
		    if (t->group == gp) {
			if (!prev)
			    ap->u.tag = t->next;
			else
			    prev->next = t->next;
			free(t);

			break;
		    }
		}
	    }

	    remove_group(group_l, gp);

	    logit(L_BOTH, "PRUNED: %s %s subsumed",
		    gp->u.not ? "exclude" : "include", gp->name);
	}
    }
}
