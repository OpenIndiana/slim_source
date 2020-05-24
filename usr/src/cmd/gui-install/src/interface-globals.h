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

#ifndef __INTERFACE_GLOBALS_H
#define	__INTERFACE_GLOBALS_H


#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glade/glade.h>
#include <welcome-screen.h>
#include <users-screen.h>
#include <datetimezone-screen.h>
#include <confirmation-screen.h>
#include <installation-disk-screen.h>
#include <installation-screen.h>
#include <failure-screen.h>
#include <finish-screen.h>

/* Glade XML file name definitions */
#define	FILENAME "gui-install.glade"
#define	DATETIMEZONEFILENAME "date-time-zone.glade"
#define	USERSFILENAME "users.glade"
#define	CONFIRMATIONFILENAME "confirmation.glade"
#define	INSTALLATIONFILENAME "installation.glade"
#define	INSTALLATIONDISKFILENAME "installationdisk.glade"
#define	FAILUREFILENAME "failure.glade"

/* Text filenames */
#define	LICENSE_AGREEMENT_FILENAME		"license"
#define	HELP_INSTALL_DISK_FILENAME		"INSTALL_DISK_PANEL.txt"
#define	HELP_INSTALL_LANGUAGE_FILENAME	"INSTALL_LANGUAGE_PANEL.txt"
#define	HELP_INSTALL_TIMEZONE_FILENAME	"INSTALL_TIMEZONE_PANEL.txt"
#define	HELP_INSTALL_USERS_FILENAME		"INSTALL_USERS_PANEL.txt"
#define	HELP_INSTALL_PROGRESS_FILENAME	"INSTALL_PROGRESS_PANEL.txt"
#define	HELP_INSTALL_CONFIRMATION_FILENAME		"INSTALL_REVIEW_PANEL.txt"
#define	HELP_INSTALL_FAILURE_FILENAME	"INSTALL_FAILURE_PANEL.txt"
#define	HELP_UPGRADE_PROGRESS_FILENAME	"UPGRADE_PROGRESS_PANEL.txt"
#define	HELP_UPGRADE_DISK_FILENAME		"UPGRADE_DISK_PANEL.txt"
#define	HELP_UPGRADE_FAILURE_FILENAME	"UPGRADE_FAILURE_PANEL.txt"
#define	HELP_UPGRADE_CONFIRMATION_FILENAME		"UPGRADE_REVIEW_PANEL.txt"
#define	HELP_FINISH_FILENAME				"FINISH_PANEL.txt"
#define	HELP_WELCOME_FILENAME			"WELCOME_PANEL.txt"

/* Text paths */
#define	LICENSE_AGREEMENT_PATH		"/usr/lib/install/data/os/5.11/licenses"
#define	HELP_PATH					TEXTDIR "/help"
#define	INSTALL_PROGRESS_PATH		TEXTDIR "/installmessages"
/* Install log path is a constant */
#define	INSTALL_LOG_FULLPATH		"/tmp/install_log"
#define	UPGRADE_LOG_FULLPATH		"/a/var/sadm/install_data/upgrade_log"

/* Glade XML root node definitions */
#define	ROOTNODE "mainwindow"
#define	WELCOMENODE "welcomescreenvbox"
#define	DISKNODE "diskselectiontoplevel"
#define	DATETIMEZONENODE "datetimezonetoplevel"
#define	TIMEZONENODE "timezonetoplevel"
#define	LANGUAGENODE "languagewindowtable"
#define	USERSNODE "userstoplevel"
#define	CONFIRMATIONNODE "confirmationtoplevel"
#define	INSTALLATIONNODE "installationwindowtable"
#define	FINISHNODE "finishbox"
#define	HELPNODE "helpdialog"
#define	FAILURENODE "failurewindowtable"

#define	NUMMILESTONES OM_POSTINSTAL_TASKS+1

