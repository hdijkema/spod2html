## Process this file with automake to produce Makefile.in
## Created by Anjuta

SUBDIRS = src \
	css 

spod2htmldocdir = ${prefix}/doc/spod2html
spod2htmldoc_DATA = \
	README\
	COPYING\
	AUTHORS\
	ChangeLog\
	INSTALL\
	NEWS


EXTRA_DIST = $(spod2htmldoc_DATA)

CLEANFILES = $(shell find . -name "*~")


# Remove doc directory on uninstall
uninstall-local:
	-rm -r $(spod2htmldocdir)
