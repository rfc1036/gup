#include "gup.h"

int locked = FALSE;		/* do we have a lock? */

/* Take a lock in the current directory */

int lockit(void)
{
    int fd;
    int count;

#ifndef NO_FLOCK
    /* more style points that FILE_LOCK, if it succeeds */
    if ((fd = open(".", O_RDONLY)) >= 0) {
	if (!flock(fd, LOCK_EX)) {
	    /* lock was successful */
	    /* close(fd); *//* XXX does this affect our lock? */
	    locked = TRUE;
	    return TRUE;
	}
	logit(L_LOG, "WARNING", "flock failed");
	close(fd);		/* flock failed, so don't hold on */
    }
    /* OK, try a file lock if that's defined */
#endif

#ifdef FILE_LOCK
    for (count = 0; count < MAX_LOCK_SLEEP_COUNT; count++) {
	fd = open("LOCK", O_CREAT | O_WRONLY | O_EXCL, 0644);
	if (fd >= 0 || errno != EEXIST)
	    break;
	sleep(LOCK_SLEEP);
    }
    if (fd < 0 && errno == EEXIST) {
	fd = open("LOCK", O_CREAT | O_WRONLY);
	if (fd >= 0)
	    logit(L_LOG, "WARNING", "Trashed pre-existing lock");
    }
    if (fd < 0)
	gupout(1, "Could not open lock file");

    /* Take the lock and write out our pid if the lock works */
    {
	char buf[32];
	int res;

	sprintf(buf, "%5d\n", getpid());
	res = write(fd, buf, strlen(buf));
	if (res < 0)
	    gupout(1, "Could not write lock file");

	close(fd);

	locked = TRUE;
	return TRUE;
    }
#endif /* FILE_LOCK */

    return FALSE;
}


/* Release the lock created by lockit() */

void unlockit(void)
{
    if (locked) {
#ifndef NO_FLOCK
	int fd;
#endif

	locked = FALSE;

#ifndef NO_FLOCK
	if ((fd = open(".", O_RDONLY)) >= 0) {
	    if (!flock(fd, LOCK_UN)) {
		close(fd);
		return;
	    }
	    logit(L_LOG, "WARNING", "unflock failed");
	    close(fd);		/* flock failed, so don't hold on */
	}
	/* OK, try a file lock if that's defined */
#endif

#ifdef FILE_LOCK
	if (unlink("LOCK") < 0) {
	    if (errno == ENOENT)
		logit(L_LOG, "WARNING",
		    "Could not delete lock file 'cos it doesn't exist!");
	    else
		logit(L_LOG, "ERROR", "Could not delete lock file");
	}
#endif
    }
}
