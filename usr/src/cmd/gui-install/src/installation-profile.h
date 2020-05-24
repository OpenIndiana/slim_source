/*
 * CDDL HEADER START
 *
 * The contents of this file are subject to the terms of the
 * Common Development and Distribution License (the "License").
 * You may not use this file except in compliance with the License.
 *
 * You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
 * or http://www.opensolaris.org/os/licensing.
 * See the License for the specific language governing permissions
 * and limitations under the License.
 *
 * When distributing Covered Code, include this CDDL HEADER in each
 * file and include the License file at usr/src/OPENSOLARIS.LICENSE.
 * If applicable, add the following below this CDDL HEADER, with the
 * fields enclosed by brackets "[]" replaced with your own identifying
 * information: Portions Copyright [yyyy] [name of copyright owner]
 *
 * CDDL HEADER END
 */
/*
 * Copyright 2007 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef __INSTALLATION_PROFILE_H
#define	__INSTALLATION_PROFILE_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <orchestrator-wrappers.h>

typedef enum {
	INSTALLATION_TYPE_INITIAL_INSTALL
	/* Extend in future for snap upgrade, live upgrade etc. */
} InstallationType;

typedef struct _InstallationProfileType {
	InstallationType installationtype;
	/* install */
	const gchar *diskname;
	/* upgrade */
	const gchar *slicename;
	gchar *disktype;
	gfloat disksize;
	gfloat installpartsize;
	disk_parts_t *partitions;

	disk_info_t *dinfo;
	upgrade_info_t *uinfo;
	char *releasename;

	struct tz_continent *continent;
	struct tz_country *country;
	struct tz_timezone *timezone;

	GList *languages;
	GList *locales;
	locale_info_t *def_locale;

	gchar *rootpassword;
	gchar *username;
	gchar *loginname;
	gchar *userpassword;
	gchar *hostname;

	gboolean installfailed;
} InstallationProfileType;

extern InstallationProfileType InstallationProfile;

#ifdef __cplusplus
}
#endif

#endif /* __INSTALLATION_PROFILE_H */
