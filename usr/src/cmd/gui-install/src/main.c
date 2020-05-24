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

#include <unistd.h>
#include <signal.h>
#include <locale.h>

#include <gtk/gtk.h>
#include <glade/glade.h>

#include "callbacks.h"
#include "interface-globals.h"
#include "welcome-screen.h"
#include "window-graphics.h"
#include "users-screen.h"
#include "confirmation-screen.h"
#include "installation-screen.h"
#include "failure-screen.h"
#include "upgrade-screen.h"
#include "finish-screen.h"
#include "orchestrator-wrappers.h"
#include "help-dialog.h"
#include "language-screen.h"
#include "disk-block-order.h"
#include "error-logging.h"

gboolean waitforsignal = FALSE;

MainWindowXML MainWindow;

/* Pango markup for the screen title and stage labels */
gchar *ScreenTitleMarkup = NULL;
gchar *ScreenSubTitleMarkup = NULL;
gchar *ActiveStageTitleMarkup = NULL;
gchar *InactiveStageTitleMarkup = NULL;

om_handle_t      omhandle;

static void
catch_sigusr(int sig_num)
{
	waitforsignal = FALSE;
}

static void
mainwindow_xml_screentitles_init(void)
{
	ScreenTitleMarkup =
		"<span font_desc=\"Bold\" size=\"x-large\" foreground=\"#587993\">%s</span>";
	ScreenSubTitleMarkup =
		"<span font_desc=\"Bold\">%s</span>";

	MainWindow.ScreenTitles = g_new0(gchar*, NUMSCREENS);
	MainWindow.ScreenTitles[WELCOME_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Welcome"));
	MainWindow.ScreenTitles[DISK_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Disk"));
	MainWindow.ScreenTitles[TIMEZONE_SCREEN] =
		g_strdup_printf(
			ScreenTitleMarkup,
			_("Time Zone, Date and Time"));
	MainWindow.ScreenTitles[LANGUAGE_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Locale"));
	MainWindow.ScreenTitles[USER_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Users"));
	/*
	 * Confirmation and Installation titles label will be modified depending
	 * on if whether the user selects install or upgrade from the
	 * welcome screen
	 */
	MainWindow.ScreenTitles[CONFIRMATION_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Installation"));
	MainWindow.ScreenTitles[INSTALLATION_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Installing"));
	MainWindow.ScreenTitles[FAILURE_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Installation Failed"));
	MainWindow.ScreenTitles[FINISH_SCREEN] =
		g_strdup_printf(ScreenTitleMarkup, _("Finished"));

	/* Secondary Titles */
	MainWindow.ScreenSubTitles = g_new0(gchar*, NUMSCREENS);
	MainWindow.ScreenSubTitles[WELCOME_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup,
			_("OpenIndiana"));
	MainWindow.ScreenSubTitles[DISK_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup,
			_("Where should OpenIndiana be installed?"));
	MainWindow.ScreenSubTitles[TIMEZONE_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup,
			_("Select a city near you on the map or set your time zone below,then set the date and time."));
	MainWindow.ScreenSubTitles[LANGUAGE_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup,
			_("Set the default language support and locale specific data formats."));
	MainWindow.ScreenSubTitles[USER_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup, _(" "));
	MainWindow.ScreenSubTitles[CONFIRMATION_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup,
			_("Review the settings below before installing. Click the back button to make changes."));
	MainWindow.ScreenSubTitles[INSTALLATION_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup, _(" "));
	MainWindow.ScreenSubTitles[FAILURE_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup, _(" "));
	MainWindow.ScreenSubTitles[FINISH_SCREEN] =
		g_strdup_printf(ScreenSubTitleMarkup, _(" "));
}

static void
mainwindow_xml_stagetitles_init(void)
{
	ActiveStageTitleMarkup =
		"<span font_desc=\"Bold\" foreground=\"#587993\">%s</span>";
	InactiveStageTitleMarkup =
		"<span font_desc=\"Bold\" foreground=\"#595A5E\">%s</span>";

	MainWindow.ActiveStageTitles = g_new0(gchar*, NUMSCREENS);
	MainWindow.InactiveStageTitles = g_new0(gchar*, NUMSCREENS);

	MainWindow.ActiveStageTitles[WELCOME_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Welcome"));
	MainWindow.InactiveStageTitles[WELCOME_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Welcome"));

	MainWindow.ActiveStageTitles[DISK_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Disk"));
	MainWindow.InactiveStageTitles[DISK_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Disk"));

	MainWindow.ActiveStageTitles[TIMEZONE_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Time Zone"));
	MainWindow.InactiveStageTitles[TIMEZONE_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Time Zone"));

	MainWindow.ActiveStageTitles[LANGUAGE_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Locale"));
	MainWindow.InactiveStageTitles[LANGUAGE_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Locale"));

	MainWindow.ActiveStageTitles[USER_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Users"));
	MainWindow.InactiveStageTitles[USER_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Users"));

	MainWindow.ActiveStageTitles[INSTALLATION_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Installation"));
	MainWindow.InactiveStageTitles[INSTALLATION_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Installation"));

	MainWindow.ActiveStageTitles[FINISH_SCREEN] =
		g_strdup_printf(ActiveStageTitleMarkup, _("Finish"));
	MainWindow.InactiveStageTitles[FINISH_SCREEN] =
		g_strdup_printf(InactiveStageTitleMarkup, _("Finish"));
}

static void
mainwindow_xml_init()
{
	MainWindow.mainwindow = NULL;
	MainWindow.backbutton = NULL;
	MainWindow.quitbutton = NULL;
	MainWindow.helpbutton = NULL;
	MainWindow.nextbutton = NULL;
	MainWindow.installbutton = NULL;
	MainWindow.upgradebutton = NULL;
	MainWindow.rebootbutton = NULL;
	MainWindow.helpdialog = NULL;
	MainWindow.helpclosebutton = NULL;
	MainWindow.helptextview = NULL;
	MainWindow.screentitlelabel = NULL;
	MainWindow.screentitlesublabel1 = NULL;
	MainWindow.screentitlesublabel2 = NULL;
	MainWindow.welcomelabel = NULL;
	MainWindow.disklabel = NULL;
	MainWindow.timezonelabel = NULL;
	MainWindow.languagelabel = NULL;
	MainWindow.userlabel = NULL;
	MainWindow.installationlabel = NULL;
	MainWindow.finishlabel = NULL;

	MainWindow.screencontentvbox = NULL;
	MainWindow.timezonetoplevel = NULL;
	MainWindow.languagewindowtable = NULL;

	MainWindow.mainwindowxml =
		glade_xml_new(GLADEDIR "/" FILENAME, ROOTNODE, NULL);

	if (!MainWindow.mainwindowxml) {
		g_warning("something went wrong creating the GUI");
		exit(-1);
	}

	/* FIXME-use data passing instead of globals */
	MainWindow.welcomewindowxml =
		glade_xml_new(GLADEDIR "/" FILENAME, WELCOMENODE, NULL);
	installationdisk_xml_init();
	upgrade_xml_init();

	/* FIXME - use data passing instead of globals */
	datetimezone_xml_init();
	MainWindow.languagewindowxml =
		glade_xml_new(GLADEDIR "/" FILENAME, LANGUAGENODE, NULL);
	MainWindow.userswindowxml =
		glade_xml_new(GLADEDIR "/" USERSFILENAME, USERSNODE, NULL);
	MainWindow.confirmationwindowxml =
		glade_xml_new(GLADEDIR "/" CONFIRMATIONFILENAME,
				CONFIRMATIONNODE, NULL);
	MainWindow.installationwindowxml =
		glade_xml_new(GLADEDIR "/" INSTALLATIONFILENAME,
				INSTALLATIONNODE, NULL);
	MainWindow.failurewindowxml =
		glade_xml_new(GLADEDIR "/" FAILUREFILENAME, FAILURENODE, NULL);
	finish_xml_init();
	MainWindow.helpxml =
		glade_xml_new(GLADEDIR "/" FILENAME, HELPNODE, NULL);

	MainWindow.mainwindow =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"mainwindow");
	MainWindow.quitbutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"quitbutton");
	MainWindow.backbutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"backbutton");
	MainWindow.nextbutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"nextbutton");
	MainWindow.helpbutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"helpbutton");
	MainWindow.installbutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"installbutton");
	MainWindow.upgradebutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"upgradebutton");
	MainWindow.screentitlelabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"screentitlelabel");
	MainWindow.screentitlesublabel1 =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"screentitlesublabel1");
	MainWindow.screentitlesublabel2 =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"screentitlesublabel2");
	MainWindow.welcomelabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"welcomelabel");
	MainWindow.disklabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"disklabel");
	MainWindow.timezonelabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"timezonelabel");
	MainWindow.languagelabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"languagelabel");
	MainWindow.userlabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"userlabel");
	MainWindow.installationlabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"installationlabel");
	MainWindow.finishlabel =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"finishlabel");
	MainWindow.rebootbutton =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"rebootbutton");
	MainWindow.screencontentvbox =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"screencontentvbox");

	MainWindow.helpdialog =
		glade_xml_get_widget(MainWindow.helpxml, "helpdialog");
	MainWindow.helpclosebutton =
		glade_xml_get_widget(MainWindow.helpxml, "helpclosebutton");
	MainWindow.helptextview =
		glade_xml_get_widget(MainWindow.helpxml, "helptextview");

	g_signal_connect(
		G_OBJECT(MainWindow.helpdialog),
		"delete-event",
		G_CALLBACK(help_dialog_delete_event),
		MainWindow.helpdialog);
	g_signal_connect(
		G_OBJECT(MainWindow.helpclosebutton),
		"clicked",
		G_CALLBACK(help_dialog_hide),
		MainWindow.helpdialog);

	mainwindow_xml_screentitles_init();
	mainwindow_xml_stagetitles_init();
}

