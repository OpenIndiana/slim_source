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

#ifndef __HELP_DIALOG_H
#define	__HELP_DIALOG_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#define	MAXBUFFER 12800

void		help_dialog_show(InstallScreen currScreen,
				gboolean bringToFront);

void		help_dialog_hide(GtkWidget *widget,
				gpointer *dialog);

void		help_dialog_delete_event(GtkWidget *widget,
				gpointer user_data);

gboolean	show_file_in_textview(GtkWidget *textview,
				gchar *filename,
				gboolean bold,
				gboolean centered,
				gboolean dontprocessCR);

gboolean	show_locale_file_in_textview(GtkWidget *textview,
				gchar *filename,
				gboolean bold,
				gboolean centered,
				gboolean dontprocessCR);

void		delete_textview_contents(GtkWidget *textview);

gchar*		help_generate_file_path(gchar *path,
				gchar *locale_id,
				gchar *filename);

void		help_dialog_refresh(InstallScreen currscreen);

#ifdef __cplusplus
}
#endif

#endif /* __HELP_DIALOG_H */
