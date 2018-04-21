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
#include "installation-profile.h"
#include "interface-globals.h"
#include "confirmation-screen.h"
#include "users-screen.h"
#include "callbacks.h"
#include "help-dialog.h"
#include "window-graphics.h"
#include "language-screen.h"
#include "installation-screen.h"

gchar *ConfirmSectionHeaderMarkup = "<span font_desc=\"Arial Bold\">%s</span>";
gchar *ConfirmSectionDetailMarkup =
	"<span font_desc=\"Arial Bold\">&#8226; </span><span font_desc=\"Arial\">%s</span>";
gchar *ConfirmSectionWarningMarkup = "<span size=\"smaller\">%s</span>";
gchar *ConfirmSectionIndentDetailMarkup =
	"<span font_desc=\"Arial Bold\">    &#8226; </span><span font_desc=\"Arial\">%s</span>";

void
confirmation_window_init(void)
{
	if (!MainWindow.confirmationwindowxml) {
		g_warning("Failed to access Confirmation Window.");
		exit(-1);
	}

	glade_xml_signal_autoconnect(MainWindow.confirmationwindowxml);

	MainWindow.ConfirmationWindow.confirmationtoplevel = NULL;
	MainWindow.ConfirmationWindow.infolabel = NULL;
	MainWindow.ConfirmationWindow.confirmmainvbox = NULL;
	MainWindow.ConfirmationWindow.confirmscrolledwindow = NULL;
	MainWindow.ConfirmationWindow.confirmviewport = NULL;
	MainWindow.ConfirmationWindow.confirmdetailvbox = NULL;
	MainWindow.ConfirmationWindow.diskvbox = NULL;
	MainWindow.ConfirmationWindow.softwarevbox = NULL;
	MainWindow.ConfirmationWindow.timezonevbox = NULL;
	MainWindow.ConfirmationWindow.languagesvbox = NULL;
	MainWindow.ConfirmationWindow.accountvbox = NULL;

	MainWindow.ConfirmationWindow.licensecheckbutton = NULL;
	MainWindow.ConfirmationWindow.licenseagreementlinkbutton = NULL;
	MainWindow.ConfirmationWindow.licenseagreementdialog = NULL;
	MainWindow.ConfirmationWindow.licenseagreementclosebutton = NULL;
	MainWindow.ConfirmationWindow.licenseagreementtextview = NULL;
}

static void
license_agreement_hide(GtkWidget *widget, gpointer *dialog)
{
	gtk_widget_hide(GTK_WIDGET(dialog));
}

static void
license_agreement_delete_event(GtkWidget *widget, gpointer *user_data)
{
	gtk_widget_hide(widget);
}

static void
license_agreement_show(GtkButton *button,
						gpointer data)
{
	if (MainWindow.TextFileLocations[LICENSE_AGREEMENT]) {
		delete_textview_contents(
			MainWindow.ConfirmationWindow.licenseagreementtextview);
		show_locale_file_in_textview(
			MainWindow.ConfirmationWindow.licenseagreementtextview,
			MainWindow.TextFileLocations[LICENSE_AGREEMENT],
			TRUE, FALSE, FALSE);
		window_graphics_dialog_set_properties(
			MainWindow.ConfirmationWindow.licenseagreementdialog);
		gtk_widget_show(MainWindow.ConfirmationWindow.licenseagreementdialog);
	}
}

static void
license_agreement_setup(void)
{
	MainWindow.ConfirmationWindow.licensecheckbutton =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
						"licensecheckbutton");
	MainWindow.ConfirmationWindow.licenseagreementlinkbutton =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
						"licenseagreementlinkbutton");
	g_signal_connect(
		G_OBJECT(MainWindow.ConfirmationWindow.licenseagreementlinkbutton),
		"clicked",
		G_CALLBACK(license_agreement_show),
		NULL);

	MainWindow.ConfirmationWindow.licenseagreementxml = glade_xml_new(
		GLADEDIR "/" CONFIRMATIONFILENAME, LICENSEAGREEMENTNODE, NULL);

	MainWindow.ConfirmationWindow.licenseagreementdialog =
		glade_xml_get_widget(MainWindow.ConfirmationWindow.licenseagreementxml,
					"licenseagreementdialog");
	MainWindow.ConfirmationWindow.licenseagreementclosebutton =
		glade_xml_get_widget(MainWindow.ConfirmationWindow.licenseagreementxml,
					"licenseagreementclosebutton");
	MainWindow.ConfirmationWindow.licenseagreementtextview =
		glade_xml_get_widget(MainWindow.ConfirmationWindow.licenseagreementxml,
					"licenseagreementtextview");
	g_signal_connect(
		G_OBJECT(MainWindow.ConfirmationWindow.licenseagreementclosebutton),
		"clicked",
		G_CALLBACK(license_agreement_hide),
		MainWindow.ConfirmationWindow.licenseagreementdialog);
	g_signal_connect(
		G_OBJECT(MainWindow.ConfirmationWindow.licenseagreementdialog),
		"delete-event",
		G_CALLBACK(license_agreement_delete_event),
		MainWindow.ConfirmationWindow.licenseagreementdialog);
}

