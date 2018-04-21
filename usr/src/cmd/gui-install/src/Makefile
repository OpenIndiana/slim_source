#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.
#
#
# Modified for use in Dwarf Caiman project
# edit from originally generated Makefile
# text substitutions:
#	CC definition commented - to be inherited from above
#	DESTDIR -> ROOT
#
include ../../Makefile.cmd
#include $(SRC)/Makefile.master

srcdir = .
top_srcdir = ..

pkgdatadir = $(datadir)/gui-install
pkglibdir = $(libdir)/gui-install
pkgincludedir = $(includedir)/gui-install
top_builddir = ..
am__cd = CDPATH="$${ZSH_VERSION+.}$(PATH_SEPARATOR)" && cd
INSTALL = /usr/sbin/install
install_sh_DATA = $(install_sh) -c -m 644
install_sh_PROGRAM = $(install_sh) -c
install_sh_SCRIPT = $(install_sh) -c
INSTALL_HEADER = $(INSTALL_DATA)
transform = $(program_transform_name)
NORMAL_INSTALL = :
PRE_INSTALL = :
POST_INSTALL = :
NORMAL_UNINSTALL = :
PRE_UNINSTALL = :
POST_UNINSTALL = :
build_triplet = i386-pc-solaris2.11
host_triplet = i386-pc-solaris2.11
bin_PROGRAMS = gui-install$(EXEEXT)
subdir = src
mkinstalldirs = /usr/bin/mkdir -p
CONFIG_HEADER = $(top_builddir)/config.h
CONFIG_CLEAN_FILES =
am__installdirs = "$(ROOT)$(bindir)" "$(ROOT)$(gladedir)"
binPROGRAMS_INSTALL = $(INSTALL_PROGRAM)
PROGRAMS = $(bin_PROGRAMS)
am__objects_1 = window-graphics.$(OBJEXT)
am__objects_2 = orchestrator-wrappers.$(OBJEXT)
am__objects_3 = error-logging.$(OBJEXT)
am_gui_install_OBJECTS = main.$(OBJEXT) callbacks.$(OBJEXT) \
	welcome-screen.$(OBJEXT) users-screen.$(OBJEXT) \
	confirmation-screen.$(OBJEXT) datetimezone-screen.$(OBJEXT) \
	language-screen.$(OBJEXT) installation-screen.$(OBJEXT) \
	installation-disk-screen.$(OBJEXT) failure-screen.$(OBJEXT) \
	diskbutton.$(OBJEXT) \
	timezone.$(OBJEXT) \
	map.$(OBJEXT) \
	upgrade-screen.$(OBJEXT) finish-screen.$(OBJEXT) $(am__objects_1) $(am__objects_2) $(am__objects_3)\
	help-dialog.$(OBJEXT) \
	disk-block-order.${OBJEXT}
