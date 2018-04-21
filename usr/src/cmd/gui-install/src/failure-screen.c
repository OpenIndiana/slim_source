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
 * Copyright (c) 2007, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <gtk/gtk.h>
#include <glade/glade-build.h>
#include <ctype.h>
#include "installation-profile.h"
#include "interface-globals.h"
#include "failure-screen.h"
#include "callbacks.h"
#include "help-dialog.h"
#include "window-graphics.h"

static const gchar *buttonmarkup = "<span foreground=\"#5582a3\">%s</span>";
static const gchar *labelmarkup = "<span font_desc=\"Bold\">%s</span>";

void
failure_window_init(void)
{
	if (!MainWindow.failurewindowxml) {
		g_warning("Failed to access Failure Window.");
		exit(-1);
	}

	glade_xml_signal_autoconnect(MainWindow.failurewindowxml);

	MainWindow.FailureWindow.failurewindowtable = NULL;
	MainWindow.FailureWindow.failureinfolabel = NULL;
	MainWindow.FailureWindow.failuredetaillabel = NULL;
	MainWindow.FailureWindow.logbuttonlabel = NULL;

	MainWindow.FailureWindow.installlogdialog = NULL;
	MainWindow.FailureWindow.installlogclosebutton = NULL;
	MainWindow.FailureWindow.installlogtextview = NULL;
}

static void
install_log_hide(GtkWidget *widget, gpointer *dialog)
{
	/*
	 * If the main installer window is still up then just hide the
	 * dialog. If it's hidden then it means the user hit quit and
	 * the GUI is waiting for the user to close the install log
	 * dialog before exiting completely.
	 */
	gtk_widget_hide(GTK_WIDGET(dialog));
	if (!GTK_WIDGET_VISIBLE(MainWindow.mainwindow))
		exit(1);
}

static void
install_log_delete_event(GtkWidget *widget, gpointer *user_data)
{
	gtk_widget_hide(widget);
}

gboolean
on_failurelogbutton_clicked(GtkWidget *widget,
						gpointer user_data)
{
	static gboolean initialised = FALSE;

	if (initialised == FALSE) {
		switch (InstallationProfile.installationtype) {
			case INSTALLATION_TYPE_INITIAL_INSTALL:
				show_locale_file_in_textview(
				    MainWindow.FailureWindow.installlogtextview,
				    MainWindow.TextFileLocations[INSTALL_LOG],
				    FALSE, FALSE, TRUE);
				break;
		}

		initialised = TRUE;
	}
	window_graphics_dialog_set_properties(
			MainWindow.FailureWindow.installlogdialog);
	gtk_widget_show(MainWindow.FailureWindow.installlogdialog);
	return (TRUE);
}

static void
installation_log_init(void)
{
	MainWindow.FailureWindow.installlogxml = glade_xml_new(
			GLADEDIR "/" FILENAME, INSTALLLOGNODE, NULL);

	MainWindow.FailureWindow.installlogdialog =
		glade_xml_get_widget(MainWindow.FailureWindow.installlogxml,
							"textviewdialog");
	MainWindow.FailureWindow.installlogclosebutton =
		glade_xml_get_widget(MainWindow.FailureWindow.installlogxml,
							"textviewclosebutton");
	MainWindow.FailureWindow.installlogtextview =
		glade_xml_get_widget(MainWindow.FailureWindow.installlogxml,
							"textview");

	switch (InstallationProfile.installationtype) {
		case INSTALLATION_TYPE_INITIAL_INSTALL:
			gtk_window_set_title(GTK_WINDOW(
			    MainWindow.FailureWindow.installlogdialog),
			    _("Installation Log"));
			break;
	}

	g_signal_connect(
			G_OBJECT(MainWindow.FailureWindow.installlogclosebutton),
			"clicked",
			G_CALLBACK(install_log_hide),
			MainWindow.FailureWindow.installlogdialog);
	g_signal_connect(
			G_OBJECT(MainWindow.FailureWindow.installlogdialog),
			"delete-event",
			G_CALLBACK(install_log_delete_event),
			MainWindow.FailureWindow.installlogdialog);
}

void
failure_screen_load_widgets(void)
{
	MainWindow.FailureWindow.failurewindowtable =
		glade_xml_get_widget(MainWindow.failurewindowxml,
		    "failurewindowtable");
	MainWindow.FailureWindow.failureinfolabel =
		glade_xml_get_widget(MainWindow.failurewindowxml,
		    "failureinfolabel");
	MainWindow.FailureWindow.failuredetaillabel =
		glade_xml_get_widget(MainWindow.failurewindowxml,
		    "failuredetaillabel");
	MainWindow.FailureWindow.logbuttonlabel =
		glade_xml_get_widget(MainWindow.failurewindowxml,
		    "logbuttonlabel");
}

void
failure_screen_set_contents(void)
{
	gchar *labelstr;
	gchar *buttonstr;

	switch (InstallationProfile.installationtype) {
		case INSTALLATION_TYPE_INITIAL_INSTALL:
			labelstr = g_strdup_printf(labelmarkup,
			    _("OpenIndiana installation did not complete normally."));
			buttonstr = g_strdup_printf(buttonmarkup,
			    _("OpenIndiana installation log"));
			break;
	}
	gtk_label_set_label(GTK_LABEL(
	    MainWindow.FailureWindow.failureinfolabel),
	    labelstr);
	gtk_label_set_label(GTK_LABEL(
	    MainWindow.FailureWindow.logbuttonlabel),
	    buttonstr);
	g_free(labelstr);
	g_free(buttonstr);
	installation_log_init();
}