void
confirmation_load_widgets(void)
{
	GdkColor colour;

	MainWindow.ConfirmationWindow.confirmationtoplevel =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
					"confirmationtoplevel");
	MainWindow.ConfirmationWindow.infolabel =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
					"infolabel");
	MainWindow.ConfirmationWindow.confirmmainvbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
					"confirmmainvbox");
	MainWindow.ConfirmationWindow.confirmscrolledwindow =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
					"confirmscrolledwindow");
	MainWindow.ConfirmationWindow.confirmviewport =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
					"confirmviewport");
	MainWindow.ConfirmationWindow.confirmdetailvbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
					"confirmdetailvbox");

	/* Set background color */
	colour.pixel = 0;
	colour.red = colour.green = colour.blue = WHITE_GDK_COLOR;
	gtk_widget_modify_bg(
				MainWindow.ConfirmationWindow.confirmviewport,
				GTK_STATE_NORMAL,
				&colour);

	MainWindow.ConfirmationWindow.diskvbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
							"diskvbox");
	MainWindow.ConfirmationWindow.softwarevbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
							"softwarevbox");
	MainWindow.ConfirmationWindow.timezonevbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
							"timezonevbox");
	MainWindow.ConfirmationWindow.languagesvbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
							"languagesvbox");
	MainWindow.ConfirmationWindow.accountvbox =
		glade_xml_get_widget(MainWindow.confirmationwindowxml,
							"accountvbox");

	license_agreement_setup();
}

static void
set_detail_label(GtkWidget *label, gchar *markUp, gchar *str)
{
	gchar *tmpStr;

	tmpStr = g_markup_printf_escaped(markUp, str);
	gtk_label_set_markup(GTK_LABEL(label), tmpStr);
	g_free(tmpStr);
}

static void
add_detail_hbox(GtkWidget *detailVBox,
					gboolean includeWarning,
					gboolean indent,
					gchar *labelStr,
					gchar *warningStr)
{
	GtkWidget *detailHBox = NULL;
	GtkWidget *detailLabel = NULL;
	GtkWidget *detailImage = NULL;
	GtkWidget *detailWarning = NULL;

	/* Create a new Hbox widget containing three children */
	detailHBox = gtk_hbox_new(FALSE, 5);
	detailLabel = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(detailLabel), TRUE);
	gtk_misc_set_padding(GTK_MISC(detailLabel), 10, 0);
	detailImage = gtk_image_new_from_stock(GTK_STOCK_DIALOG_WARNING,
						GTK_ICON_SIZE_MENU);
	detailWarning = gtk_label_new(NULL);
	gtk_label_set_selectable(GTK_LABEL(detailWarning), TRUE);

	gtk_box_pack_start(GTK_BOX(detailHBox), detailLabel, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(detailHBox), detailImage, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(detailHBox), detailWarning, FALSE, TRUE, 0);
	gtk_box_pack_start(GTK_BOX(detailVBox), detailHBox, FALSE, FALSE, 0);
	gtk_widget_show_all(detailHBox);

	if (indent) {
		set_detail_label(detailLabel,
				ConfirmSectionIndentDetailMarkup,
				labelStr);
	} else {
		set_detail_label(detailLabel,
				ConfirmSectionDetailMarkup,
				labelStr);
	}

	if (includeWarning && warningStr) {
		set_detail_label(detailWarning,
				ConfirmSectionWarningMarkup,
				warningStr);
		gtk_widget_show(detailImage);
		gtk_widget_show(detailWarning);
	} else {
		gtk_widget_hide(detailImage);
		gtk_widget_hide(detailWarning);
	}
}

static void
remove_detail_hbox(GtkWidget *child, gpointer user_data)
{
	if (strncmp(gtk_widget_get_name(child), "GtkHBox", 7) == 0) {
		gtk_widget_hide(child);
		gtk_widget_destroy(child);
	}
}

