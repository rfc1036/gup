/*
 *  Routines to read and write mail and news headers.  The code here
 *  is gross and complicated.
 */
#include "gup.h"
#include "rfc822.h"

#define QUESTIONABLE(c) \
	((c) == '"' || (c) == '(' || (c) == ')' || (c) == '\\')


/*
 *  Crack an RFC822 from header field into address and fullname.  We do
 *  this to make sure we write things out in official form.  "Be liberal
 *  in what you accept, conservative in what you generate."  Anyhow, we
 *  read things into three buffers, one for all <...> text, one for all
 *  (...) text, and a third for stuff not in either.  Either the first or
 *  third buffer will be the real address, depending on whether there is
 *  anything in buffer two or not.
 */
int CrackFrom(ADDRCHAR *addr, char *name, char *p)
{
    register ADDRCHAR *adp;
    register char *bp;
    register char *ap;
    register ADDRCHAR *cp = NULL;	/* keep gcc 2 happy */
    register ADDRCHAR CurrChar;
    register int comment;
    register int address;
    register int addrfound;
    register int QuoteNext;
    register int InQuotes;
    ADDRCHAR *xp;
    ADDRCHAR comm[LG_SIZE];
    ADDRCHAR commbuf[LG_SIZE];
    ADDRCHAR addrbuf[LG_SIZE];

    /* Just to make sure. */
    *name = '\0';
    *addr = '\0';

    if (p == NULL)
	return FALSE;

    /* Eat leading white space. */
    while (*p && isspace(*p))
	p++;

    /* Set defaults. */
    comm[0] = '\0';
    commbuf[0] = '\0';
    addrbuf[0] = '\0';
    adp = addrbuf;
    comment = 0;
    addrfound = 0;
    address = 0;
    QuoteNext = 0;
    InQuotes = 0;
    for (; *p; p++) {
	CurrChar = (*p & UNQUOTE_MASK) | QuoteNext;
	QuoteNext = CurrChar == '\\' ? QUOTE_MASK : 0;
	if (!comment && CurrChar == '"')
	    InQuotes = InQuotes ? 0 : QUOTE_MASK;
	else
	    CurrChar |= InQuotes;
	switch (CurrChar) {
	case '(':
	    if (comment == 0) {
		cp = commbuf;
		*cp = '\0';
	    }
	    comment++;
	    break;
	case ')':
	    if (comment > 0 && --comment == 0) {
		*cp = '\0';
		xp = comm;
		if (*xp) {
		    while (*xp)
			xp++;
		    *xp++ = ',';
		    *xp++ = ' ';
		}
		for (cp = &commbuf[1]; (*xp++ = *cp) != '\0'; cp++)
		    continue;
		cp = NULL;
		continue;
	    }
	    break;
	case '<':
	    if (address)
		return FALSE;	/* AWK! Abort! */
	    if (!comment) {
		address++;
		*adp = '\0';
		adp = addr;
	    }
	    break;
	case '>':
	    if (!comment && address) {
		address--;
		addrfound++;
		for (*adp = '\0', adp = addrbuf; *adp;)
		    adp++;
		continue;
	    }
	    break;
	}

	if (comment)
	    *cp++ = CurrChar;
	else if (!address || CurrChar != '<')
	    *adp++ = CurrChar;
	if (*p == '\0')
	    break;
    }

    *adp++ = '\0';

    if (addrfound) {
	for (ap = name, xp = addrbuf; (*ap++ = (*xp & UNQUOTE_MASK)) != 0; xp++)
	    continue;
    } else {
	for (cp = addr, xp = addrbuf; (*cp++ = *xp) != 0; xp++)
	    continue;
	*name = '\0';
    }

    /* Just to be sure that we got the full name, we'll take all of
     * the comments. */
    if (*comm) {
	ap = name;
	if (*ap) {
	    while (*ap)
		ap++;
	    *ap++ = ',';
	    *ap++ = ' ';
	}
	for (xp = comm; (*ap = (*xp++ & UNQUOTE_MASK)) != 0; ap++)
	    continue;
    }
    /* Copy back, skipping leading spaces and trailing spaces. */
    for (cp = addr; isspace(*cp); cp++)
	continue;
    for (xp = addr; (*xp = *cp++) != 0;)
	xp++;
    while (*addr && isspace(*--xp))
	*xp = '\0';

    /* Since characters in (comments) are interpreted differently from
     * those in "phrase" <address>, play it safe and remove all
     * questionable characters. */
    for (ap = name, bp = name; isspace(*bp) || QUESTIONABLE(*bp); bp++)
	continue;
    while ((*ap = *bp++) != '\0')
	if (!QUESTIONABLE(*ap))
	    ap++;
    while (*name && isspace(*--ap))
	*ap = '\0';
    return TRUE;
}