/* Pango markup for the screen title and stage labels */
extern gchar *ScreenTitleMarkup;
extern gchar *ScreenSubTitleMarkup;
extern gchar *ActiveStageTitleMarkup;
extern gchar *InactiveStageTitleMarkup;

typedef struct _MainWindowXML {
	GladeXML *mainwindowxml;
	GladeXML *welcomewindowxml;
	GladeXML *installationdiskwindowxml;
	GladeXML *datetimezonewindowxml;
	GladeXML *timezonewindowxml;
	GladeXML *languagewindowxml;
	GladeXML *userswindowxml;
	GladeXML *confirmationwindowxml;
	GladeXML *installationwindowxml;
	GladeXML *failurewindowxml;
	GladeXML *finishxml;
	GladeXML *helpxml;

	GtkWidget *mainwindow;
	GtkWidget *helpbutton;
	GtkWidget *quitbutton;
	GtkWidget *backbutton;
	GtkWidget *nextbutton;
	GtkWidget *installbutton;

	GtkWidget *screentitlelabel;
	GtkWidget *screentitlesublabel1;
	GtkWidget *screentitlesublabel2;

	GtkWidget *welcomelabel;
	GtkWidget *disklabel;
	GtkWidget *timezonelabel;
	GtkWidget *languagelabel;
	GtkWidget *userlabel;
	GtkWidget *installationlabel;
	GtkWidget *finishlabel;

	GtkWidget *screencontentvbox;

	GtkWidget *timezonetoplevel;
	InstallationDiskWindowXML InstallationDiskWindow;
	DateTimeZoneWindowXML DateTimeZoneWindow;
	UsersWindowXML UsersWindow;
	WelcomeWindowXML WelcomeWindow;
	GtkWidget *languagewindowtable;
	ConfirmationWindowXML ConfirmationWindow;
	InstallationWindowXML InstallationWindow;
	FailureWindowXML FailureWindow;
	FinishWindowXML FinishWindow;

	gchar **ScreenTitles;
	gchar **ScreenSubTitles;
	gchar **ActiveStageTitles;
	gchar **InactiveStageTitles;
	gchar **TextFileLocations;

	gint CurrentMileStone;
	guint OverallPercentage;
	guint *MileStonePercentage;
	gboolean *MileStoneComplete;

	/* upgrade screen */
	GtkWidget *upgradebutton;
	/* Finish screen */
	GtkWidget *rebootbutton;

	/* Help dialog */
	GtkWidget *helpdialog;
	GtkWidget *helpclosebutton;
	GtkWidget *helptextview;
} MainWindowXML;

extern MainWindowXML MainWindow;

typedef enum {
	WELCOME_SCREEN = 0,
	DISK_SCREEN,
	TIMEZONE_SCREEN,
	LANGUAGE_SCREEN,
	USER_SCREEN,
	CONFIRMATION_SCREEN,
	INSTALLATION_SCREEN,
	FAILURE_SCREEN,
	FINISH_SCREEN,
	NUMSCREENS /* Not an actual screen */
} InstallScreen;


typedef enum {
	LICENSE_AGREEMENT = 0,
	HELP_INSTALL_DISK,
	HELP_INSTALL_LANGUAGE,
	HELP_INSTALL_TIMEZONE,
	HELP_INSTALL_USERS,
	HELP_INSTALL_PROGRESS,
	HELP_UPGRADE_PROGRESS,
	HELP_INSTALL_CONFIRMATION,
	HELP_UPGRADE_CONFIRMATION,
	HELP_FINISH,
	HELP_WELCOME,
	HELP_UPGRADE_DISK,
	HELP_INSTALL_FAILURE,
	HELP_UPGRADE_FAILURE,
	INSTALL_LOG,
	UPGRADE_LOG,
	NUMTEXTFILES /* Not an actual file, put new files before this entry */
} TextFiles;

#ifdef __cplusplus
}
#endif

#endif /* __INTERFACE_GLOBALS_H */
