/* Handle logging to the log file and mail file */

#include "gup.h"


static void log_line(FILE * fp, int lflags, const char *stamp, const char *prefix, const char *log_msg);

/* Log a message, with a time stamp to the logfile and or mailfile */

void logit(int lflags, const char *prefix, const char *log_msg)
{
    time_t secs;
    struct tm *tm;
    char stamp[100];

    static int tried_mail_open = FALSE;

    if (lflags & L_TIMESTAMP) {
	time(&secs);
	tm = localtime(&secs);
	strftime(stamp, sizeof(stamp), "%d %h %Y %T: ", tm);
    }
    if (log_fp && (lflags & L_LOG)) {
	log_line(log_fp, lflags, stamp, prefix, log_msg);
	fflush(log_fp);
    }
    if (lflags & L_MAIL) {
/*
 * If the mail pipe has not been opened yet, then that means that this
 * is an error prior to a valid site command which means that mail goes
 * to the address found in the inbound headers, or as a last resort the
 * BACKSTOP_MAILID.
 */

	if (!mail_fp && !tried_mail_open) {
	    tried_mail_open = TRUE;
	    mail_fp = mail_open(TRUE,
			    (char *) NULL, (char *) NULL, (char *) NULL);
	}
	if (mail_fp)
	    /* to prevent ordering problems with parts using stdout */
	    log_line(mail_fp, lflags, stamp, prefix, log_msg);
    }
}


static void log_line(FILE *fp, int lflags, const char *stamp, const char *prefix, const char *log_msg)
{
    if (lflags & L_TIMESTAMP)
	fputs(stamp, fp);
    if (prefix && *prefix) {
	fputs(prefix, fp);
	fputs(": ", fp);
    }
    fputs(log_msg, fp);
    fputs("\n", fp);
}