gui_install_OBJECTS = $(am_gui_install_OBJECTS)
am__DEPENDENCIES_1 =
gui_install_DEPENDENCIES = $(am__DEPENDENCIES_1)
DEFAULT_INCLUDES = -I. -I$(srcdir) -I$(top_builddir)
depcomp = $(SHELL) $(top_srcdir)/depcomp
am__depfiles_maybe = depfiles
COMPILE = $(CC) $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS) $(CFLAGS)
CCLD = $(CC)
LINK = $(CCLD) $(AM_CFLAGS) $(CFLAGS) $(AM_LDFLAGS) $(LDFLAGS) -o $@
SOURCES = $(gui_install_SOURCES)
DIST_SOURCES = $(gui_install_SOURCES) 
am__vpath_adj_setup = srcdirstrip=`echo "$(srcdir)" | sed 's|.|.|g'`;
am__vpath_adj = case $$p in \
    $(srcdir)/*) f=`echo "$$p" | sed "s|^$$srcdirstrip/||"`;; \
    *) f=$$p;; \
  esac;
am__strip_dir = `echo $$p | sed -e 's|^.*/||'`;
gladeDATA_INSTALL = $(INSTALL_DATA)
DATA = $(glade_DATA)
ETAGS = etags
CTAGS = ctags
DISTFILES = $(DIST_COMMON) $(DIST_SOURCES) $(TEXINFOS) $(EXTRA_DIST)
ACLOCAL = ${SHELL} ${SRC}/cmd/gui-install/missing --run aclocal-1.9
AMDEP_FALSE = #
AMDEP_TRUE = 
AMTAR = ${SHELL} ${SRC}/cmd/gui-install/missing --run tar
AUTOCONF = ${SHELL} ${SRC}/cmd/gui-install/missing --run autoconf
AUTOHEADER = ${SHELL} ${SRC}/cmd/gui-install/missing --run autoheader
AUTOMAKE = ${SHELL} ${SRC}/cmd/gui-install/missing --run automake-1.9
AWK = nawk
CATALOGS = 
CATOBJEXT = .gmo
#commented to allow inheritance CC = /ws/onnv-tools/SUNWspro/SS11/bin/cc
CCDEPMODE = depmode=none
CFLAGS = -Wno-deprecated-declarations
CPP = ${CC} -E
#CPP = /ws/onnv-tools/SUNWspro/SS11/bin/cc -E
CPPFLAGS = -I${SRC}/lib/liborchestrator -I${SRC}/lib/liblogsvc -std=c99 -D__EXTENSIONS__
CYGPATH_W = echo
DATADIRNAME = share
DEFS = -DHAVE_CONFIG_H
DEPDIR = .deps
ECHO_C = 
ECHO_N = -n
ECHO_T = 
EGREP = /usr/sfw/bin/ggrep -E
EXEEXT = 
GETTEXT_PACKAGE = gui-install
GMOFILES = 
GMSGFMT = /usr/bin/msgfmt
GREP = /usr/sfw/bin/ggrep
INSTALL_DATA = ${INSTALL} -m 644 -f
INSTALL_PROGRAM = ${INSTALL} -f
INSTALL_SCRIPT = ${INSTALL} -f
INSTALL_STRIP_PROGRAM = ${SHELL} $(install_sh) -f -s
INSTOBJEXT = .mo
INTLLIBS = 
LDFLAGS = -L${ROOT}/usr/snadm/lib $(LDLIBS.cmd) -R/usr/snadm/lib -lorchestrator -llogsvc -lzoneinfo -lnvpair -lbe
LIBOBJS = 
LIBS = 
LTLIBOBJS = 
MAINT = #
MAINTAINER_MODE_FALSE = 
MAINTAINER_MODE_TRUE = #
MAKEINFO = ${SHELL} ${SRC}/cmd/gui-install/missing --run makeinfo
MSGFMT = /usr/bin/msgfmt
OBJEXT = o
PACKAGE = gui-install
PACKAGE_BUGREPORT = 
PACKAGE_CFLAGS = -DORBIT2=1 -D_REENTRANT -D_PTHREADS -I/usr/include/gtk-2.0 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/gconf/2 -I/usr/include/libbonoboui-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/orbit-2.0 -I/usr/include/libbonobo-2.0 -I/usr/include/bonobo-activation-2.0 -I/usr/include/pango-1.0 -I/usr/sfw/include/freetype2 -I/usr/sfw/include -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/libxml2 -I/usr/include/libglade-2.0
PACKAGE_LIBS = -lSM -lICE -lpangoft2-1.0 -lbonobo-2 -lbonobo-activation -lpopt -lgconf-2 -lORBit-2 -lgthread-2.0 -lthread -lglade-2.0 -lgtk-x11-2.0 -lxml2 -lpthread -lz -lsocket -lnsl -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0  
PACKAGE_NAME = 
PACKAGE_STRING = 
PACKAGE_TARNAME = 
PACKAGE_VERSION = 
PATH_SEPARATOR = :
PKG_CONFIG = /usr/bin/pkg-config
POFILES = 
POSUB = po
PO_IN_DATADIR_FALSE = 
PO_IN_DATADIR_TRUE = 
SET_MAKE = 
SHELL = /bin/bash
STRIP = 
USE_NLS = yes
VERSION = 0.2
XGETTEXT = /usr/bin/xgettext
ac_ct_CC = ${CC}
#ac_ct_CC = /ws/onnv-tools/SUNWspro/SS11/bin/cc
am__fastdepCC_FALSE = 
am__fastdepCC_TRUE = #
am__include = include
am__leading_dot = .
am__quote = 
am__tar = ${AMTAR} chof - "$$tardir"
am__untar = ${AMTAR} xf -
bindir = ${exec_prefix}/bin
build = i386-pc-solaris2.11
build_alias = 
build_cpu = i386
build_os = solaris2.11
build_vendor = pc
datadir = ${datarootdir}
datarootdir = ${prefix}/share
docdir = ${datarootdir}/doc/${PACKAGE}
dvidir = ${docdir}
exec_prefix = ${prefix}
host = i386-pc-solaris2.11
host_alias = 
host_cpu = i386
host_os = solaris2.11
host_vendor = pc
htmldir = ${docdir}
includedir = ${prefix}/include
infodir = ${datarootdir}/info
install_sh = $(INS)
libdir = ${exec_prefix}/lib
libexecdir = ${exec_prefix}/libexec
localedir = ${datarootdir}/locale
localstatedir = ${prefix}/var
mandir = ${datarootdir}/man
mkdir_p = mkdir -p --
oldincludedir = /usr/include
pdfdir = ${docdir}
prefix = /usr
program_transform_name = s,x,x,
psdir = ${docdir}
sbindir = ${exec_prefix}/sbin
sharedstatedir = ${prefix}/com
sysconfdir = ${prefix}/etc
target_alias = 
INCLUDES = \
	-DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	-DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\" \
	-DGLADEDIR=\""$(datadir)/$(PACKAGE)"\" \
	-DTEXTDIR=\""$(datadir)/$(PACKAGE)"\" \
	-DPIXMAPDIR=\""$(datadir)/$(PACKAGE)"\" \
	-DORBIT2=1 -D_REENTRANT -D_PTHREADS -I/usr/include/gtk-2.0 -I/usr/include/gconf/2 -I/usr/include/libbonoboui-2.0 -I/usr/include/glib-2.0 -I/usr/lib/glib-2.0/include -I/usr/include/orbit-2.0 -I/usr/include/libbonobo-2.0 -I/usr/include/bonobo-activation-2.0 -I/usr/include/pango-1.0 -I/usr/sfw/include/freetype2 -I/usr/sfw/include -I/usr/lib/gtk-2.0/include -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/libxml2 -I/usr/include/libglade-2.0  -I/usr/include/gdk-pixbuf-2.0

