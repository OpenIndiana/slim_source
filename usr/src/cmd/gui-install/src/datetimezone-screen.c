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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <ctype.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <time.h>
#include <libzoneinfo.h>

#include <glade/glade.h>
#include "callbacks.h"
#include "interface-globals.h"
#include "datetimezone-screen.h"
#include "timezone.h"

#undef ANALOG_CLOCK

/* 0 = am, 1 = pm, 2 = 24 */
gint ampmmode = 2; /* Default is 24 hour mode */

/* Forward declarations */
gboolean datetimezone_set_current_date_and_time(void);
gboolean update_clock(void);

/* Callbacks referenced in Glade XML file */

void
on_yearspinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	gint year;
	GDateMonth month;
	gint day;
	gint daysinmonth;

	year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	month = (GDateMonth) gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
		(MainWindow.DateTimeZoneWindow.monthspinner));

	/* Need to check the month of february's range for leap year transitions */
	if (month == G_DATE_FEBRUARY) {
		daysinmonth = g_date_get_days_in_month(month, (GDateYear)year);
		day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(
			MainWindow.DateTimeZoneWindow.dayspinner));
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(
			MainWindow.DateTimeZoneWindow.dayspinner), 1, daysinmonth);

		if (day > daysinmonth) {
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(
				MainWindow.DateTimeZoneWindow.dayspinner), daysinmonth);
		} else {
			/*
			 * The spin button seems to clobber it's current value when
			 * you set it's range, even when valid
			 */
			gtk_spin_button_set_value(GTK_SPIN_BUTTON(
				MainWindow.DateTimeZoneWindow.dayspinner), day);
		}
	}

}

void
on_monthspinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	GtkSpinButton *yearspinner;
	gint year;
	static gint prevmonth = -1;
	gint month;
	gint day;
	gint daysinmonth;

	year = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
		(MainWindow.DateTimeZoneWindow.yearspinner));
	month = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	if (month < 10) {
		gchar *monthtext = g_strdup_printf("%02d", month);
		gtk_entry_set_text(GTK_ENTRY(widget), monthtext);
		g_free(monthtext);
	}
	day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON
		(MainWindow.DateTimeZoneWindow.dayspinner));

	/*
	 * Clamp the range for the day spinner based on the month and leap
	 * year status
	 */
	daysinmonth = g_date_get_days_in_month((GDateMonth)month, (GDateYear)year);
	gtk_spin_button_set_range(GTK_SPIN_BUTTON(
		MainWindow.DateTimeZoneWindow.dayspinner), 1, daysinmonth);
	/* Adjust the current day value if it exceeds the days in that month */
	if (day > daysinmonth) {
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(
			MainWindow.DateTimeZoneWindow.dayspinner), daysinmonth);
	} else {
		/*
		 * The spin button seems to clobber it's current value when you
		 * set it's range, even when valid
		 */
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(
			MainWindow.DateTimeZoneWindow.dayspinner), day);
	}
#ifdef ANALOG_CLOCK
	yearspinner = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.yearspinner);
	if (prevmonth == 12 && month == 1) { /* Increment year */
		gtk_spin_button_spin(yearspinner, GTK_SPIN_STEP_FORWARD, 1);
	} else if (prevmonth == 1 && month == 12) { /* Decrement year */
		gtk_spin_button_spin(yearspinner, GTK_SPIN_STEP_BACKWARD, 1);
	}
	prevmonth = month;
#endif
}

void
on_dayspinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	GtkSpinButton *yearspinner;
	GtkSpinButton *monthspinner;
	gdouble firstday, lastday;
	static int prevday = -1;
	gint day;

	day = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

