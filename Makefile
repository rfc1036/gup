#################################################
# Change the following to suit your needs	#
#################################################

CC = cc
P_CFLAGS	= -g -O2 -Wall

# Destination directories for the executable and man page. Note that
# the executable is only used in a .forward so /usr/local/bin may
# not be the most appropriate place - though it doesn't hurt.

BINDIR		= /usr/bin
MANDIR		= /usr/man/man1

#################################################
# DO NOT CHANGE ANYTHING AFTER THIS COMMENT	#
#################################################

SHELL	=	/bin/sh

SRC	=	gup.c wildmat.c misc.c prune.c help.c mail.c \
		log.c newsgroups.c lock.c sort.c \
		rfc822.c

OBJS    =       $(SRC:.c=.o)

HDRS	=	gup.h rfc822.h config.h

CFLAGS  =       $(P_CFLAGS) $(P_NO_FLAGS) $(P_USE_FLAGS) $(P_INCLUDES)

LDFLAGS =       $(P_LDFLAGS) $(P_LIBS)

all:	gup

gup:	$(OBJS)
	$(CC) $(CFLAGS) $(OBJS) $(LDFLAGS) -o $@

# Lazy and safe
$(OBJS):	$(HDRS) Makefile

install:	$(BINDIR)/gup $(MANDIR)/gup.1

$(BINDIR)/gup:	gup
	cp $? $@

$(MANDIR)/gup.1:	gup.1
	cp $? $@

clean:
	rm -f $(OBJS) gup core a.out

