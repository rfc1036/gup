
/*
 * HEADER FILE FOR NEWS/MAIL GATEWAY CODE.
 * $Header: /var/src/gup/RCS/rfc822.h,v 1.1 93/07/22 09:52:33 andrew Exp Locker: andrew $
 */

/* Do you need to handle eight-bit characters in your addresses?  If
 * not (e.g., just USASCII) then #undef the next line.
 */
/* this is broken! why? */
/*#define DO_8BIT_CHARS */


#define SM_SIZE		512	/* A smallish buffer size */
#define LG_SIZE		1024	/* big buffer size                */

#if	!defined(DO_8BIT_CHARS)
#define ADDRCHAR	char	/* Addresses internal representation      */
#define QUOTE_MASK	0x0080	/* Mask to turn on 8-bit quoting  */
#define UNQUOTE_MASK	0x007F	/* Mask to turn off the 8-bit quote       */
#else
#define ADDRCHAR	short	/* Addresses internal representation      */
#define QUOTE_MASK	0x0100	/* Mask to turn on 9-bit quoting  */
#define UNQUOTE_MASK	0x00FF	/* Mask to turn off the 9-bit quote       */
#endif				/* defined(DO_8BIT_CHARS) */

/*
 * External declarations.
 */

/* Routines we provide. */
extern int CrackFrom(ADDRCHAR *addr, char *name, char *p);