#ifdef ANALOG_CLOCK
	if (prevday < 0) {
		prevday = day;
		return;
	}

	monthspinner = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.monthspinner);
	gtk_spin_button_get_range(GTK_SPIN_BUTTON(widget), &firstday, &lastday);

	if (prevday == lastday && day == 1) { /* increment month */
		gtk_spin_button_spin(monthspinner, GTK_SPIN_STEP_FORWARD, 1);
	} else if (prevday == 1 && day == lastday) { /* decrement month */
		/*
		 * Tricky because the last day of the
		 * previous month could be different + feb. differs in leap years
		 */
		gint year;
		gint month;
		gint daysinmonth;

		yearspinner =
			GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.yearspinner);
		year = gtk_spin_button_get_value_as_int(yearspinner);
		month = gtk_spin_button_get_value_as_int(monthspinner);
		if (month == 1)
			month = 12;
		else
			month--;
		daysinmonth =
			g_date_get_days_in_month((GDateMonth)month, (GDateYear)year);
		gtk_spin_button_spin(monthspinner, GTK_SPIN_STEP_BACKWARD, 1);
		g_signal_handlers_block_matched((gpointer *)widget,
							G_SIGNAL_MATCH_FUNC,
							-1,
							g_quark_from_string("value_changed"),
							NULL,
							(gpointer *)on_dayspinner_value_changed,
							NULL);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), daysinmonth);
		g_signal_handlers_unblock_matched((gpointer *)widget,
							G_SIGNAL_MATCH_FUNC,
							-1,
							g_quark_from_string("value_changed"),
							NULL,
							(gpointer *)on_dayspinner_value_changed,
							NULL);

	}
	prevday = day;
#endif /* ANALOG_CLOCK */
}

void
on_hourspinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	GtkSpinButton *dayspinner;
	GtkComboBox *ampmcombo;
	static int prevhour = -1;
	gint ampmmode;
	gint hour;

#ifdef ANALOG_CLOCK
	hour = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));

	if (prevhour < 0) {
		prevhour = hour;
		return;
	}

	dayspinner =
		GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.dayspinner);
	ampmcombo =
		GTK_COMBO_BOX(MainWindow.DateTimeZoneWindow.ampmcombobox);
	ampmmode =
		gtk_combo_box_get_active(ampmcombo);
	switch (ampmmode) {
		case 0: /* am */
			if (prevhour == 11 && hour == 12) /* AM -> PM */
				gtk_combo_box_set_active(ampmcombo, 1);
			else if (prevhour == 12 && hour == 11) { /* AM -> PM(yesterday) */
				gtk_combo_box_set_active(ampmcombo, 1);
				gtk_spin_button_spin(dayspinner, GTK_SPIN_STEP_BACKWARD, 1);
			}
			break;
		case 1: /* pm */
			if (prevhour == 11 && hour == 12) { /* PM -> AM (next day) */
				gtk_combo_box_set_active(ampmcombo, 0);
				gtk_spin_button_spin(dayspinner, GTK_SPIN_STEP_FORWARD, 1);
			} else if (prevhour == 12 && hour == 11) /* PM -> AM */
				gtk_combo_box_set_active(ampmcombo, 0);
			break;
		case 2: /* 24 hour mode */
			if (prevhour == 23 && hour == 0) { /* next day */
				gtk_spin_button_spin(dayspinner, GTK_SPIN_STEP_FORWARD, 1);
			} else if (prevhour == 0 && hour == 23) { /* yesterday */
				gtk_spin_button_spin(dayspinner, GTK_SPIN_STEP_BACKWARD, 1);
			}
			break;
	}
	prevhour = hour;
#endif /* ANALOG_CLOCK */
}

void
on_minutespinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	GtkSpinButton *hourspinner;
	static int prevminute = -1;
	gint minute;

	minute = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(widget));
	if (minute < 10) {
		gchar *minutetext = g_strdup_printf("%02d", minute);
		gtk_entry_set_text(GTK_ENTRY(widget), minutetext);
		g_free(minutetext);
	}

#ifdef ANALOG_CLOCK
	if (prevminute < 0) {
		prevminute = minute;
		return;
	}

	hourspinner =
			GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.hourspinner);
	if (prevminute == 59 && minute == 0) {
		gtk_spin_button_spin(hourspinner, GTK_SPIN_STEP_FORWARD, 1);
	} else if (prevminute == 0 && minute == 59) {
		gtk_spin_button_spin(hourspinner, GTK_SPIN_STEP_BACKWARD, 1);
	}
	prevminute = minute;
#endif
}

