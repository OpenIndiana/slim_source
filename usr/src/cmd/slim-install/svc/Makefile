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
# Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
#

SYSMANIFESTSRC=	live-sysidtool.xml live-io-tracing.xml

SYSFSMANIFESTSRC=	live-root-fs.xml live-usr-fs.xml \
			live-root-fs-minimal.xml

A11YMANIFESTSRC=	live-a11y.xml
                
SVCMETHODSRC=	media-fs-root live-fs-root-minimal \
		live-sysidtool-system  live-io-tracing live-a11y

SVCSHARESRC=	live_fs_include.sh

PROFILESRC=	network_nwam.xml

include $(SRC)/cmd/Makefile.cmd

FILEMODE= 444

SYSMANIFESTS= $(SYSMANIFESTSRC:%=$(ROOTMANSYS)/%)
SYSFSMANIFESTS= $(SYSFSMANIFESTSRC:%=$(ROOTMANSYSFIL)/%)
A11YMANIFESTS= $(A11YMANIFESTSRC:%=$(ROOTMANSYS)/%)
PROFILES= $(PROFILESRC:%=$(ROOTETCSVCPROFILE)/%)
SVCSHARE= $(SVCSHARESRC:%=$(ROOTLIBSVCSHARE)/%)

SVCMETHODS= $(SVCMETHODSRC:%=$(ROOTLIBSVCMETHOD)/%)
$(SVCMETHODS) :=	FILEMODE= 0555

all:

install: all $(ROOTMANSYS) $(ROOTMANSYSDEV) $(ROOTMANSYSFIL) \
	$(ROOTETCSVCPROFILE) $(ROOTLIBSVCMETHOD) $(ROOTLIBSVCSHARE) \
	$(SYSMANIFESTS) $(SYSFSMANIFESTS) \
	$(A11YMANIFESTS) $(PROFILES) $(SVCMETHODS) $(SVCSHARE)

clobber clean:

include $(SRC)/cmd/Makefile.targ