static void
mainwindow_ui_init()
{
	GtkWidget *mainwindow;
	GtkWidget *imagepadbox;
	GtkWidget *solarisimage;
	GtkWidget *screencontenteventbox;
	GtkWidget *screencontentviewport;
	GtkRequisition requisition;
	GtkSizeGroup *sizegroup;
	static GdkColor backcolour;

	glade_xml_signal_autoconnect(MainWindow.mainwindowxml);

	/* Tweak the pieces of the UI that we can't easily do in Glade XML */
	mainwindow =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"mainwindow");

	/*
	 * Make quit, help, back, next, install, upgrade and reboot
	 * buttons all the same size
	 */
	sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	gtk_size_group_add_widget(sizegroup, MainWindow.quitbutton);
	gtk_size_group_add_widget(sizegroup, MainWindow.helpbutton);
	gtk_size_group_add_widget(sizegroup, MainWindow.backbutton);
	gtk_size_group_add_widget(sizegroup, MainWindow.nextbutton);
	gtk_size_group_add_widget(sizegroup, MainWindow.installbutton);
	gtk_size_group_add_widget(sizegroup, MainWindow.upgradebutton);
	gtk_size_group_add_widget(sizegroup, MainWindow.rebootbutton);

	g_signal_connect(mainwindow, "delete-event",
	    G_CALLBACK(on_quitbutton_clicked),
	    NULL);

	window_graphics_set_size_properties(mainwindow);

	gtk_widget_set_sensitive(MainWindow.backbutton, FALSE);

	/* Set background for screen content event box to WHITE */
	screencontenteventbox =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"screencontenteventbox");
	screencontentviewport =
		glade_xml_get_widget(
			MainWindow.mainwindowxml,
			"screencontentviewport");
	gdk_color_parse(WHITE_COLOR, &backcolour);
	gtk_widget_modify_bg(screencontenteventbox, GTK_STATE_NORMAL, &backcolour);
	gtk_widget_modify_bg(screencontentviewport, GTK_STATE_NORMAL, &backcolour);

	gtk_window_set_default_icon_name("system-software-install");
}