void
on_ampmcombobox_changed(GtkWidget *widget, gpointer user_data)
{
	static int previndex = 2; /* Initially use 24 hour mode */
	int index, newindex;
	int oldhours, newhours;
	GtkSpinButton *hourspinner;
	GtkComboBox *combo;

	combo = GTK_COMBO_BOX(widget);
	hourspinner = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.hourspinner);
	oldhours = gtk_spin_button_get_value_as_int(hourspinner);
	newindex = index = gtk_combo_box_get_active(combo);

	if (index == 2) { /* 24 hour mode */
		gtk_spin_button_set_range(hourspinner, 0, 23);
		/* If converting from 12-hr PM to 24 */
		if (previndex == 0) { /* 12:00am becomes 00:00 */
			if (oldhours == 12)
				newhours = 0;
			else
				newhours = oldhours;
		} else if (previndex == 1) { /* 1:00pm becomes 13:00 etc. */
			if (oldhours == 12)
				newhours = 12;
			else if (oldhours < 12)
				newhours = oldhours + 12;
		}
	} else if (index == 1) { /* PM */
		if (previndex == 2)  { /* 24hour -> PM */
			gtk_spin_button_set_range(hourspinner, 1, 12);
				if (oldhours % 12 == 0)
					newhours = 12;
				else if (oldhours > 12)
					newhours = oldhours - 12;
				else
					newhours = oldhours;
		} else if (previndex == 0) { /* PM -> AM */
			newhours = oldhours;
		}
	} else if (index == 0) { /* AM */
		if (previndex == 2) { /* 24hr -> AM */
			gtk_spin_button_set_range(hourspinner, 1, 12);
			if (oldhours % 12 == 0)
				newhours = 12;
			else if (oldhours < 12)
				newhours = oldhours;
			else if (oldhours > 12) {
				newhours = oldhours - 12;
			}
		} else if (previndex == 1) { /* PM -> AM */
			newhours = oldhours;
		}
	}

	gtk_spin_button_set_value(hourspinner, newhours);
	previndex = newindex;
}

static void
datetimezone_spinners_filter(GtkEntry *widget,
			const gchar *newtext,
			gint length,
			gint *position)
{
	const gchar *currenttext;
	gchar newnumstring [4];
	gint newnum;
	gdouble min, max;

	if (length > 1)
		return;

	currenttext = gtk_entry_get_text(GTK_ENTRY(widget));
	gtk_spin_button_get_range(GTK_SPIN_BUTTON(widget),
			&min, &max);
	/*
	 * Need to generate newstring based on insertion position
	 */
	if ((*position == 0) && (strlen(currenttext) > 0))
		snprintf(newnumstring, 4, "%c%s%c", *newtext, currenttext, 0);
	else
		snprintf(newnumstring, 4, "%s%c%c", currenttext, *newtext, 0);
	newnum = atoi(newnumstring);

	/*
	 * Block things like nonnumeric characters, characters outside range,
	 * and strings that are too long
	 */
	if ((!isdigit(*newtext)) ||
		(newnum > (gint)max) || (strlen(newnumstring) > 2)) {
		gdk_beep();
		g_signal_stop_emission_by_name(GTK_OBJECT(widget), "insert_text");
		return;
	}
}

static void
datetimezone_spinners_focus_out(GtkWidget *widget,
			GdkEventFocus *event,
			gpointer user_data)
{
	gchar *val = gtk_editable_get_chars(GTK_EDITABLE(widget), 0, -1);
	gint   num = atoi(val);
	gchar *value;

	gtk_spin_button_set_value(GTK_SPIN_BUTTON(widget), (gfloat)num);

	value = g_strdup_printf("%02d", num);
	gtk_entry_set_text(GTK_ENTRY(widget), value);

	g_free(value);
	g_free(val);
}

gboolean
get_selected_tz(InstallationProfileType *profile)
{
	Timezone *timezone =
		TIMEZONE(MainWindow.DateTimeZoneWindow.timezone);

	return timezone_get_selected_tz(timezone, profile);
}