orchestrator_wrapper_SOURCES = \
	orchestrator-wrappers.c \
	orchestrator-wrappers.h \
	orchestrator_api.h 

window_graphics_SOURCES = \
        window-graphics.c \
        window-graphics.h \
	pixbufs.h

error_logging_SOURCES = \
		error-logging.c \
		error-logging.h

gui_install_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	welcome-screen.c welcome-screen.h \
	users-screen.c users-screen.h \
	confirmation-screen.c confirmation-screen.h \
	datetimezone-screen.c datetimezone-screen.h \
	language-screen.c language-screen.h\
	installation-screen.c installation-screen.h \
	installation-disk-screen.c installation-disk-screen.h \
	failure-screen.c failure-screen.h \
	finish-screen.c finish-screen.h \
	diskbutton.c diskbutton.h \
	timezone.c timezone.h \
	map.c map.h \
	upgrade-screen.c upgrade-screen.h \
	installation-profile.h interface-globals.h \
	$(error_logging_SOURCES) \
	$(window_graphics_SOURCES) \
	$(orchestrator_wrapper_SOURCES) \
	help-dialog.c help-dialog.h \
	disk-block-order.c disk-block-order.h

#gui_install_SOURCES = \
	main.c \
	callbacks.c callbacks.h \
	welcome-screen.c welcome-screen.h \
	users-screen.c users-screen.h \
	confirmation-screen.c confirmation-screen.h \
	datetimezone-screen.c datetimezone-screen.h \
	language-screen.c language-screen.h\
	installation-screen.c installation-screen.h \
	installation-disk-screen.c installation-disk-screen.h \
	failure-screen.c failure-screen.h \
	devtools-screen.c devtools-screen.h \
	diskbutton.c diskbutton.h \
	upgrade-screen.c upgrade-screen.h \
	installation-profile.h interface-globals.h \
	$(window_graphics_SOURCES) \
	$(orchestrator_wrapper_SOURCES) \
	help-dialog.c help-dialog.h \
	disk-block-order.c disk-block-order.h
	