static void
remove_detail_widgets(void)
{
	gtk_container_foreach(
		GTK_CONTAINER(MainWindow.ConfirmationWindow.diskvbox),
		remove_detail_hbox, NULL);
	gtk_container_foreach(
		GTK_CONTAINER(MainWindow.ConfirmationWindow.softwarevbox),
		remove_detail_hbox, NULL);
	gtk_container_foreach(
		GTK_CONTAINER(MainWindow.ConfirmationWindow.timezonevbox),
		remove_detail_hbox, NULL);
	gtk_container_foreach(
		GTK_CONTAINER(MainWindow.ConfirmationWindow.languagesvbox),
		remove_detail_hbox, NULL);
	gtk_container_foreach(
		GTK_CONTAINER(MainWindow.ConfirmationWindow.accountvbox),
		remove_detail_hbox, NULL);
}

void
confirmation_screen_set_contents(void)
{
	gchar *tmpStr;
	gchar *tmpStr2;
	gchar *tmpErr;
	gfloat diskSize, partitionSize, minsize;
	static gboolean firstTimeHere = FALSE;

	if (firstTimeHere == FALSE) {
		firstTimeHere = TRUE;
	} else {
		remove_detail_widgets();
	}

	switch (InstallationProfile.installationtype) {
		case INSTALLATION_TYPE_INITIAL_INSTALL:
			/* Disk Information */
			gtk_widget_show(MainWindow.ConfirmationWindow.diskvbox);

			/* Disk info should be contained in the structure */
			/* InstallationProfile..installationdisk */

			/* Disk Info */
			/* Disk name : idisk->dinfo.diskName */
			/* Disk size : idisk->dinfo.diskSize */
			/* Disk type : idisk->dinfo.diskType */
			/* Disk vend : idisk->dinfo.vendor */
			/* Disk boot : idisk->dinfo.bootDisk */
			/* Disk labe : idisk->dinfo.label */

			/* Slice Info */
			/* Part Id : idisk->dslices->partitionId */
			/* diskname : idisk->dslices->diskName */
			/* NDKMAP slices */
			/* Id : idisk->dslices->sinfo[0].sliceId */
			/* size : idisk->dslices->sinfo[0].sliceSize */
			/* mount : idisk->dslices->sinfo[0].mountPoint */
			/* tag : idisk->dslices->sinfo[0].tag */
			/* flags : idisk->dslices->sinfo[0].flags */

			diskSize = InstallationProfile.disksize;
			partitionSize = InstallationProfile.installpartsize;

			if (partitionSize == 0) {
				partitionSize = diskSize;
			}

			if (diskSize == partitionSize) {
				if (InstallationProfile.diskname != NULL) {
					tmpStr =
						g_strdup_printf(
							_("%.1f GB disk (%s)"),
							diskSize,
							InstallationProfile.diskname);
				} else {
					tmpStr =
						g_strdup_printf(
							_("%.1f GB disk"),
							diskSize);
				}
				tmpErr =
					g_strdup(_("This disk will be erased"));
			} else {
				if (InstallationProfile.diskname != NULL) {
					tmpStr =
						g_strdup_printf(
							_("%.1f GB partition on %.1f GB disk (%s)"),
							partitionSize,
							diskSize,
							InstallationProfile.diskname);
				} else {
					tmpStr =
						g_strdup_printf(
							_("%.1f GB partition on %.1f GB disk"),
							partitionSize,
							diskSize);
				}
				tmpErr =
					g_strdup(
						_("This partition will be erased"));
			}

			add_detail_hbox(
				MainWindow.ConfirmationWindow.diskvbox,
				TRUE, FALSE,
				_(tmpStr),
				_(tmpErr));
			g_free(tmpStr);
			g_free(tmpErr);

			minsize = orchestrator_om_get_mininstall_sizegb(TRUE);
			tmpStr = g_strdup_printf(
				_("The whole installation will take up %.1fGB hard disk space."),
				minsize);
			add_detail_hbox(
				MainWindow.ConfirmationWindow.diskvbox,
				FALSE, FALSE,
				tmpStr,
				NULL);
			g_free(tmpStr);

			/* Software Information */
			gtk_widget_show(
				MainWindow.ConfirmationWindow.softwarevbox);

			add_detail_hbox(
				MainWindow.ConfirmationWindow.softwarevbox,
				FALSE, FALSE,
				_("OpenIndiana"),
				NULL);

			/* Timezone Information */
			if (InstallationProfile.timezone) {
				gtk_widget_show(
					MainWindow.ConfirmationWindow.timezonevbox);

				add_detail_hbox(
					MainWindow.ConfirmationWindow.timezonevbox,
					FALSE, FALSE,
					InstallationProfile.timezone->tz_name,
					NULL);
			}

			/* Language Support Information */
			gtk_widget_show(
				MainWindow.ConfirmationWindow.languagesvbox);

			if (InstallationProfile.def_locale) {
				tmpStr2 = NULL;
				tmpStr2 =
					g_strdup(
						orchestrator_om_locale_get_desc(
							InstallationProfile.def_locale));
				if (!tmpStr2) {
					tmpStr2 =
						g_strdup(orchestrator_om_locale_get_name(
							InstallationProfile.def_locale));
					g_warning(
						"Default language error: no locale description "
						"for locale: %s",
						tmpStr2);
				}
			}

			tmpStr = g_strdup_printf(_("Default Language: %s"),
						InstallationProfile.def_locale ?
						tmpStr2 : _("C/Posix"));
			g_free(tmpStr2);

			add_detail_hbox(
				MainWindow.ConfirmationWindow.languagesvbox,
				FALSE, FALSE,
				tmpStr,
				NULL);
			g_free(tmpStr);

			tmpStr = g_strdup(_("Language Support:"));
			construct_language_string(&tmpStr, TRUE, ' ');

			add_detail_hbox(
				MainWindow.ConfirmationWindow.languagesvbox,
				FALSE, FALSE, tmpStr, NULL);
			g_free(tmpStr);

			/* Accounts Information */
			gtk_widget_show(
				MainWindow.ConfirmationWindow.accountvbox);

			if (!InstallationProfile.rootpassword) {
				add_detail_hbox(
					MainWindow.ConfirmationWindow.accountvbox,
					TRUE, FALSE,
					_("Root Account:"),
					_("A Root password is not defined. The system is unsecured."));
			}

			if (!InstallationProfile.loginname) {
				add_detail_hbox(
					MainWindow.ConfirmationWindow.accountvbox,
					TRUE, FALSE,
					_("User Account:"),
					_("No user account."));
			} else {
				tmpStr =
					g_strdup_printf(
						_("User Account: %s"),
						InstallationProfile.loginname);
				add_detail_hbox(
					MainWindow.ConfirmationWindow.accountvbox,
					FALSE, FALSE,
					_(tmpStr),
					NULL);
				g_free(tmpStr);
			}

			if (InstallationProfile.hostname) {
				tmpStr =
					g_strdup_printf(
						_("Host name: %s"),
						InstallationProfile.hostname);
				add_detail_hbox(
					MainWindow.ConfirmationWindow.accountvbox,
					FALSE, FALSE,
					_(tmpStr),
					NULL);
				g_free(tmpStr);
			}
			break;

	}
#ifdef POST_PREVIEW_RELEASE
	gtk_toggle_button_set_active(
		GTK_TOGGLE_BUTTON(
			MainWindow.ConfirmationWindow.licensecheckbutton),
		FALSE);
#endif /* POST_PREVIEW_RELEASE */

	if (installation_get_dummy_install() == TRUE) {
		g_debug("Performing DUMMY Install\n");
	} else {
		g_debug("Performing REAL Install\n");
	}
}

