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

#include <sys/wait.h>
#include <gdk/gdkkeysyms.h>
#include <glade/glade.h>
#include "interface-globals.h"
#include "help-dialog.h"
#include "finish-screen.h"
#include "window-graphics.h"

static const gchar *labelmarkup = "<span font_desc=\"Bold\">%s</span>";
static const gchar *buttonmarkup = "<span foreground=\"#5582a3\">%s</span>";

void
finish_xml_init(void)
{
	MainWindow.finishxml = glade_xml_new(GLADEDIR "/" FILENAME, FINISHNODE, NULL);
	MainWindow.FinishWindow.finishbox =
	    glade_xml_get_widget(MainWindow.finishxml, "finishbox");
	MainWindow.FinishWindow.finishlabel =
	    glade_xml_get_widget(MainWindow.finishxml, "finishlabel1");
	MainWindow.FinishWindow.logbuttonlabel =
	    glade_xml_get_widget(MainWindow.finishxml, "logbuttonlabel");
}

gboolean
on_key_press_event(GtkWidget *widget,
				GdkEventKey *event,
				gpointer user_data)
{
	switch (event->keyval) {
	case	GDK_Return:
	case	GDK_space:
		g_warning("key activated");
		break;
	}

	return (FALSE);
}

static void
installation_log_hide(GtkWidget *widget, gpointer *dialog)
{
	/*
	 * If the main installer window is still up then just hide the
	 * dialog. If it's hidden the it means the user hit quit and
	 * the GUI is waiting for the user to close the install log
	 * dialog before exiting completely.
	 */
	gtk_widget_hide(GTK_WIDGET(dialog));
	if (!GTK_WIDGET_VISIBLE(MainWindow.mainwindow))
		exit(0);
}

static void
installation_log_delete_event(GtkWidget *widget, gpointer *user_data)
{
	gtk_widget_hide(widget);
}

gboolean
on_logbutton_clicked(GtkWidget *widget,
				gpointer user_data)
{
	static gboolean initialised = FALSE;

	if (initialised == FALSE) {
		switch (InstallationProfile.installationtype) {
			case INSTALLATION_TYPE_INITIAL_INSTALL:
				show_locale_file_in_textview(
				    MainWindow.FinishWindow.installationlogtextview,
				    MainWindow.TextFileLocations[INSTALL_LOG],
				    FALSE, FALSE, TRUE);
				break;
		}
		initialised = TRUE;
	}
	window_graphics_dialog_set_properties(
			MainWindow.FinishWindow.installationlogdialog);
	gtk_widget_show(MainWindow.FinishWindow.installationlogdialog);
	return (TRUE);
}


static void
installation_log_init(void)
{
	MainWindow.FinishWindow.installationlogxml =
			glade_xml_new(GLADEDIR "/" FILENAME, INSTALLATIONLOGNODE, NULL);

	MainWindow.FinishWindow.installationlogclosebutton =
		glade_xml_get_widget(MainWindow.FinishWindow.installationlogxml,
				"textviewclosebutton");
	MainWindow.FinishWindow.installationlogdialog =
		glade_xml_get_widget(MainWindow.FinishWindow.installationlogxml,
				"textviewdialog");
	MainWindow.FinishWindow.installationlogtextview =
		glade_xml_get_widget(MainWindow.FinishWindow.installationlogxml,
				"textview");

	switch (InstallationProfile.installationtype) {
		case INSTALLATION_TYPE_INITIAL_INSTALL:
			gtk_window_set_title(GTK_WINDOW(
			    MainWindow.FinishWindow.installationlogdialog),
			    _("Installation Log"));
			break;
	}
	g_signal_connect(G_OBJECT(MainWindow.FinishWindow.installationlogclosebutton), "clicked",
			G_CALLBACK(installation_log_hide),
			MainWindow.FinishWindow.installationlogdialog);
	g_signal_connect(G_OBJECT(MainWindow.FinishWindow.installationlogdialog),
			"delete-event",
			G_CALLBACK(installation_log_delete_event),
			MainWindow.FinishWindow.installationlogdialog);
}

void
finish_ui_init(void)
{
	glade_xml_signal_autoconnect(MainWindow.finishxml);

	gtk_box_pack_start(GTK_BOX(MainWindow.screencontentvbox),
					MainWindow.FinishWindow.finishbox, TRUE, TRUE, 0);
}

void
finish_screen_set_contents(void)
{
	gchar *labelstr;
	gchar *buttonstr;

	switch (InstallationProfile.installationtype) {
		case INSTALLATION_TYPE_INITIAL_INSTALL:
			labelstr = g_strdup_printf(labelmarkup,
			    _("OpenIndiana installation is complete. Review the "
			    "OpenIndiana installation log for more information"));
			buttonstr = g_strdup_printf(buttonmarkup,
			    _("OpenIndiana installation log"));
			break;
	}
	gtk_label_set_label(GTK_LABEL(
	    MainWindow.FinishWindow.finishlabel),
	    labelstr);
	gtk_label_set_label(GTK_LABEL(
	    MainWindow.FinishWindow.logbuttonlabel),
		buttonstr);
	g_free(labelstr);
	g_free(buttonstr);
	installation_log_init();
}
