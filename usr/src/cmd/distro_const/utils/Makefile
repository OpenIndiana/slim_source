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
# Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

include ../../Makefile.cmd

FILEMODE = 0555

all:=		TARGET=	all
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber
install:=	TARGET=	install

PYMODULES=	boot_archive_initialize.py \
		boot_archive_archive.py \
		grub_setup.py \
		loader_setup.py \
		im_pop.py \
		plat_setup.py \
		pre_boot_archive_pkg_image_mod.py

PYCMODULES=	$(PYMODULES:%.py=__pycache__/%.cpython$(PYTHON3_PKGVERS).pyc)

DC_UTIL_FILES=	mkrepo \
		create_iso \
		create_usb \
		gen_cd_content \
		boot_archive_configure \
		boot_archive_compress_usr \
		post_boot_archive_pkg_image_mod \
		post_boot_archive_pkg_image_mod_custom \
		generic_live.xml \
		$(PYMODULES)

# Note: PYCMODULES are not delivered.
# They are only compiled for error checking purposes.
ROOTDC_UTIL_FILES=	$(DC_UTIL_FILES:%=$(ROOTDC)/%)
$(ROOTDC)/generic_live.xml :=	FILEMODE = 0444

all:		python

clean:
		$(RM) $(PYCMODULES)

clobber: clean

install:	all .WAIT $(ROOTDC) \
		$(ROOTDC_UTIL_FILES)

python:
		$(PYTHON) -m compileall -l $(@D)


include ../../Makefile.targ
