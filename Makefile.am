# $Id: Makefile.am,v 1.9 2000/08/28 19:13:47 cph Exp $

SUBDIRS = test-wads DOS Win32

EXTRA_DIST = visplane.txt README

DOCS = README AUTHORS COPYING NEWS visplane.txt

docdir = $(datadir)/doc/@PACKAGE@
doc_DATA = $(DOCS)

bin_PROGRAMS=bsp

bsp_SOURCES=bsp.c endian.c funcs.c level.c makenode.c picknode.c bsp.h structs.h

bsp.c funcs.c endian.c level.c makenode.c picknode.c : bsp.h structs.h config.h

config.h	:	configure.in
	./configure

dos	:	$(DOCS)
	-mkdir dos-dist
	for fn in $(DOCS); do perl -p -e 's/$$/\r/' < $$fn > `echo dos-dist/$$fn | perl -p -e 's/(.txt)?$$/.txt/'`; done

make-README	:	index.html
	lynx -dump index.html > README