void
datetimezone_xml_init(void)
{
	/* widgets for date, time */
	MainWindow.datetimezonewindowxml =
		glade_xml_new(GLADEDIR "/" DATETIMEZONEFILENAME,
				DATETIMEZONENODE, NULL);
	MainWindow.DateTimeZoneWindow.datetimezonetoplevel =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml,
				"datetimezonetoplevel");

	MainWindow.DateTimeZoneWindow.outervbox =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "outervbox");
	MainWindow.DateTimeZoneWindow.yearspinner =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "yearspinner");
	MainWindow.DateTimeZoneWindow.monthspinner =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "monthspinner");
	MainWindow.DateTimeZoneWindow.dayspinner =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "dayspinner");
	MainWindow.DateTimeZoneWindow.hourspinner =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "hourspinner");
	MainWindow.DateTimeZoneWindow.minutespinner =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "minutespinner");
	MainWindow.DateTimeZoneWindow.ampmcombobox =
		glade_xml_get_widget(MainWindow.datetimezonewindowxml, "ampmcombobox");
}

void
datetimezone_ui_init(void)
{
	GtkSizeGroup *sizegroup;
	GtkWidget *label;
	GtkWidget *timezone;

	timezone = timezone_new();
	MainWindow.DateTimeZoneWindow.timezone = timezone;
	gtk_widget_show(MainWindow.DateTimeZoneWindow.timezone);
	gtk_box_pack_start(GTK_BOX(MainWindow.DateTimeZoneWindow.outervbox),
		MainWindow.DateTimeZoneWindow.timezone, FALSE, FALSE, 0);
	gtk_box_reorder_child(GTK_BOX(MainWindow.DateTimeZoneWindow.outervbox),
		MainWindow.DateTimeZoneWindow.timezone, 0);

	gtk_box_pack_start(GTK_BOX(MainWindow.screencontentvbox),
		MainWindow.DateTimeZoneWindow.datetimezonetoplevel, TRUE, TRUE, 0);
	gtk_entry_set_alignment(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.yearspinner), 1.0);
	gtk_entry_set_alignment(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.monthspinner), 1.0);
	gtk_entry_set_alignment(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.minutespinner), 1.0);
	gtk_entry_set_alignment(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.hourspinner), 1.0);
	gtk_entry_set_alignment(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.minutespinner), 1.0);
	gtk_entry_set_width_chars(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.monthspinner), 2);
	gtk_entry_set_width_chars(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.dayspinner), 2);
	gtk_entry_set_width_chars(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.hourspinner), 2);
	gtk_entry_set_width_chars(
		GTK_ENTRY(MainWindow.DateTimeZoneWindow.minutespinner), 2);

	sizegroup = gtk_size_group_new(GTK_SIZE_GROUP_BOTH);
	label = timezone_get_continent_label(TIMEZONE(timezone));
	gtk_size_group_add_widget(sizegroup, label);
	label = timezone_get_country_label(TIMEZONE(timezone));
	gtk_size_group_add_widget(sizegroup, label);
	label = timezone_get_timezone_label(TIMEZONE(timezone));
	gtk_size_group_add_widget(sizegroup, label);
	label = glade_xml_get_widget(MainWindow.datetimezonewindowxml,
			"datelabel");
	gtk_size_group_add_widget(sizegroup, label);
	label = glade_xml_get_widget(MainWindow.datetimezonewindowxml,
			"timelabel");
	gtk_size_group_add_widget(sizegroup, label);

	/* Use 24hour mode initially */
	gtk_combo_box_set_active(
		GTK_COMBO_BOX(MainWindow.DateTimeZoneWindow.ampmcombobox), 2);

	/* UI is initialised correctly so we can connect up the signals now */
	glade_xml_signal_autoconnect(MainWindow.datetimezonewindowxml);

	g_signal_connect(G_OBJECT(MainWindow.DateTimeZoneWindow.monthspinner),
		"insert_text",
		G_CALLBACK(datetimezone_spinners_filter),
		NULL);
	g_signal_connect(G_OBJECT(MainWindow.DateTimeZoneWindow.dayspinner),
		"insert_text",
		G_CALLBACK(datetimezone_spinners_filter),
		NULL);
	g_signal_connect(G_OBJECT(MainWindow.DateTimeZoneWindow.hourspinner),
		"insert_text",
		G_CALLBACK(datetimezone_spinners_filter),
		NULL);
	g_signal_connect(G_OBJECT(MainWindow.DateTimeZoneWindow.minutespinner),
		"insert_text",
		G_CALLBACK(datetimezone_spinners_filter),
		NULL);

	g_signal_connect_after(G_OBJECT(MainWindow.DateTimeZoneWindow.monthspinner),
		"focus_out_event",
		G_CALLBACK(datetimezone_spinners_focus_out),
		NULL);
	g_signal_connect_after(G_OBJECT(MainWindow.DateTimeZoneWindow.dayspinner),			"focus_out_event",
		G_CALLBACK(datetimezone_spinners_focus_out),
		NULL);
	g_signal_connect_after(
		G_OBJECT(MainWindow.DateTimeZoneWindow.minutespinner),
		"focus_out_event",
		G_CALLBACK(datetimezone_spinners_focus_out),
		NULL);

	g_timeout_add(1000, (GSourceFunc)update_clock, NULL);
}


