/*
 * Handle starting up the mail command and piping the 822 headers
 * and other data to it.
 */

#include "gup.h"

FILE *mail_open(int open_now, const char *to, const char *command,
	const char *headers)
{
    FILE *fp;

    static char *m_to = NULL;
    static char *m_command = NULL;
    static char *m_headers = NULL;

/* Stash variables as they are defined */

    if (to)
	m_to = xstrdup(to);
    if (command) {
#if 1
	m_command = xstrdup(command);
#else
	m_command = malloc(strlen(command) + 4 + sizeof(BACKSTOP_MAILID));
	if (!m_command)
	    gupout(1, "malloc of m_command failed");
	strcpy(m_command, command);
	strcat(m_command, " -f ");
	strcat(m_command, BACKSTOP_MAILID);
#endif
    }
    if (headers)
	m_headers = xstrdup(headers);

    if (!open_now)
	return NULL;

    if (!m_command)
	gupout(1, "Install error. No mail command supplied");

    fp = popen(m_command, "w");
    if (!fp)
	gupout(1, "Could not open a pipe to '%s'", m_command);

    /* Write order is: TO: , supplied headers file, blank line.
     * Note that the headers are expected to contain reply-to, Subject and
     * such, but can be empty or non-existant for sendmail. Furthermore,
     * there is no reason why a preceding blab of text cannot go in there too!
     */
    if (m_to)
	fprintf(fp, "To: %s\n", m_to);

    if (m_headers) {
	FILE *hdr_fp;
	char lbuf[MAX_LINE_SIZE];

	if ((hdr_fp = fopen(m_headers, "r"))) {
	    while (fgets(lbuf, sizeof(lbuf), hdr_fp))
		fputs(lbuf, fp);
	    fclose(hdr_fp);
	} else
	    logit(L_LOG, "WARNING: Could not open '%s' (%s)",
		    m_headers, strerror(errno));
    } else			/* Add a default header */
	fputs("Subject: Results of your request\n", fp);

    fputs("\n\n", fp);		/* End of headers for certain */
    return fp;
}

extern void mail_close(FILE *fp)
{
    if (pclose(fp) == -1)
	gupout(1, "pclose() of mail command failed");
}
