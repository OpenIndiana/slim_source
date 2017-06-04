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

JACKCONFIGAUTOFILESRC = iotrace.desktop ddu_silent.desktop
JACKFILESRC644 = xorg.conf.vesa
JACKDESKTOPFILESRC = 	install_openindiana.desktop ddu.desktop \
			.os-icons-installed gparted.desktop \
			text_install_openindiana.desktop
JACKDOTFILESRC = Xclients bashrc profile xscreensaver dmrc

include $(SRC)/cmd/slim-install/Makefile.slim

JACKFILES644= $(JACKFILESRC644:%=$(ROOTJACK)/%)
$(JACKFILES644) :=	FILEMODE= 0644

JACKCONFIGAUTOFILES= $(JACKCONFIGAUTOFILESRC:%=$(ROOTJACKCONFIGAUTO)/%)
$(JACKCONFIGAUTOFILES) :=      FILEMODE= 0644

JACKDESKTOPFILES= $(JACKDESKTOPFILESRC:%=$(ROOTJACKDESKTOP)/%)
$(JACKDESKTOPFILES) :=      FILEMODE= 0755

JACKDOTFILES= $(JACKDOTFILESRC:%=$(ROOTJACK)/.%)
$(JACKDOTFILES) :=	FILEMODE= 0755

all: 

install: all $(ROOTJACK) $(ROOTJACKCONFIGAUTO) $(ROOTJACKDESKTOP) \
	$(JACKFILES644) $(JACKDOTFILES) $(JACKCONFIGAUTOFILES) \
	$(JACKDESKTOPFILES)

$(ROOTJACK) $(ROOTJACKCONFIGAUTO) $(ROOTJACKDESKTOP):
	$(INS.dir)

$(ROOTJACK)/.%: % $(ROOTJACK)
	$(INS.rename)

$(ROOTJACK)/%: % $(ROOTJACK)
	$(INS.file)

$(ROOTJACKCONFIGAUTO)/%: % $(ROOTJACKCONFIGAUTO)
	$(INS.file)

$(ROOTJACKDESKTOP)/%: % $(ROOTJACKDESKTOP)
	$(INS.file)

clobber clean:

.os-icons-installed:
	$(TOUCH) $@
