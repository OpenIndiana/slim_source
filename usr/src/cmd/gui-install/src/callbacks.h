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
 * Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifndef __CALLBACKS_H
#define	__CALLBACKS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <orchestrator_api.h>
void 		target_discovery_callback(om_callback_info_t *cb_data,
				uintptr_t app_data);

void		on_nextbutton_clicked(GtkButton *button,
				gpointer user_data);

void		on_backbutton_clicked(GtkButton *button,
				gpointer user_data);

void		on_installbutton_clicked(GtkButton *button,
				gpointer user_data);

void		on_upgradebutton_clicked(GtkButton *button,
				gpointer user_data);

gboolean	on_quitbutton_clicked(GtkButton *button,
				gpointer user_data);

void		on_helpbutton_clicked(GtkButton *button,
				gpointer user_data);

gboolean	on_expose_event(GtkWidget *window,
				gpointer user_data);

void		on_users_entry_changed(GtkEditable *editable,
				gpointer user_data);

gboolean	on_rootpassword_focus_out_event(GtkWidget *widget,
				GdkEventFocus *event,
				gpointer user_data);

gboolean	on_userentry_focus_in_event(GtkWidget *widget,
				GdkEventFocus *event,
				gpointer user_data);

gboolean	on_userpassword_focus_out_event(GtkWidget *widget,
				GdkEventFocus *event,
				gpointer user_data);

void		on_licensecheckbutton_toggled(GtkToggleButton *togglebutton,
				gpointer user_data);

gboolean	gui_install_prompt_dialog(gboolean ok_cancel,
				gboolean set_ok_default,
				gboolean use_accept,
				GtkMessageType type,
				gchar *primary,
				gchar *secondary);

gboolean	on_loginname_focus_out_event(GtkWidget *widget,
				GdkEventFocus *event,
				gpointer user_data);

gboolean	on_username_focus_out_event(GtkWidget *widget,
				GdkEventFocus *event,
				gpointer user_data);

gboolean	on_hostname_focus_out_event(GtkWidget *widget,
				GdkEventFocus *event,
				gpointer user_data);

#ifdef __cplusplus
}
#endif

#endif /* __CALLBACKS_H */