/*
 * Sets the UI date and time based on the
 * current system time
 */
gboolean
datetimezone_set_current_date_and_time(void)
{
	GtkComboBox *ampmcombo;
	GTimeVal currenttimeval;
	GDate currentdate;
	gint hourval;

	struct tm *currentzone;
	time_t currenttime;

	ampmcombo = GTK_COMBO_BOX(MainWindow.DateTimeZoneWindow.ampmcombobox);

	g_get_current_time(&currenttimeval);
	g_date_set_time_val(&currentdate, &currenttimeval);
	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.yearspinner),
		currentdate.year);

	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.monthspinner),
		currentdate.month);

	if (currentdate.month < 10) {
		gchar *monthtext = g_strdup_printf("%02d", currentdate.month);
		gtk_entry_set_text(
			GTK_ENTRY(MainWindow.DateTimeZoneWindow.monthspinner), monthtext);
		g_free(monthtext);
	}

	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.dayspinner),
		currentdate.day);
	if (currentdate.day < 10) {
		gchar *daytext = g_strdup_printf("%02d", currentdate.day);
		gtk_entry_set_text(GTK_ENTRY(MainWindow.DateTimeZoneWindow.dayspinner),
			daytext);
		g_free(daytext);
	}

	currenttime = time(NULL);
	currentzone = localtime(&currenttime);

	if (ampmmode != 2) {
		if (currentzone->tm_hour < 12) {
			if (currentzone->tm_hour == 0) {
				hourval = 12;
			}
			gtk_combo_box_set_active(ampmcombo, 0);
		} else if (currentzone->tm_hour > 12) {
			hourval = currentzone->tm_hour - 12;
			gtk_combo_box_set_active(ampmcombo, 1);
		}
	} else {
		hourval = currentzone->tm_hour;
	}
	gtk_spin_button_set_value(GTK_SPIN_BUTTON
		(MainWindow.DateTimeZoneWindow.hourspinner), hourval);
	if (hourval < 10) {
		gchar *hourtext = g_strdup_printf("%02d", hourval);
		gtk_entry_set_text(
		    GTK_ENTRY(MainWindow.DateTimeZoneWindow.minutespinner),
		    hourtext);
		g_free(hourtext);
	}

	gtk_spin_button_set_value(
		GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.minutespinner),
		currentzone->tm_min);
	if (currentzone->tm_min < 10) {
		gchar *minutetext = g_strdup_printf("%02d", currentzone->tm_min);
		gtk_entry_set_text(
			GTK_ENTRY(MainWindow.DateTimeZoneWindow.minutespinner),
			minutetext);
		g_free(minutetext);
	}

	return (TRUE);
}

void
datetimezone_set_system_clock(gboolean reallysetit)
{
	GtkSpinButton *year, *month, *day;
	GtkSpinButton *hour, *minute;
	GtkComboBox *ampm;
	time_t oldsystemtime;
	time_t newsystemtime;
	gchar *tzenv;
	int status = OM_SUCCESS;

	year = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.yearspinner);
	month = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.monthspinner);
	day = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.dayspinner);
	hour = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.hourspinner);
	minute = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.minutespinner);
	ampm = GTK_COMBO_BOX(MainWindow.DateTimeZoneWindow.ampmcombobox);

	struct tm *oldtime;
	struct tm *newtime;

	newtime = g_new0(struct tm, 1);