static void
text_files_init(void)
{
	gchar *locale_id = NULL;

	/* Get the current Locale */
	locale_id = setlocale(LC_MESSAGES, NULL);

	/* initialize the structure element TextFileLocations */
	MainWindow.TextFileLocations = g_new0(gchar*, NUMTEXTFILES);

	/* License Agreement */
	MainWindow.TextFileLocations[LICENSE_AGREEMENT] =
		help_generate_file_path(
			LICENSE_AGREEMENT_PATH,
			locale_id,
			LICENSE_AGREEMENT_FILENAME);

	/* Help Files */
	MainWindow.TextFileLocations[HELP_INSTALL_DISK] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_DISK_FILENAME);

	MainWindow.TextFileLocations[HELP_INSTALL_LANGUAGE] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_LANGUAGE_FILENAME);

	MainWindow.TextFileLocations[HELP_INSTALL_TIMEZONE] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_TIMEZONE_FILENAME);

	MainWindow.TextFileLocations[HELP_INSTALL_USERS] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_USERS_FILENAME);

	MainWindow.TextFileLocations[HELP_INSTALL_PROGRESS] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_PROGRESS_FILENAME);

	MainWindow.TextFileLocations[HELP_UPGRADE_PROGRESS] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_UPGRADE_PROGRESS_FILENAME);

	MainWindow.TextFileLocations[HELP_UPGRADE_FAILURE] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_UPGRADE_FAILURE_FILENAME);

	MainWindow.TextFileLocations[HELP_INSTALL_CONFIRMATION] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_CONFIRMATION_FILENAME);

	MainWindow.TextFileLocations[HELP_UPGRADE_CONFIRMATION] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_UPGRADE_CONFIRMATION_FILENAME);

	MainWindow.TextFileLocations[HELP_FINISH] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_FINISH_FILENAME);

	MainWindow.TextFileLocations[HELP_WELCOME] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_WELCOME_FILENAME);

	MainWindow.TextFileLocations[HELP_UPGRADE_DISK] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_UPGRADE_DISK_FILENAME);

	MainWindow.TextFileLocations[HELP_INSTALL_FAILURE] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_INSTALL_FAILURE_FILENAME);

	MainWindow.TextFileLocations[HELP_UPGRADE_FAILURE] =
		help_generate_file_path(
			HELP_PATH,
			locale_id,
			HELP_UPGRADE_FAILURE_FILENAME);
	/*
	 * Install log doesn't exist yet most likely, and isn't localised
	 * so dispense with the formalities.
	 */
	MainWindow.TextFileLocations[INSTALL_LOG] = INSTALL_LOG_FULLPATH;
	MainWindow.TextFileLocations[UPGRADE_LOG] = UPGRADE_LOG_FULLPATH;
}

