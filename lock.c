#include "gup.h"

static int locked = FALSE;	/* do we have a lock? */

/* Take a lock in the current directory */
int lockit(void)
{
    int fd;
#ifdef USE_DOTLOCK
    int count;
#endif

#ifdef USE_FLOCK
    /* more style points that FILE_LOCK, if it succeeds */
    if ((fd = open(".", O_RDONLY)) >= 0) {
	if (flock(fd, LOCK_EX) == 0) {	/* lock was successful */
	    locked = TRUE;
	    return TRUE;
	    /* one fd is used here and never closed */
	}
	logit(L_LOG, "WARNING: flock failed");
	close(fd);		/* flock failed, so don't hold on */
    }
#endif

#ifdef USE_DOTLOCK
    for (count = 0; count < MAX_LOCK_SLEEP_COUNT; count++) {
	fd = open("LOCK", O_CREAT | O_WRONLY | O_EXCL, 0644);
	if (fd >= 0 || errno != EEXIST)
	    break;
	sleep(LOCK_SLEEP);
    }
    if (fd < 0 && errno == EEXIST) {
	fd = open("LOCK", O_CREAT | O_WRONLY);
	if (fd >= 0)
	    logit(L_LOG, "WARNING: Trashed pre-existing lock");
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
#endif

    return FALSE;
}

/* Release the lock created by lockit() */
void unlockit(void)
{
    if (locked) {
#ifdef USE_FLOCK
	int fd;
#endif

	locked = FALSE;

#ifdef USE_FLOCK
	if ((fd = open(".", O_RDONLY)) >= 0) {
	    if (flock(fd, LOCK_UN) == 0) {
		close(fd);
		return;
	    }
	    logit(L_LOG, "WARNING: unflock failed");
	    close(fd);		/* flock failed, so don't hold on */
	}
#endif

#ifdef USE_DOTLOCK
	if (unlink("LOCK") < 0) {
	    if (errno == ENOENT)
		logit(L_LOG, "WARNING: Could not delete lock file "
			"'cos it doesn't exist!");
	    else
		logit(L_LOG, "ERROR: Could not delete lock file");
	}
#endif
    }
}
