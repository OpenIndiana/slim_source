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

include ../Makefile.cmd

all:=		TARGET=	all
clean:=		TARGET=	clean
clobber:=	TARGET=	clobber
install:=	TARGET=	install

SUBDIRS=	utils slim_cd text_install loader

PROGS=		distro_const

PYMODULES=	dc_checkpoint.py \
		__init__.py \
		dc_defs.py \
		dc_ti.py \
		ValidatorModule.py \
		DefaultsModule.py \
		dc_utils.py

PYCMODULES=	$(PYMODULES:%.py=%.pyc)

MANIFEST_FILES= DC-manifest.defval.xml \
		DC-manifest.rng

PYTHON_EXECS=	finalizer_checkpoint.py \
		finalizer_rollback.py

ROOTPROGS=	$(PROGS:%=$(ROOTUSRBIN)/%)

ROOTPYMODULES=	$(PYMODULES:%=$(ROOTPYTHONVENDORINSTALLDC)/%)

ROOTPYCMODULES=	$(PYCMODULES:%=$(ROOTPYTHONVENDORINSTALLDC)/%)

ROOTMANIFESTS=	$(MANIFEST_FILES:%=$(ROOTDC)/%)

ROOTPYTHONEXECS= $(PYTHON_EXECS:%=$(ROOTDC)/%)


all:		python $(PROGS)

clean:	$(SUBDIRS)
	$(RM) $(PROGS) *.pyc

clobber: clean

install: all .WAIT $(ROOTPROGS) \
	$(ROOTPYTHONVENDOR) \
	$(ROOTPYTHONVENDORINSTALL) \
	$(ROOTPYTHONVENDORINSTALLDC) \
	$(ROOTPYMODULES) \
	$(ROOTPYCMODULES) \
	$(ROOTDC) \
	$(ROOTMANIFESTS) \
	$(ROOTPYTHONEXECS) \
	$(SUBDIRS)

python:
	$(PYTHON) -m compileall -l $(@D)

distro_const: distro_const.py
	$(CP) distro_const.py distro_const
	$(CHMOD) 755 distro_const

$(SUBDIRS): FRC
	cd $@; pwd; echo $(TARGET); $(MAKE) $(TARGET)

FRC:

include ../Makefile.targ
