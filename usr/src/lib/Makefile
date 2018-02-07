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
# Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
#

include $(SRC)/Makefile.master

SUBDIRS=	install_utils \
		libaiscf \
		libaiscf_pymod \
		libict  \
		libict_pymod \
		liblogsvc \
		liblogsvc_pymod \
		liborchestrator \
		libtarget_pymod \
		libtd \
		libti \
		libti_pymod \
		libtransfer \
		libtransfer_pymod \
		libzoneinfo_pymod

COMSUBDIRS=	liberrsvc_pymod \
		liberrsvc \

.PARALLEL:	$(SUBDIRS)

ROOTINC=	$(ROOT)/usr/include
ROOTINCADMIN=	$(ROOTINC)/admin

all:=		TARGET= all
check:=		TARGET= check
clean:=		TARGET= clean
clobber:=	TARGET= clobber
install:=	TARGET= install
install_h:=	TARGET= install_h
lint:=		TARGET= lint

.KEEP_STATE:

all check clean clobber lint: $(COMSUBDIRS) $(SUBDIRS)

install: .WAIT $(COMSUBDIRS) $(SUBDIRS)

headers: install_h

install_h:	$(COMSUBDIRS) $(SUBDIRS)

# library dependencies
libaiscf_pymod:		libaiscf
liblogsvc_pymod:	liblogsvc
libtransfer_pymod:	libtransfer liblogsvc
libti_pymod:		libti
liborchestrator:	libtd liblogsvc libti libtransfer_pymod libict
libict:			liblogsvc libti libtransfer_pymod
libtd:			liblogsvc
libti:			liblogsvc
libtransfer:		liblogsvc
libict_pymod:		liblogsvc
libtarget_pymod:	libtd libti


$(COMSUBDIRS) $(SUBDIRS): FRC
	cd $@; pwd; $(MAKE) $(TARGET)

FRC:
