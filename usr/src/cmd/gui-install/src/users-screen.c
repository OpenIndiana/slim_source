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
#include <ctype.h>
#include <pwd.h>
#include <netdb.h>
#include "installation-profile.h"
#include "interface-globals.h"
#include "users-screen.h"
#include "callbacks.h"

gchar *PasswordErrorMarkup = N_("<span size=\"smaller\"><span font_desc=\"Bold\">Error: </span>%s</span>");

static gboolean
is_str_empty(gchar *str)
{
	return ((!str || strlen(str) < 1) ? TRUE : FALSE);
}

static gboolean
is_password_valid(gchar *pwd, gchar **errormsg)
{
	gboolean ret_val = TRUE;
	gboolean contains_alpha = FALSE;
	gboolean contains_num = FALSE;
	gboolean contains_special = FALSE;
	int i;

	/* For the moment priviliged users can set the password to whatever */
	/* they want, so this function always returns TRUE */
	/* Code is being left in place in case this scenario ever changes. */
	return (TRUE);
#ifdef CHECK_PASSWORDS
	/* Ensure password matches that defined in passwd(1) */
	if (!is_str_empty(pwd)) {
		/* Put specific character/length checking in here */
		if (strlen(pwd) < 6) {
			ret_val = FALSE;
			*errormsg = g_strdup(_("Password must contain at least 6 characters."));
		} else {
			/* Ensure there is at least one numeric/special char and 1 alpha */
			for (i = 0; i < strlen(pwd); i++) {
				if (isalpha(pwd[i])) {
					contains_alpha = TRUE;
				} else if (isdigit(pwd[i])) {
					contains_num = TRUE;
				} else {
					contains_special = TRUE;
				}
				if (contains_alpha && (contains_num || contains_special)) {
					break;
				}
			}

			if (!contains_alpha) {
				*errormsg = g_strdup(_("Password must contain 1 alphabetical character."));
				ret_val = FALSE;
			} else if (!(contains_num || contains_special)) {
				*errormsg = g_strdup(_("Password must contain 1 digit/special character."));
				ret_val = FALSE;
			}
		}
	}

	return (ret_val);
#endif
}

static gboolean
is_password_equal(gchar *pwd1, gchar *pwd2)
{
	if (!pwd1) {
		if (pwd2) {
			return (FALSE);
		}
	} else if (!pwd2) {
		return (FALSE);
	} else if (strcmp(pwd1, pwd2) != 0) {
		return (FALSE);
	}

	return (TRUE);
}

void
users_window_init(void)
{
	if (!MainWindow.userswindowxml) {
		g_warning("Failed to access Users Window.");
		exit(-1);
	}

	glade_xml_signal_autoconnect(MainWindow.userswindowxml);

	MainWindow.UsersWindow.userstoplevel = NULL;
	MainWindow.UsersWindow.rootpassword1entry = NULL;
	MainWindow.UsersWindow.rootpassword2entry = NULL;
	MainWindow.UsersWindow.rootpasswordinfo= NULL;
	MainWindow.UsersWindow.rootpasswordinfoimage = NULL;
	MainWindow.UsersWindow.rootpasswordinfolabel = NULL;
	MainWindow.UsersWindow.usernameentry = NULL;
	MainWindow.UsersWindow.loginnameentry = NULL;
	MainWindow.UsersWindow.loginnameinfo= NULL;
	MainWindow.UsersWindow.loginnameinfoimage = NULL;
	MainWindow.UsersWindow.loginnameinfolabel = NULL;
	MainWindow.UsersWindow.userpassword1entry = NULL;
	MainWindow.UsersWindow.userpassword2entry = NULL;
	MainWindow.UsersWindow.userpasswordinfo= NULL;
	MainWindow.UsersWindow.userpasswordinfoimage = NULL;
	MainWindow.UsersWindow.userpasswordinfolabel = NULL;
	MainWindow.UsersWindow.hostnameentry = NULL;
	MainWindow.UsersWindow.hostnameinfo = NULL;
	MainWindow.UsersWindow.hostnameinfoimage = NULL;
	MainWindow.UsersWindow.hostnameinfolabel = NULL;

	MainWindow.UsersWindow.error_posted = FALSE;

	InstallationProfile.rootpassword = NULL;
	InstallationProfile.username = NULL;
	InstallationProfile.loginname = NULL;
	InstallationProfile.userpassword = NULL;
	InstallationProfile.hostname = NULL;
}

gboolean
users_password_key_press(GtkWidget *entry,
			GdkEventKey *event,
			gpointer user_data)
{
	GdkModifierType state;

	gdk_event_get_state((GdkEvent *)event, &state);

	/*
	 * Check if a user is pasting into the password field either via
	 * CTRL-V or the Insert key. Returning TRUE indicates the event
	 * has been handled and there fore the keystroke is not processed
	 * any further. e.g. characters are not pasted.
	 */
	if ((event->keyval == GDK_v && (state & GDK_CONTROL_MASK)) ||
		event->keyval == GDK_Insert) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

gboolean
users_password_button_press(GtkWidget *entry,
			GdkEventButton *event,
			gpointer user_data)
{
	if (event->button == 2) {
		return (TRUE);
	} else {
		return (FALSE);
	}
}

void
users_load_widgets(void)
{
	GtkSizeGroup *sizegroup = NULL;
	GtkWidget *rootpassword1label;
	GtkWidget *rootpassword2label;
	GtkWidget *userpassword1label;
	GtkWidget *userpassword2label;
	GtkWidget *usernamelabel;
	GtkWidget *loginnamelabel;
	GtkWidget *hostnamelabel;

	MainWindow.UsersWindow.userstoplevel = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"userstoplevel");
	MainWindow.UsersWindow.rootpassword1entry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"rootpassword1entry");
	MainWindow.UsersWindow.rootpassword2entry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"rootpassword2entry");
	MainWindow.UsersWindow.rootpasswordinfo= glade_xml_get_widget(
													MainWindow.userswindowxml,
													"rootpasswordinfohbox");
	MainWindow.UsersWindow.rootpasswordinfoimage = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"rootpasswordinfoimage");
	MainWindow.UsersWindow.rootpasswordinfolabel = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"rootpasswordinfolabel");
	MainWindow.UsersWindow.usernameentry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"usernameentry");
	MainWindow.UsersWindow.loginnameentry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"loginnameentry");
	MainWindow.UsersWindow.loginnameinfo= glade_xml_get_widget(
													MainWindow.userswindowxml,
													"loginnameinfohbox");
	MainWindow.UsersWindow.loginnameinfoimage = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"loginnameinfoimage");
	MainWindow.UsersWindow.loginnameinfolabel = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"loginnameinfolabel");
	MainWindow.UsersWindow.userpassword1entry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"userpassword1entry");
	MainWindow.UsersWindow.userpassword2entry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"userpassword2entry");
	MainWindow.UsersWindow.userpasswordinfo = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"userpasswordinfohbox");
	MainWindow.UsersWindow.userpasswordinfoimage = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"userpasswordinfoimage");
	MainWindow.UsersWindow.userpasswordinfolabel = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"userpasswordinfolabel");
	MainWindow.UsersWindow.hostnameentry = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"hostnameentry");
	MainWindow.UsersWindow.hostnameinfo = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"hostnameinfohbox");
	MainWindow.UsersWindow.hostnameinfoimage = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"hostnameinfoimage");
	MainWindow.UsersWindow.hostnameinfolabel = glade_xml_get_widget(
													MainWindow.userswindowxml,
													"hostnameinfolabel");


	rootpassword1label = glade_xml_get_widget(MainWindow.userswindowxml,
									"rootpassword1label");
	rootpassword2label = glade_xml_get_widget(MainWindow.userswindowxml,
									"rootpassword2label");
	userpassword1label = glade_xml_get_widget(MainWindow.userswindowxml,
									"userpassword1label");
	userpassword2label = glade_xml_get_widget(MainWindow.userswindowxml,
									"userpassword2label");
	usernamelabel = glade_xml_get_widget(MainWindow.userswindowxml,
									"usernamelabel");
	loginnamelabel = glade_xml_get_widget(MainWindow.userswindowxml,
									"loginnamelabel");
	hostnamelabel = glade_xml_get_widget(MainWindow.userswindowxml,
									"hostnamelabel");

	sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	gtk_size_group_add_widget(sizegroup, rootpassword1label);
	gtk_size_group_add_widget(sizegroup, rootpassword2label);
	gtk_size_group_add_widget(sizegroup, usernamelabel);
	gtk_size_group_add_widget(sizegroup, loginnamelabel);
	gtk_size_group_add_widget(sizegroup, userpassword1label);
	gtk_size_group_add_widget(sizegroup, userpassword2label);
	gtk_size_group_add_widget(sizegroup, hostnamelabel);

	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.userpassword1entry),
			"key-press-event",
			G_CALLBACK(users_password_key_press),
			NULL);
	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.userpassword2entry),
			"key-press-event",
			G_CALLBACK(users_password_key_press),
			NULL);
	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.rootpassword1entry),
			"key-press-event",
			G_CALLBACK(users_password_key_press),
			NULL);
	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.rootpassword2entry),
			"key-press-event",
			G_CALLBACK(users_password_key_press),
			NULL);

	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.userpassword1entry),
			"button-press-event",
			G_CALLBACK(users_password_button_press),
			NULL);
	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.userpassword2entry),
			"button-press-event",
			G_CALLBACK(users_password_button_press),
			NULL);
	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.rootpassword1entry),
			"button-press-event",
			G_CALLBACK(users_password_button_press),
			NULL);
	g_signal_connect(G_OBJECT(MainWindow.UsersWindow.rootpassword2entry),
			"button-press-event",
			G_CALLBACK(users_password_button_press),
			NULL);

	/* Unset select text on focus, we will do this manually */
	g_object_set(
	    gtk_widget_get_settings(MainWindow.UsersWindow.usernameentry),
	    "gtk-entry-select-on-focus", FALSE, NULL);
	g_object_set(
	    gtk_widget_get_settings(MainWindow.UsersWindow.loginnameentry),
	    "gtk-entry-select-on-focus", FALSE, NULL);
	g_object_set(
	    gtk_widget_get_settings(MainWindow.UsersWindow.userpassword1entry),
	    "gtk-entry-select-on-focus", FALSE, NULL);
	g_object_set(
	    gtk_widget_get_settings(MainWindow.UsersWindow.userpassword2entry),
	    "gtk-entry-select-on-focus", FALSE, NULL);
	g_object_set(
	    gtk_widget_get_settings(MainWindow.UsersWindow.hostnameentry),
	    "gtk-entry-select-on-focus", FALSE, NULL);
}

gboolean
users_validate_login_name(gboolean check_changed)
{
	struct passwd   password;	/* user password information */
	struct passwd  *p  = NULL;
	static gint 	pwbuflen = 0;
	gchar *pw_buffer;	/* Storage for the password */
	const gchar *loginname = NULL;
	gchar *errormsg = NULL;
	gboolean loginname_changed = FALSE;
	gboolean ret_val = TRUE;
	int i;

	if (check_changed) {
		loginname_changed = GPOINTER_TO_INT(
						g_object_get_data(
							G_OBJECT(MainWindow.UsersWindow.loginnameentry),
							"changed"));
	}

	if (pwbuflen == 0) {
		/* Determine the buffer length required for passwd details */
		pwbuflen = sysconf(_SC_GETPW_R_SIZE_MAX);
		g_assert(pwbuflen >= 0);
	}

	if ((check_changed && loginname_changed) || (!check_changed)) {
		loginname = gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.loginnameentry));

		/* Only validate the login name if it's greater than 0 chars */
		if (strlen(loginname) > 0) {
			/*
			 * Reject anything that's already got a passwd file entry:
			 * root, daemon, nobody, bin, sys etc...
			 */
			pw_buffer = g_new0(gchar, pwbuflen);
			p = getpwnam_r(loginname, &password, pw_buffer, pwbuflen);

			if (p != NULL) {
				/*
				 * An entry matching loginname has been found
				 * in the passwd file. It is possible that the
				 * installer was stopped for some reason after
				 * the loginname was added. Only reject it if
				 * the UID is not the one used by the
				 * installer.
				 */
				if (password.pw_uid != om_get_user_uid()) {
					gchar *message = g_strdup_printf(
					    _("\"%s\" cannot be used"),
					    loginname);

					errormsg =
					    g_strdup_printf(_(PasswordErrorMarkup),
					    message);
					g_free(message);
					ret_val = FALSE;
				}
			}
			g_free(pw_buffer);

			/* Temporary Check that user login name is not alldigits */
			for (i = 0; i < strlen(loginname); i++) {
				if (!isdigit(loginname[i]))
					break;
			}
			if (i == strlen(loginname)) {
				errormsg = g_strdup(_("Log-in name cannot be all digits"));
				ret_val = FALSE;
			}

			if (strlen(loginname) > MAX_LOGIN_NAME_LEN) {
				errormsg = g_strdup(_("Log-in name cannot be greater than 8 characters"));
				ret_val = FALSE;
			}

			if (check_changed) {
				g_object_set_data(
						G_OBJECT(MainWindow.UsersWindow.loginnameentry),
						"changed",
						GINT_TO_POINTER(FALSE));
			}
		}
	}

	if (!ret_val) {
		gtk_entry_set_text(
				GTK_ENTRY(MainWindow.UsersWindow.loginnameentry),
				"");
		gtk_widget_grab_focus(
				MainWindow.UsersWindow.loginnameentry);
		gtk_widget_show(MainWindow.UsersWindow.loginnameinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.loginnameinfolabel),
					errormsg);
		MainWindow.UsersWindow.error_posted = TRUE;
		g_free(errormsg);
	}
	return (ret_val);
}

gboolean
users_validate_user_passwords(GtkWidget *widget, gboolean check_changed)
{
	gboolean pwd1_changed = FALSE;
	gboolean pwd2_changed = FALSE;
	gboolean ret_val = TRUE;
	gchar *pwd1, *pwd2;
	gchar *errormsg = NULL;

	if (check_changed) {
		pwd1_changed = GPOINTER_TO_INT(
						g_object_get_data(
							G_OBJECT(MainWindow.UsersWindow.userpassword1entry),
							"changed"));
		pwd2_changed = GPOINTER_TO_INT(
						g_object_get_data(
							G_OBJECT(MainWindow.UsersWindow.userpassword2entry),
							"changed"));
	}

	if ((check_changed && (pwd1_changed || pwd2_changed)) ||
		(!check_changed)) {

		/* Get Text */
		pwd1 = (gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword1entry));
		pwd2 = (gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword2entry));

		if (widget == MainWindow.UsersWindow.userpassword1entry) {
			/* Only check validty of password */
			if (!is_password_valid(pwd1, &errormsg)) {
				ret_val = FALSE;
			}
		} else {
			if (pwd1 || pwd2) {
				if (!is_password_valid(pwd1, &errormsg)) {
					ret_val = FALSE;
				} else if (!is_password_valid(pwd2, &errormsg)) {
					ret_val = FALSE;
				} else if (!is_password_equal(pwd1, pwd2)) {
					errormsg = g_strdup_printf(_(PasswordErrorMarkup),
												_("Passwords do not match."));
					ret_val = FALSE;
				}
			}
		}

		if (check_changed) {
			g_object_set_data(
					G_OBJECT(MainWindow.UsersWindow.userpassword1entry),
					"changed",
					GINT_TO_POINTER(FALSE));
			g_object_set_data(
					G_OBJECT(MainWindow.UsersWindow.userpassword2entry),
					"changed",
					GINT_TO_POINTER(FALSE));
		}
	}

	if (!ret_val) {
		gtk_entry_set_text(
				GTK_ENTRY(MainWindow.UsersWindow.userpassword1entry),
				"");
		gtk_entry_set_text(
				GTK_ENTRY(MainWindow.UsersWindow.userpassword2entry),
				"");
		gtk_widget_grab_focus(
				MainWindow.UsersWindow.userpassword1entry);
		gtk_widget_show(MainWindow.UsersWindow.userpasswordinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.userpasswordinfolabel),
					errormsg);
		g_free(errormsg);
		MainWindow.UsersWindow.error_posted = TRUE;
	}
	return (ret_val);
}

gboolean
users_validate_root_passwords(GtkWidget *widget, gboolean check_changed)
{
	gboolean pwd1_changed = FALSE;
	gboolean pwd2_changed = FALSE;
	gboolean ret_val = TRUE;
	gchar *pwd1, *pwd2;
	gchar *errormsg = NULL;

	if (check_changed) {
		pwd1_changed = GPOINTER_TO_INT(
						g_object_get_data(
							G_OBJECT(MainWindow.UsersWindow.rootpassword1entry),
							"changed"));
		pwd2_changed = GPOINTER_TO_INT(
						g_object_get_data(
							G_OBJECT(MainWindow.UsersWindow.rootpassword2entry),
							"changed"));
	}

	if ((check_changed && (pwd1_changed || pwd2_changed)) ||
		(!check_changed)) {

		/* Get Text */
		pwd1 = (gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.rootpassword1entry));
		pwd2 = (gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.rootpassword2entry));

		if (widget == MainWindow.UsersWindow.rootpassword1entry) {
			/* Only check validty of password */
			if (!is_password_valid(pwd1, &errormsg)) {
				ret_val = FALSE;
			}
		} else {
			if (pwd1 || pwd2) {
				if (!is_password_valid(pwd1, &errormsg)) {
					ret_val = FALSE;
				} else if (!is_password_valid(pwd2, &errormsg)) {
					ret_val = FALSE;
				} else if (!is_password_equal(pwd1, pwd2)) {
					errormsg = g_strdup_printf(_(PasswordErrorMarkup),
												_("Passwords do not match."));
					ret_val = FALSE;
				}
			}
		}

		/* Unset "changed" */
		if (check_changed) {
			g_object_set_data(
				G_OBJECT(MainWindow.UsersWindow.rootpassword1entry),
				"changed",
				GINT_TO_POINTER(FALSE));
			g_object_set_data(
				G_OBJECT(MainWindow.UsersWindow.rootpassword2entry),
				"changed",
				GINT_TO_POINTER(FALSE));
		}
	}

	if (!ret_val) {
		/* Display Warning, reset passwords and possibly set focus */
		gtk_entry_set_text(
				GTK_ENTRY(MainWindow.UsersWindow.rootpassword1entry),
				"");
		gtk_entry_set_text(
				GTK_ENTRY(MainWindow.UsersWindow.rootpassword2entry),
				"");
		gtk_widget_grab_focus(
				MainWindow.UsersWindow.rootpassword1entry);
		gtk_widget_show(MainWindow.UsersWindow.rootpasswordinfoimage);
		gtk_label_set_label(
				GTK_LABEL(MainWindow.UsersWindow.rootpasswordinfolabel),
				errormsg);
		g_free(errormsg);
		MainWindow.UsersWindow.error_posted = TRUE;
	}
	return (ret_val);
}

gboolean
user_account_entered(void)
{
	/* User account only needs login name so just Check the login name field */
	return (!is_str_empty((gchar *) gtk_entry_get_text(
					GTK_ENTRY(MainWindow.UsersWindow.loginnameentry))));
}

gboolean
root_password_entered(void)
{
	/* If one password entered then both must have been entered */
	return (!is_str_empty((gchar *) gtk_entry_get_text(
					GTK_ENTRY(MainWindow.UsersWindow.rootpassword1entry))));
}

gboolean
users_validate(void)
{
	gboolean username_empty = FALSE;
	gboolean user_pwd1_empty = FALSE;
	gboolean user_pwd2_empty = FALSE;
	gboolean host_name_empty = FALSE;
	gboolean ret_val = TRUE;

	if (MainWindow.UsersWindow.error_posted) {
		return (FALSE);
	}

	if ((ret_val = users_validate_root_passwords(
			MainWindow.UsersWindow.rootpassword2entry,
			FALSE)) == FALSE) {
		gui_install_prompt_dialog(
			FALSE,
			FALSE,
			FALSE,
			GTK_MESSAGE_ERROR,
			_("Root Password Invalid"),
			_("The two root passwords do not match\nRe-enter the root password."));
		return (ret_val);
	}

	if ((ret_val = users_validate_user_passwords(
			MainWindow.UsersWindow.userpassword2entry,
			FALSE)) == FALSE) {
		gui_install_prompt_dialog(
			FALSE,
			FALSE,
			FALSE,
			GTK_MESSAGE_ERROR,
			_("User Password Invalid"),
			_("The two user passwords do not match\nRe-enter the user password."));
		return (ret_val);
	}

	if (user_account_entered()) {
		if ((ret_val = users_validate_login_name(FALSE)) == FALSE) {
			gui_install_prompt_dialog(
				FALSE,
				FALSE,
				FALSE,
				GTK_MESSAGE_ERROR,
				_("Invalid User Account"),
				_("Invalid Log-in name.\nEnter a different Log-in name."));
			return (ret_val);
		}
	}

	username_empty = is_str_empty((gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.usernameentry)));
	user_pwd1_empty = is_str_empty((gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword1entry)));
	user_pwd2_empty = is_str_empty((gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword2entry)));

	if ((!username_empty ||
		!user_pwd1_empty ||
		!user_pwd2_empty) &&
		!user_account_entered()) {
		ret_val = FALSE;

		if (!username_empty) {
			gtk_widget_grab_focus(
				MainWindow.UsersWindow.usernameentry);
			users_entry_select_text(
				MainWindow.UsersWindow.usernameentry);
		} else {
			gtk_widget_grab_focus(
				MainWindow.UsersWindow.loginnameentry);
			users_entry_select_text(
				MainWindow.UsersWindow.loginnameentry);
		}
		gui_install_prompt_dialog(
			FALSE,
			FALSE,
			FALSE,
			GTK_MESSAGE_ERROR,
			_("Invalid User Account"),
			_("The Log-in name cannot be blank.\nEnter a Log-in name or clear all user account fields."));
		return (ret_val);
	}

	host_name_empty = is_str_empty((gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.hostnameentry)));

	if (!host_name_empty) {
		if ((ret_val = users_validate_host_name(FALSE)) == FALSE) {
			gui_install_prompt_dialog(
				FALSE,
				FALSE,
				FALSE,
				GTK_MESSAGE_ERROR,
				_("Invalid Computer Name"),
				_("The computer name contains invalid characters.\nEnter a valid computer name."));
		return (ret_val);
		}
	}

	if (!root_password_entered()) {
		ret_val =
			gui_install_prompt_dialog(
				TRUE,
				TRUE,
				FALSE,
				GTK_MESSAGE_WARNING,
				_("No root password"),
				_("A root password has not been defined. The system is completely unsecured.\nClick Cancel to set a root password."));
		if (!ret_val) {
			users_clear_info_warning_labels();
			users_entry_unselect_text(
				MainWindow.UsersWindow.hostnameentry);
			gtk_widget_grab_focus(
				MainWindow.UsersWindow.rootpassword1entry);
			users_entry_select_text(
				MainWindow.UsersWindow.rootpassword1entry);
			return (ret_val);
		}
	}

	if (host_name_empty) {
		gtk_entry_set_text(
		    GTK_ENTRY(MainWindow.UsersWindow.hostnameentry),
		    "openindiana");
		ret_val = gui_install_prompt_dialog(
		    TRUE,
		    TRUE,
		    FALSE,
		    GTK_MESSAGE_WARNING,
		    _("Invalid Computer Name"),
		    _("The computer name cannot be blank. It has been reset to the default value.\nClick Cancel to set a different computer name."));
		if (!ret_val) {
			gtk_widget_grab_focus(
				MainWindow.UsersWindow.hostnameentry);
			users_entry_select_text(
				MainWindow.UsersWindow.hostnameentry);
			return (ret_val);
		}
	}

	if (ret_val) {
		/* user has chosen to continue to summary screen */
		if (!user_account_entered()) {
			/* Ensure other user account fields are blanked out */
			if (!user_account_entered) {
				if (!username_empty) {
					gtk_entry_set_text(
						GTK_ENTRY(MainWindow.UsersWindow.usernameentry),
						"");
				}

				if (!user_pwd1_empty) {
					gtk_entry_set_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword1entry),
						"");
				}

				if (!user_pwd2_empty) {
					gtk_entry_set_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword2entry),
						"");
				}
			}
		}
	}

	return (ret_val);
}