gboolean
confirmation_agree_license(void)
{
	gboolean ret_val = TRUE;

	if (!gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(
				MainWindow.ConfirmationWindow.licensecheckbutton))) {
		ret_val =
			gui_install_prompt_dialog(
				TRUE,
				FALSE,
				TRUE,
				GTK_MESSAGE_WARNING,
				_("Do you accept the terms of the license agreement ?"),
				_("To review the license agreement click Cancel, then click 'Review license agreement'."));
	}
	return (ret_val);
}

gboolean
confirmation_check_label_button_release(GtkWidget *widget,
						GdkEvent *event,
						gpointer data)
{
	if (gtk_toggle_button_get_active(
			GTK_TOGGLE_BUTTON(
				MainWindow.ConfirmationWindow.licensecheckbutton))) {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(
				MainWindow.ConfirmationWindow.licensecheckbutton),
			FALSE);
	} else {
		gtk_toggle_button_set_active(
			GTK_TOGGLE_BUTTON(
				MainWindow.ConfirmationWindow.licensecheckbutton),
			TRUE);
	}
	return (TRUE);
}

/*
 * Set the default widget with focus.
 * The default widget for confirmation screen
 * is the 1st label.
 */
void
confirmation_screen_set_default_focus(void)
{
	GList *l;

	/* 1st child is a hbox */
	l = gtk_container_get_children(
		GTK_CONTAINER(MainWindow.ConfirmationWindow.diskvbox));
	if (l) {
		/* 1st child of the hbox is the label */
		l = gtk_container_get_children(GTK_CONTAINER(l->data));
		if (l) {
			gtk_widget_grab_focus(GTK_WIDGET(l->data));
		}
	}
}
