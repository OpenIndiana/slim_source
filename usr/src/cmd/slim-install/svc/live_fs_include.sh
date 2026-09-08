#!/bin/sh
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

# Set up the builtin commands.
builtin cd

#
# libc_mount
#
# Mount libc. If there is an optimized libc available in /usr
# that fits this processor, mount it on top of the base libc.
#
libc_mount() {
	MOE=`/usr/bin/moe -32 '/usr/lib/libc/$HWCAP'`
	if [ -n "$MOE" ]; then
		/usr/sbin/mount | egrep -s "^/lib/libc.so.1 on "
		if [ $? -ne 0 ]; then
			/usr/sbin/mount -O -F lofs $MOE /lib/libc.so.1
		fi
	fi
}

#
# Update runtime linker cache
#
update_linker_cache()
{
	if [ -f /etc/crle.conf ]
	then

		PATH=/sbin:/usr/sbin:/usr/bin:/usr/ccs/bin:/usr/X11R6/bin:/opt/DTT/bin
		export PATH

		LD_LIBRARY_PATH=/lib:/usr/lib:/usr/sfw/lib:/usr/X11R6/lib
		export LD_LIBRARY_PATH

		. /etc/crle.conf
		#/usr/bin/crle $CRLE_OPTS
	fi
}

#
# The platform profile must be created and applied at boot time, based
# on the architecture of the machine
#
apply_platform_profile()
{
	if [ -f /lib/svc/manifest/system/early-manifest-import.xml ]; then
		SMF_PROF_DIR=/etc/svc/profile
	else
		SMF_PROF_DIR=/var/svc/profile
	fi
	if [ ! -f ${SMF_PROF_DIR}/platform.xml ]; then
		this_karch=`uname -m`
		this_plat=`uname -i`

		if [ -f ${SMF_PROF_DIR}/platform_$this_plat.xml ]; then
			platform_profile=platform_$this_plat.xml
		elif [ -f ${SMF_PROF_DIR}/platform_$this_karch.xml ]; then
			platform_profile=platform_$this_karch.xml
		fi
	fi

	if [ -n "$platform_profile" ]; then
        	(cd ${SMF_PROF_DIR}; ln -s $platform_profile platform.xml)
		/usr/sbin/svccfg apply ${SMF_PROF_DIR}/platform.xml
		if [ $? -ne 0 ]; then
			echo "Failed to apply ${SMF_PROF_DIR}/platform.xml" > /dev/msglog
		fi
	fi
}
