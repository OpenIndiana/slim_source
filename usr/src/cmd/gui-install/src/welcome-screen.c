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
 * Copyright 2008 Sun Microsystems, Inc.  All rights reserved.
 * Use is subject to license terms.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gi18n.h>
#include <gtk/gtk.h>
#include "callbacks.h"
#include "installation-profile.h"
#include "interface-globals.h"
#include "window-graphics.h"
#include "welcome-screen.h"
#include "help-dialog.h"

/*
 * Signal handler connected up by Glade XML signal autoconnect
 * for the release notes button clicked event.
 */
gboolean
on_releasenotesbutton_clicked(GtkWidget *widget,
		gpointer user_data)
{
	GError *error = NULL;
	gboolean result;

	result = gtk_show_uri(gtk_widget_get_screen(widget),
		RELEASENOTESURL,
		GDK_CURRENT_TIME,
		&error);
	if (result != TRUE) {
		gui_install_prompt_dialog(
			FALSE,
			FALSE,
			FALSE,
			GTK_MESSAGE_ERROR,
			_("Unable to display release notes"),
			error->message);
		g_error_free(error);
	}
	return (TRUE);
}

void
welcome_screen_init(void)
{
	/*
	 * Welcome screen specific initialisation code.
	 */
	glade_xml_signal_autoconnect(MainWindow.welcomewindowxml);

	InstallationProfile.installationtype = INSTALLATION_TYPE_INITIAL_INSTALL;

	MainWindow.WelcomeWindow.welcomescreenvbox =
			glade_xml_get_widget(MainWindow.welcomewindowxml,
					"welcomescreenvbox");
	gtk_box_pack_start(GTK_BOX(MainWindow.screencontentvbox),
			MainWindow.WelcomeWindow.welcomescreenvbox, TRUE, TRUE, 0);

	MainWindow.WelcomeWindow.releasebutton =
			glade_xml_get_widget(MainWindow.welcomewindowxml,
					"releasenotesbutton");
	gtk_widget_grab_focus(MainWindow.WelcomeWindow.releasebutton);
}

/*
 * Set default widget with focus. For welcome screen,
 * the default widget is release notes button
 */
void
welcome_screen_set_default_focus(void)
{
	gtk_widget_grab_focus(MainWindow.WelcomeWindow.releasebutton);
}