void
users_clear_info_warning_labels(void)
{
	/* Blank out info labels if there is a message there */
	if (GTK_WIDGET_VISIBLE(MainWindow.UsersWindow.rootpasswordinfoimage)) {
		gtk_widget_hide(MainWindow.UsersWindow.rootpasswordinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.rootpasswordinfolabel),
					_("Re-enter to check for typing errors."));
	}

	if (GTK_WIDGET_VISIBLE(MainWindow.UsersWindow.loginnameinfoimage)) {
		gtk_widget_hide(MainWindow.UsersWindow.loginnameinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.loginnameinfolabel),
					_("Required when creating a user account."));
	}

	if (GTK_WIDGET_VISIBLE(MainWindow.UsersWindow.userpasswordinfoimage)) {
		gtk_widget_hide(MainWindow.UsersWindow.userpasswordinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.userpasswordinfolabel),
					_("Re-enter to check for typing errors."));
	}

	if (GTK_WIDGET_VISIBLE(MainWindow.UsersWindow.hostnameinfoimage)) {
		gtk_widget_hide(MainWindow.UsersWindow.hostnameinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.hostnameinfolabel),
					"");
	}
	MainWindow.UsersWindow.error_posted = FALSE;
}

void
users_store_data(void)
{
	const gchar *tmpStr;

	/* Reset All InstallationProfile User data to NULL */
	if (InstallationProfile.rootpassword != NULL) {
		g_free(InstallationProfile.rootpassword);
		InstallationProfile.rootpassword = NULL;
	}

	if (InstallationProfile.username != NULL) {
		g_free(InstallationProfile.username);
		InstallationProfile.username = NULL;
	}

	if (InstallationProfile.loginname != NULL) {
		g_free(InstallationProfile.loginname);
		InstallationProfile.loginname = NULL;
	}

	if (InstallationProfile.userpassword != NULL) {
		g_free(InstallationProfile.userpassword);
		InstallationProfile.userpassword = NULL;
	}

	if (InstallationProfile.hostname != NULL) {
		g_free(InstallationProfile.hostname);
		InstallationProfile.hostname = NULL;
	}

	if (root_password_entered()) {
		InstallationProfile.rootpassword =
				g_strdup((gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.rootpassword1entry)));
	}

	if (user_account_entered()) {
		tmpStr = gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.usernameentry));
		if (!is_str_empty((gchar *)tmpStr)) {
			InstallationProfile.username = g_strdup(tmpStr);
		}

		tmpStr = gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.loginnameentry));
		if (!is_str_empty((gchar *)tmpStr)) {
			InstallationProfile.loginname = g_strdup(tmpStr);
		}

		tmpStr = gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.userpassword1entry));
		if (!is_str_empty((gchar *)tmpStr)) {
			InstallationProfile.userpassword = g_strdup(tmpStr);
		}
	}

	tmpStr = gtk_entry_get_text(
					GTK_ENTRY(MainWindow.UsersWindow.hostnameentry));
	if (!is_str_empty((gchar *)tmpStr)) {
		InstallationProfile.hostname = g_strdup(tmpStr);
	}
}