gui_install_LDADD = -lSM -lICE -lpangoft2-1.0 -lpopt -lgconf-2 -lORBit-2 -lgthread-2.0 -lthread -lglade-2.0 -lgtk-x11-2.0 -lxml2 -lpthread -lz -lsocket -lnsl -lgdk-x11-2.0 -latk-1.0 -lgdk_pixbuf-2.0 -lm -lpangocairo-1.0 -lpango-1.0 -lcairo -lgobject-2.0 -lgmodule-2.0 -lglib-2.0   $(INTLLIBS)

gladedir = $(datadir)/$(PACKAGE)
glade_DATA = \
	$(PACKAGE).glade \
	confirmation.glade \
	date-time-zone.glade \
	failure.glade \
	installationdisk.glade \
	installation.glade \
	users.glade \
	diskbutton.glade \
	$(NULL)

EXTRA_DIST = \
	$(glade_DATA) \
	$(text_DATA) \
	$(NULL)

all: all-am

.SUFFIXES:
.SUFFIXES: .c .o .obj

install-binPROGRAMS: $(bin_PROGRAMS)
	@$(NORMAL_INSTALL)
	test -z "$(bindir)" || $(mkdir_p) "$(ROOT)$(bindir)"
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  if test -f $$p \
	  ; then \
	   echo " $(INSTALL_PROGRAM_ENV) $(binPROGRAMS_INSTALL) '$(ROOT)$(bindir)/' '$$p'"; \
	   $(INSTALL_PROGRAM_ENV) $(binPROGRAMS_INSTALL) "$(ROOT)$(bindir)/" "$$p" || exit 1; \
	  else :; fi; \
	done

uninstall-binPROGRAMS:
	@$(NORMAL_UNINSTALL)
	@list='$(bin_PROGRAMS)'; for p in $$list; do \
	  f=`echo "$$p" | sed 's,^.*/,,;s/$(EXEEXT)$$//;$(transform);s/$$/$(EXEEXT)/'`; \
	  echo " rm -f '$(ROOT)$(bindir)/$$f'"; \
	  rm -f "$(ROOT)$(bindir)/$$f"; \
	done

clean-binPROGRAMS:
	-test -z "$(bin_PROGRAMS)" || rm -f $(bin_PROGRAMS)
gui-install$(EXEEXT): $(gui_install_OBJECTS) $(gui_install_DEPENDENCIES) 
	@rm -f gui-install$(EXEEXT)
	$(LINK) $(gui_install_LDFLAGS) $(gui_install_OBJECTS) $(gui_install_LDADD) $(LIBS)

mostlyclean-compile:
	-rm -f *.$(OBJEXT)

distclean-compile:
	-rm -f *.tab.c

#include ./$(DEPDIR)/callbacks.Po
#include ./$(DEPDIR)/confirmation-screen.Po
#include ./$(DEPDIR)/datetimezone-screen.Po
#include ./$(DEPDIR)/diskbutton.Po
#include ./$(DEPDIR)/failure-screen.Po
#include ./$(DEPDIR)/help-dialog.Po
#include ./$(DEPDIR)/installation-disk-screen.Po
#include ./$(DEPDIR)/installation-screen.Po
#include ./$(DEPDIR)/language-screen.Po
#include ./$(DEPDIR)/main.Po
#include ./$(DEPDIR)/orchestrator-wrappers.Po
#include ./$(DEPDIR)/upgrade-screen.Po
#include ./$(DEPDIR)/users-screen.Po
#include ./$(DEPDIR)/welcome-screen.Po
#include ./$(DEPDIR)/window-graphics.Po

.c.o:
#	if $(COMPILE) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o $@ $<; \
#	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
	source='$<' object='$@' libtool=no \
	$(COMPILE) -c $<
#	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) 