static void
initialize_milestone_completion()
{
	MainWindow.OverallPercentage = 0;
	MainWindow.MileStonePercentage = g_new0(guint, NUMMILESTONES);
	MainWindow.MileStoneComplete = g_new0(gboolean, NUMMILESTONES);

	MainWindow.CurrentMileStone = -1;
	/* g_new0 initialises ints to 0 so no need to initialise percentages */

	/* Target Discovery Milestones */
	MainWindow.MileStoneComplete[OM_DISK_DISCOVERY] = FALSE;
	MainWindow.MileStoneComplete[OM_PARTITION_DISCOVERY] = FALSE;
	MainWindow.MileStoneComplete[OM_SLICE_DISCOVERY] = FALSE;
	MainWindow.MileStoneComplete[OM_UPGRADE_TARGET_DISCOVERY] = FALSE;

	/* System Validation Milestone */
	MainWindow.MileStoneComplete[OM_UPGRADE_CHECK] = FALSE;

	/* Install/Upgrade Type Milestones */
	MainWindow.MileStoneComplete[OM_TARGET_INSTANTIATION] = FALSE;
	MainWindow.MileStoneComplete[OM_SOFTWARE_UPDATE] = FALSE;
	MainWindow.MileStoneComplete[OM_POSTINSTAL_TASKS] = FALSE;
}