void
users_entry_unselect_text(GtkWidget *widget)
{
	gint s, e;

	if (gtk_editable_get_selection_bounds(GTK_EDITABLE(widget), &s, &e)) {
		gtk_editable_select_region(GTK_EDITABLE(widget), 0, 0);
	}
}

void
users_entry_select_text(GtkWidget *widget)
{
	gtk_editable_select_region(GTK_EDITABLE(widget), 0, -1);
}

static gboolean
invalid_hostname_character(gchar *hostname)
{
	gboolean ret_val = FALSE;
	gint i = 0;

	for (i = 0; i < strlen(hostname); i++) {
		if ((!isalnum(hostname[i]) && hostname[i] != '-' &&
			hostname[i] != '_' && hostname[i] != '.') ||
			(isblank(hostname[i]))) {
			ret_val = TRUE;
			break;
		}
	}
	return (ret_val);
}

gboolean
users_validate_host_name(gboolean check_changed)
{
	gchar *hostname = NULL;
	gchar *errormsg = NULL;
	gboolean hostname_changed = FALSE;
	gboolean ret_val = TRUE;
	gint hostlen = 0;

	if (check_changed) {
		hostname_changed = GPOINTER_TO_INT(
						g_object_get_data(
							G_OBJECT(MainWindow.UsersWindow.hostnameentry),
							"changed"));
	}

	if ((check_changed && hostname_changed) || (!check_changed)) {
		hostname = (gchar *) gtk_entry_get_text(
						GTK_ENTRY(MainWindow.UsersWindow.hostnameentry));
		if (is_str_empty(hostname)) {
			errormsg = g_strdup_printf(_(PasswordErrorMarkup),
										_("A computer name is required."));
			ret_val = FALSE;
		} else {
			hostlen = strlen(hostname);
			if (hostlen > MAXHOSTNAMELEN) {
				errormsg =
					g_strdup_printf(
						_(PasswordErrorMarkup),
						_("Computer name exceeds maximum length."));
				ret_val = FALSE;
			} else if (invalid_hostname_character(hostname)) {
				errormsg =
					g_strdup_printf(
						_(PasswordErrorMarkup),
						_("Computer name contains invalid characters."));
				ret_val = FALSE;
			} else if (hostname[hostlen-1] == '-' ||
						hostname[hostlen-1] == '_' ||
						hostname[hostlen-1] == '.') {
				errormsg =
					g_strdup_printf(
						_(PasswordErrorMarkup),
						_("Computer name ends with invalid character."));
				ret_val = FALSE;
			}
		}

		if (check_changed) {
			g_object_set_data(
					G_OBJECT(MainWindow.UsersWindow.hostnameentry),
					"changed",
					GINT_TO_POINTER(FALSE));
		}
	}

	if (!ret_val) {
		if (is_str_empty(hostname)) {
			gtk_entry_set_text(
					GTK_ENTRY(MainWindow.UsersWindow.hostnameentry),
					"openindiana");
		}
		gtk_widget_grab_focus(
				MainWindow.UsersWindow.hostnameentry);
		gtk_widget_show(MainWindow.UsersWindow.hostnameinfoimage);
		gtk_label_set_label(
					GTK_LABEL(MainWindow.UsersWindow.hostnameinfolabel),
					errormsg);
		MainWindow.UsersWindow.error_posted = TRUE;
		g_free(errormsg);
	}
	return (ret_val);
}