.c.obj:
#	if $(COMPILE) -MT $@ -MD -MP -MF "$(DEPDIR)/$*.Tpo" -c -o $@ `$(CYGPATH_W) '$<'`; \
#	then mv -f "$(DEPDIR)/$*.Tpo" "$(DEPDIR)/$*.Po"; else rm -f "$(DEPDIR)/$*.Tpo"; exit 1; fi
	source='$<' object='$@' libtool=no \
	$(COMPILE) -c `$(CYGPATH_W) '$<'`
uninstall-info-am:
install-gladeDATA: $(glade_DATA)
	@$(NORMAL_INSTALL)
	test -z "$(gladedir)" || $(mkdir_p) "$(ROOT)$(gladedir)"
	@list='$(glade_DATA)'; for p in $$list; do \
	  if test -f "$$p"; then d=; else d="$(srcdir)/"; fi; \
	  echo " $(gladeDATA_INSTALL) '$(ROOT)$(gladedir)' '$$d$$p'"; \
	  $(gladeDATA_INSTALL) "$(ROOT)$(gladedir)" "$$d$$p"; \
	done
#	DEPDIR=$(DEPDIR) $(CCDEPMODE) $(depcomp) 

uninstall-gladeDATA:
	@$(NORMAL_UNINSTALL)
	@list='$(glade_DATA)'; for p in $$list; do \
	  f=$(am__strip_dir) \
	  echo " rm -f '$(ROOT)$(gladedir)/$$f'"; \
	  rm -f "$(ROOT)$(gladedir)/$$f"; \
	done

ID: $(HEADERS) $(SOURCES) $(LISP) $(TAGS_FILES)
	list='$(SOURCES) $(HEADERS) $(LISP) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	mkid -fID $$unique
tags: TAGS

TAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) \
		$(TAGS_FILES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)  $(LISP) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	if test -z "$(ETAGS_ARGS)$$tags$$unique"; then :; else \
	  test -n "$$unique" || unique=$$empty_fix; \
	  $(ETAGS) $(ETAGSFLAGS) $(AM_ETAGSFLAGS) $(ETAGS_ARGS) \
	    $$tags $$unique; \
	fi
ctags: CTAGS
CTAGS:  $(HEADERS) $(SOURCES)  $(TAGS_DEPENDENCIES) \
		$(TAGS_FILES) $(LISP)
	tags=; \
	here=`pwd`; \
	list='$(SOURCES) $(HEADERS)  $(LISP) $(TAGS_FILES)'; \
	unique=`for i in $$list; do \
	    if test -f "$$i"; then echo $$i; else echo $(srcdir)/$$i; fi; \
	  done | \
	  $(AWK) '    { files[$$0] = 1; } \
	       END { for (i in files) print i; }'`; \
	test -z "$(CTAGS_ARGS)$$tags$$unique" \
	  || $(CTAGS) $(CTAGSFLAGS) $(AM_CTAGSFLAGS) $(CTAGS_ARGS) \
	     $$tags $$unique

GTAGS:
	here=`$(am__cd) $(top_builddir) && pwd` \
	  && cd $(top_srcdir) \
	  && gtags -i $(GTAGS_ARGS) $$here

distclean-tags:
	-rm -f TAGS ID GTAGS GRTAGS GSYMS GPATH tags