int
main(int argc, char *argv[])
{
	gchar **remaining_args = NULL;
	GOptionEntry option_entries[] = {
		/* ... your application's command line options go here ... */
		{ "wait-for-sigusr1", 'w', 0,
			G_OPTION_ARG_NONE, (gpointer)&waitforsignal,
			"Wait to receive the SIGUSR1 signal before showing the GUI.",
			NULL},
		/*
		 * last but not least a special option that collects
		 * filenames
		 */
		{ G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY,
			&remaining_args,
			"Special option that collects any remaining arguments for us" },
		{ NULL }
	};
	GOptionContext *option_context;
	GError *error;

	option_context = g_option_context_new("installer-app");
#ifdef ENABLE_NLS
	bindtextdomain(GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset(GETTEXT_PACKAGE, "UTF-8");
	textdomain(GETTEXT_PACKAGE);
	setlocale(LC_ALL,"");
#endif

	gui_error_logging_init("gui-install");
	gtk_init(&argc, &argv);
	g_option_context_add_main_entries(
		option_context,
		option_entries,
		GETTEXT_PACKAGE);
	g_option_context_parse(option_context, &argc, &argv, &error);

	if (getuid() != 0) {
		g_warning("The OpenIndiana installer must be run as root. Quitting.");
		exit(-1);
	}

	if (om_process_running() != OM_PROC_NOT_RUNNING) {
		gui_install_prompt_dialog(FALSE, FALSE, FALSE,
			GTK_MESSAGE_ERROR,
			_("Installer Startup Terminated"),
			_("Only one instance of this Installer is allowed. "
			"Another instance is already running."));
		g_warning("Only one instance of this Installer is allowed. "
			"Another instance is already running.");
		exit(-1);
	}

	/*
	 * parse remaining command-line arguments that are not
	 * options (e.g. filenames or URIs or whatever), if any
	 */
	if (remaining_args != NULL) {
	    gint i, num_args;

		num_args = g_strv_length(remaining_args);
		for (i = 0; i < num_args; ++i) {
			/* process remaining_args[i] here */
		}
		g_strfreev(remaining_args);
		remaining_args = NULL;
	}
	glade_init();

	/*
	 * Kick off target discovery ASAP
	 */
	initialize_milestone_completion();

	omhandle = om_initiate_target_discovery(target_discovery_callback);

	if (omhandle == OM_FAILURE) {
		/* QUIT FATAL ERROR Target Discovery could not be started */
		g_critical(_("Target Discovery failed to start\n"));
		exit(-1);
	}

	signal(SIGUSR1, catch_sigusr);

	/* Wait until the keyboard layout app signals us */
	while (waitforsignal == TRUE)
		pause();

	mainwindow_xml_init();
	mainwindow_ui_init();

	/* The initial screen shown will always be the welcome screen */
	welcome_screen_init();
	installationdisk_ui_init();
	upgrade_detection_screen_init();
	users_window_init();
	datetimezone_ui_init();
	confirmation_window_init();
	installation_window_init();
	failure_window_init();
	finish_ui_init();
	text_files_init();

	gtk_widget_show(glade_xml_get_widget(MainWindow.mainwindowxml,
			"mainwindow"));

	gtk_main();

	/* cleanup */
	language_cleanup();
	upgrade_info_cleanup();
	return (0);
}