#if 0
	int tm_sec;		/* seconds after the minute [0, 60]  */
	int tm_min;		/* minutes after the hour [0, 59] */
	int tm_hour;	/* hour since midnight [0, 23] */
	int tm_mday;	/* day of the month [1, 31] */
	int tm_mon;		/* months since January [0, 11] */
	int tm_year;	/* years since 1900 */
	int tm_wday;	/* days since Sunday [0, 6] */
	int tm_yday;	/* days since January 1 [0, 365] */
	int tm_isdst;	/* flag for daylight savings time */
#endif

	newtime->tm_sec = 0;
	newtime->tm_mday = -1;
	newtime->tm_wday = -1;
	newtime->tm_yday = -1;
	newtime->tm_isdst = -1;

	newtime->tm_hour = 0;
	if (gtk_combo_box_get_active(ampm) == 1)
		newtime->tm_hour = 12;

	newtime->tm_min =
		gtk_spin_button_get_value_as_int(minute);
	newtime->tm_hour +=
		gtk_spin_button_get_value_as_int(hour);
	newtime->tm_mday =
		gtk_spin_button_get_value_as_int(day);
	newtime->tm_mon =
		gtk_spin_button_get_value_as_int(month) - 1;
	newtime->tm_year =
		gtk_spin_button_get_value_as_int(year) - 1900;

	/*
	 * Set up the TZ environment before setting the system time.
	 * Timezone is already set in profile by get_selected_tz()
	 * so it's safe to reference it now. om_set_time_zone() takes
	 * care of the TZ env and sets the rtc.
	 */
	status = om_set_time_zone(InstallationProfile.timezone->tz_name);
	if (status != OM_SUCCESS) {
		/* Not a fatal error */
		g_warning("om_set_time_zone() failed. Failure code: %d",
		    om_get_error());
		g_warning("System time will probably be wrong after reboot");
	}
	tzset();

	oldsystemtime = time(NULL);
	oldtime = localtime(&oldsystemtime);
	/* Preserve the seconds value since the GUI doesn't let you set it */
	newtime->tm_sec = oldtime->tm_sec;
	newsystemtime = mktime(newtime);
	/* Set the time for real */
	if (stime(&newsystemtime) < 0) {
		g_warning("Failed to set system clock: %s",
			g_strerror(errno));
	}
}

gboolean
update_clock(void)
{
	static int currentmin = -1;
	GtkSpinButton *minute;

	if (currentmin < 0) {
		minute = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.minutespinner);
		currentmin = gtk_spin_button_get_value_as_int(minute);
	}

	static struct tm *prevtime = NULL;
	struct tm *newtime;

	time_t prevsystemtime;
	time_t newsystemtime;

	if (!prevtime) {
		prevsystemtime = time(NULL);
		prevtime = g_new0(struct tm, 1);
		prevtime = gmtime(&prevsystemtime);
		return (TRUE);
	}

	newsystemtime = time(NULL);
	newtime = g_new0(struct tm, 1);
	newtime = gmtime(&newsystemtime);

	if (currentmin != newtime->tm_min) {
		minute = GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.minutespinner);
		gtk_spin_button_spin(minute, GTK_SPIN_STEP_FORWARD, 1);
		currentmin = newtime->tm_min;
		if (currentmin == 0) { /* Increment the hour spinner too */
			GtkSpinButton *hour =
				GTK_SPIN_BUTTON(MainWindow.DateTimeZoneWindow.hourspinner);
			gtk_spin_button_spin(hour, GTK_SPIN_STEP_FORWARD, 1);
		}
	}
	prevtime = newtime;
	return (TRUE);
}

/*
 * Set the default widget with focus for datetimezone screen.
 */
void
datetimezone_screen_set_default_focus(void)
{
	timezone_set_default_focus(TIMEZONE(MainWindow.DateTimeZoneWindow.timezone));
	datetimezone_set_current_date_and_time();
}
