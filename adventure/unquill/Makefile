#
# Makefile for Unix version of Unquill
#
# Currently, without a DOS cross-compiler, I have to do the DOS version
# separately.
#
# [0.10.0] Dropped CP/M support, since Unquill is now too complex for the
#          Hi-Tech C compiler to cope with.
#
IQSRC  = IQRun/iqrun.inf IQRun/iqproc.h IQRun/linker.c IQRun/Makefile  \
         IQRun/COPYING.LIB
SRCS   = unquill.c   playgame.c tables.c   condact.c   inform.c   zcode.c \
	 unquill.h Makefile  xtables.c  dosvideo.c tsans08.inc Makefile.win32 \
	 $(IQSRC)
OBJS   = unquill.o   playgame.o   tables.o   condact.o   inform.o   zcode.o \
	 xtables.o
DOCS   = COPYING unquill.txt unquill.1
#
# Directory for iqrun.z5. Note trailing slash!
#
LIBDIR = /usr/lib/games/zcode/
#
# Install locations
#
BINDIR = /usr/local/bin
MANDIR = /usr/local/man/man1


CC      = gcc
#CFLAGS  = -g -ansi -U__STRICT_ANSI__ -Wall -pedantic -DLIBDIR=\"$(LIBDIR)\"
CFLAGS  = -O2 -ansi -U__STRICT_ANSI__ -Wall -pedantic -DLIBDIR=\"$(LIBDIR)\"
ZIP     = zip 

all:	unquill $(DOCS) iqrun.z5

iqrun.z5: $(IQSRC)
	cd IQRun && make && cp iqrun.z5 ..
	cd ..

unquill:	$(OBJS)
	$(CC) -o unquill $(OBJS)

unquill.man:	unquill.1
	nroff -c -mandoc $< > $@

unquill.txt:	unquill.man
	col -b < $< > $@

install: unquill unquill.1 iqrun.z5
	install -m 755 unquill   $(BINDIR)
	install -m 644 unquill.1 $(MANDIR)
	install -m 644 iqrun.z5  $(LIBDIR)

tidy:
	rm -f $(OBJS) *~ *.bak unquill.man

clean:	tidy
	rm -f unquill.com unquill

$(OBJS):	unquill.h

zip: unquill.exe unquillw32.exe iqrun.z5 $(SRCS) $(DOCS)
	$(ZIP) unquill.zip $(SRCS) $(DOCS) iqrun.z5 unquill.exe unquillw32.exe

