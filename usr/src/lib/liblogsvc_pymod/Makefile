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

#
# Copyright 2009 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

LIBRARY		= liblogsvc

OBJECTS		= liblogsvc.o

CPYTHONLIBS	= liblogsvc.so

PRIVHDRS	=
EXPHDRS		=
HDRS		= $(EXPHDRS) $(PRIVHDRS)

include ../Makefile.lib

INCLUDE		= -I$(PYINCDIR) -I../liblogsvc

CPPFLAGS	+= ${INCLUDE} $(CPPFLAGS.master) -D_FILE_OFFSET_BITS=64
CFLAGS		+= $(DEBUG_CFLAGS)  ${CPPFLAGS}
SOFLAGS		+= -L$(ROOTADMINLIB) -R$(ROOTADMINLIB:$(ROOT)%=%) \
		-llogsvc $(LIBPYTHON3) -lnvpair

static:	

dynamic:	$(CPYTHONLIB)

all:		$(HDRS) dynamic

install_h:

install:	all .WAIT \
                $(ROOTPYTHONVENDOR) \
                $(ROOTPYTHONVENDORINSTALL) \
		$(ROOTPYTHONVENDORINSTALLLIBS)

lint:		lint_SRCS

include ../Makefile.targ
