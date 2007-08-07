/* Handle logging to the log file and mail file */

#include "gup.h"

static void log_line(FILE *fp, int lflags, const char *stamp,
	const char *prefix, const char *out_msg, va_list AP)
{
    if (lflags & L_TIMESTAMP)
	fputs(stamp, fp);
    if (prefix)
	fprintf(fp, "%s: ", prefix);
    vfprintf(fp, out_msg, AP);
    fputs("\n", fp);
}

/* Log a message, with a time stamp to the logfile and or mailfile */
void vlogit(int lflags, const char *prefix, const char *log_msg, va_list AP)
{
    time_t secs;
    struct tm *tm;
    char stamp[100];
    va_list AP2;

    static int tried_mail_open = FALSE;

    va_copy(AP2, AP);
    if (lflags & L_TIMESTAMP) {
	time(&secs);
	tm = localtime(&secs);
	strftime(stamp, sizeof(stamp), "%d %h %Y %T: ", tm);
    }
    if (log_fp && (lflags & L_LOG)) {
	log_line(log_fp, lflags, stamp, prefix, log_msg, AP);
	fflush(log_fp);
    }
    if (lflags & L_MAIL) {
	/* If the mail pipe has not been opened yet, then that means that this
	 * is an error prior to a valid site command which means that mail goes
	 * to the address found in the inbound headers, or as a last resort the
	 * BACKSTOP_MAILID.
	 */
	if (!mail_fp && !tried_mail_open) {
	    tried_mail_open = TRUE;
#ifndef TEST
	    mail_fp = mail_open(TRUE, NULL, NULL, NULL);
#endif
	}
	if (mail_fp)
	    /* to prevent ordering problems with parts using stdout */
	    log_line(mail_fp, lflags, stamp, prefix, log_msg, AP2);
    }
    va_end(AP2);
}

void logit(int lflags, const char *log_msg, ...)
{
    va_list ap;

    va_start(ap, log_msg);
    vlogit(lflags, NULL, log_msg, ap);
    va_end(ap);
}

#ifdef TEST
FILE *log_fp;
FILE *mail_fp;

int main(void)
{
    log_fp = stdout;
    mail_fp = stdout;
    logit(L_MAIL, ">%s<>%d<", "string", 1234);
    exit(0);
}
#endif