distdir: $(DISTFILES)
	@srcdirstrip=`echo "$(srcdir)" | sed 's|.|.|g'`; \
	topsrcdirstrip=`echo "$(top_srcdir)" | sed 's|.|.|g'`; \
	list='$(DISTFILES)'; for file in $$list; do \
	  case $$file in \
	    $(srcdir)/*) file=`echo "$$file" | sed "s|^$$srcdirstrip/||"`;; \
	    $(top_srcdir)/*) file=`echo "$$file" | sed "s|^$$topsrcdirstrip/|$(top_builddir)/|"`;; \
	  esac; \
	  if test -f $$file || test -d $$file; then d=.; else d=$(srcdir); fi; \
	  dir=`echo "$$file" | sed -e 's,/[^/]*$$,,'`; \
	  if test "$$dir" != "$$file" && test "$$dir" != "."; then \
	    dir="/$$dir"; \
	    $(mkdir_p) "$(distdir)$$dir"; \
	  else \
	    dir=''; \
	  fi; \
	  if test -d $$d/$$file; then \
	    if test -d $(srcdir)/$$file && test $$d != $(srcdir); then \
	      cp -pR $(srcdir)/$$file $(distdir)$$dir || exit 1; \
	    fi; \
	    cp -pR $$d/$$file $(distdir)$$dir || exit 1; \
	  else \
	    test -f $(distdir)/$$file \
	    || cp -p $$d/$$file $(distdir)/$$file \
	    || exit 1; \
	  fi; \
	done
check-am: all-am
check: check-am
all-am: Makefile $(PROGRAMS) $(DATA)
installdirs:
	for dir in "$(ROOT)$(bindir)" "$(ROOT)$(gladedir)"; do \
	  test -z "$$dir" || $(mkdir_p) "$$dir"; \
	done
install: install-am
install-exec: install-exec-am
install-data: install-data-am
uninstall: uninstall-am

install-am: all-am
	@$(MAKE) $(AM_MAKEFLAGS) install-exec-am install-data-am

installcheck: installcheck-am
install-strip:
	$(MAKE) $(AM_MAKEFLAGS) INSTALL_PROGRAM="$(INSTALL_STRIP_PROGRAM)" \
	  install_sh_PROGRAM="$(INSTALL_STRIP_PROGRAM)" INSTALL_STRIP_FLAG=-s \
	  `test -z '$(STRIP)' || \
	    echo "INSTALL_PROGRAM_ENV=STRIPPROG='$(STRIP)'"` install
mostlyclean-generic:

clean-generic:

distclean-generic:
	-test -z "$(CONFIG_CLEAN_FILES)" || rm -f $(CONFIG_CLEAN_FILES)

maintainer-clean-generic:
	@echo "This command is intended for maintainers to use"
	@echo "it deletes files that may require special tools to rebuild."
clean: clean-am

clean-am: clean-binPROGRAMS clean-generic mostlyclean-am

distclean: distclean-am
	-rm -rf ./$(DEPDIR)
	-rm -f Makefile
distclean-am: clean-am distclean-compile distclean-generic \
	distclean-tags

dvi: dvi-am

dvi-am:

html: html-am

info: info-am

info-am:

install-data-am: install-gladeDATA

install-exec-am: install-binPROGRAMS

install-info: install-info-am

install-man:

installcheck-am:

LINTTARG=	$(PROGRAMS:%=%.lint)
lint:	$(LINTTARG)

gui-install.lint:=	VARPREFIX=gui_install

LINTFLAGS= -sx $(DEFS) $(DEFAULT_INCLUDES) $(INCLUDES) $(AM_CPPFLAGS) \
	$(CPPFLAGS) $(AM_CFLAGS)

$(LINTTARG):
	$(LINT.c) $($(VARPREFIX)_LDFLAGS) $($(VARPREFIX)_LDADD:-R%=) $(LIBS) \
	    $($(VARPREFIX)_OBJECTS:%.o=%.c)

maintainer-clean: maintainer-clean-am
	-rm -rf ./$(DEPDIR)
	-rm -f Makefile
maintainer-clean-am: distclean-am maintainer-clean-generic

mostlyclean: mostlyclean-am

mostlyclean-am: mostlyclean-compile mostlyclean-generic

pdf: pdf-am

pdf-am:

ps: ps-am

ps-am:

uninstall-am: uninstall-binPROGRAMS uninstall-gladeDATA \
	uninstall-info-am

.PHONY: CTAGS GTAGS all all-am check check-am clean clean-binPROGRAMS \
	clean-generic ctags distclean distclean-compile \
	distclean-generic distclean-tags distdir dvi dvi-am html \
	html-am info info-am install install-am install-binPROGRAMS \
	install-data install-data-am install-exec install-exec-am \
	install-gladeDATA install-info install-info-am install-man \
	install-strip installcheck installcheck-am installdirs \
	lint $(LINTTARG) \
	maintainer-clean maintainer-clean-generic mostlyclean \
	mostlyclean-compile mostlyclean-generic pdf pdf-am ps ps-am \
	tags uninstall uninstall-am uninstall-binPROGRAMS \
	uninstall-gladeDATA uninstall-info-am

# Tell versions [3.59,3.63) of GNU make to not export all variables.
# Otherwise a system limit (for SysV at least) may be exceeded.
.NOEXPORT:
