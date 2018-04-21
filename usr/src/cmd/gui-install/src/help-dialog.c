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

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <glib/gstdio.h>

#include "installation-profile.h"
#include "interface-globals.h"
#include "window-graphics.h"
#include "help-dialog.h"

gboolean
show_file_in_textview(GtkWidget *textview,
						gchar *filename,
						gboolean bold,
						gboolean centered,
						gboolean dontprocessCR)
{
	FILE *fp;
	char ch, prevch;
	gboolean ret_val = TRUE;
	char str[MAXBUFFER];
	GtkTextBuffer *textBuffer = NULL;
	GtkTextTagTable *textTagTable = NULL;
	GtkTextTag *textTagBold = NULL;
	GtkTextTag *textTagUnderline = NULL;
	GtkTextTag *textTagCenter = NULL;
	int lineNum = 0, chNum = 0;
	GtkTextIter start, end;

	/* bold/centered paramaters apply only to the first line of input */

	if (!filename) {
		g_warning("Error : Must provide file name to read\n");
		return (FALSE);
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		g_warning("Error : Failed to open file : %s\n", filename);
		return (FALSE);
	}

	textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));

	if ((!bold && !centered) && dontprocessCR) {
		/* Example would be install log, we just want to dump contents */
		/* into the textview without any formatting */
		while (fgets(str, MAXBUFFER, fp)) {
			gtk_text_buffer_insert_at_cursor(textBuffer, (const gchar *)str,
					strlen(str));
		}

	} else {
		if (bold || centered) {
			textTagTable = gtk_text_buffer_get_tag_table(textBuffer);

			if (textTagTable) {
				textTagBold = gtk_text_tag_table_lookup(textTagTable, "Bold");
				textTagUnderline =
					gtk_text_tag_table_lookup(textTagTable, "Underline");
				textTagCenter = gtk_text_tag_table_lookup(textTagTable,
															"Center");
			}

			if (!textTagBold) {
				textTagBold = gtk_text_buffer_create_tag(textBuffer, "Bold",
										"weight", PANGO_WEIGHT_BOLD, NULL);
			}
			if (!textTagUnderline) {
				textTagUnderline =
					gtk_text_buffer_create_tag(textBuffer, "Underline",
						"underline", PANGO_UNDERLINE_SINGLE, NULL);
			}
			if (!textTagCenter) {
				textTagCenter = gtk_text_buffer_create_tag(textBuffer,
										"Center", "justification",
										GTK_JUSTIFY_CENTER, NULL);
			}
		}

		lineNum = 0;
		chNum = 0;
		while ((ch = fgetc(fp)) != EOF) {
			if (ch == '\n') {
				lineNum++;
				if (dontprocessCR) {
					/* Just simply process the first line for BOLD/CENTER */
					if (lineNum == 1) {
						prevch = '\0';
						str[chNum] = '\n';
						str[chNum+1] = '\n';
						str[chNum+2] = '\0';
						gtk_text_buffer_insert_at_cursor(textBuffer,
											(const gchar *)str,
											strlen(str));
						chNum = 0;
						gtk_text_buffer_get_bounds(textBuffer, &start, &end);

						if (bold) {
							gtk_text_buffer_apply_tag_by_name(textBuffer,
													"Bold", &start, &end);
							gtk_text_buffer_apply_tag_by_name(textBuffer,
													"Underline", &start, &end);
						}
						if (centered) {
							gtk_text_buffer_apply_tag_by_name(textBuffer,
													"Center", &start, &end);
						}
					} else {
						prevch = ch;
						str[chNum] = ch;
						chNum++;

						if (chNum == (MAXBUFFER-2)) {
							str[chNum] = '\0';
							gtk_text_buffer_insert_at_cursor(textBuffer,
													(const gchar *)str,
													strlen(str));
							chNum = 0;
						}
					}
				} else {
					if (prevch == '\n' || lineNum == 1) {
						prevch = '\0';
						str[chNum] = '\n';
						str[chNum+1] = '\n';
						str[chNum+2] = '\0';
						gtk_text_buffer_insert_at_cursor(textBuffer,
											(const gchar *)str,
											strlen(str));
						chNum = 0;
						if (lineNum == 1 && (bold || centered)) {
							gtk_text_buffer_get_bounds(textBuffer, &start,
														&end);

							if (bold) {
								gtk_text_buffer_apply_tag_by_name(textBuffer,
													"Bold", &start, &end);
								gtk_text_buffer_apply_tag_by_name(textBuffer,
													"Underline", &start, &end);
							}
							if (centered) {
								gtk_text_buffer_apply_tag_by_name(textBuffer,
													"Center", &start, &end);
							}
						}
					} else {
						if (chNum > 0) {
							prevch = ch;
							str[chNum] = ' ';
							chNum++;
						}
					}
				}
			} else {
				prevch = ch;
				str[chNum] = ch;
				chNum++;

				if (chNum == (MAXBUFFER-2)) {
					str[chNum] = '\0';
					gtk_text_buffer_insert_at_cursor(textBuffer,
											(const gchar *)str,
											strlen(str));
					chNum = 0;
				}
			}
		}

		if (chNum > 0) {
			str[chNum] = '\0';
			gtk_text_buffer_insert_at_cursor(textBuffer, (const gchar *)str,
						strlen(str));
		}
	}

	return (ret_val);
}

gboolean
show_locale_file_in_textview(GtkWidget *textview,
				gchar *filename,
				gboolean bold,
				gboolean centered,
				gboolean dontprocessCR)
{
	gboolean ret_val = TRUE;
	FILE *fp = NULL;
	gint tfd = 0;
	gchar *tfilename = NULL;
	gchar *contents = NULL;
	gchar *contents_utf8 = NULL;
	gsize len = 0, len_utf8 = 0, bytes_read = 0;

	if (!filename) {
		g_warning("Error : Must provide file name to read\n");
		return (FALSE);
	}

	if ((fp = fopen(filename, "r")) == NULL) {
		g_warning("Error : Failed to open file : %s\n", filename);
		return (FALSE);
	}
	fclose(fp);

	if ((tfd = g_file_open_tmp("gui-install_localefile_XXXXXX",
					&tfilename, NULL)) == -1) {
		g_warning("Error : Failed to open temporary file.\n");
		return (FALSE);
	}
	close(tfd);

	if ((g_file_get_contents(filename, &contents, &len, NULL)) == FALSE) {
		g_warning("Error : Failed to get contents of file : %s\n", filename);
		g_unlink(tfilename);
		g_free(tfilename);
		return (FALSE);
	}

	if ((contents_utf8 = g_locale_to_utf8((const gchar *)contents, len,
			&bytes_read, &len_utf8, NULL)) == NULL) {
		g_warning("Error : Failed to convert contents of file to UTF-8 : %s\n",
			filename);
		g_free(contents);
		g_unlink(tfilename);
		g_free(tfilename);
		return (FALSE);
	}

	if ((g_file_set_contents((const gchar *)tfilename,
			(const gchar *)contents_utf8, len_utf8, NULL)) == FALSE) {
		g_warning("Error : Failed to write to tmp file : %s\n", tfilename);
		g_free(contents);
		g_free(contents_utf8);
		g_unlink(tfilename);
		g_free(tfilename);
		return (FALSE);
	}

	ret_val = show_file_in_textview(textview, tfilename,
						bold, centered, dontprocessCR);

	g_free(contents);
	g_free(contents_utf8);
	g_unlink(tfilename);
	g_free(tfilename);

	return (ret_val);
}

void
delete_textview_contents(GtkWidget *textview)
{
	GtkTextBuffer *textBuffer;
	textBuffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(textview));
	gtk_text_buffer_set_text(textBuffer, "", -1);
}

void
help_dialog_show(InstallScreen currScreen, gboolean bringToFront)
{
	gchar *tmpFileName = NULL;

	switch (currScreen) {
		case WELCOME_SCREEN :
			tmpFileName = MainWindow.TextFileLocations[HELP_WELCOME];
			break;
		case DISK_SCREEN :
			switch (InstallationProfile.installationtype) {
				case INSTALLATION_TYPE_INITIAL_INSTALL:
					tmpFileName =
								MainWindow.TextFileLocations[HELP_INSTALL_DISK];
					break;
			}
			break;
		case TIMEZONE_SCREEN :
			tmpFileName = MainWindow.TextFileLocations[HELP_INSTALL_TIMEZONE];
			break;
		case LANGUAGE_SCREEN :
			tmpFileName = MainWindow.TextFileLocations[HELP_INSTALL_LANGUAGE];
			break;
		case USER_SCREEN :
			tmpFileName = MainWindow.TextFileLocations[HELP_INSTALL_USERS];
			break;
		case CONFIRMATION_SCREEN :
			switch (InstallationProfile.installationtype) {
				case INSTALLATION_TYPE_INITIAL_INSTALL:
					tmpFileName =
						MainWindow.TextFileLocations[HELP_INSTALL_CONFIRMATION];
					break;
			}
			break;
		case INSTALLATION_SCREEN :
			switch (InstallationProfile.installationtype) {
				case INSTALLATION_TYPE_INITIAL_INSTALL:
					tmpFileName =
							MainWindow.TextFileLocations[HELP_INSTALL_PROGRESS];
					break;
			}
			break;
		case FAILURE_SCREEN :
			switch (InstallationProfile.installationtype) {
				case INSTALLATION_TYPE_INITIAL_INSTALL:
					tmpFileName =
							MainWindow.TextFileLocations[HELP_INSTALL_FAILURE];
					break;
			}
			break;
		case FINISH_SCREEN :
			tmpFileName = MainWindow.TextFileLocations[HELP_FINISH];
			break;
	}

	if (tmpFileName) {
		delete_textview_contents(MainWindow.helptextview);
		show_file_in_textview(MainWindow.helptextview, tmpFileName,
								TRUE, FALSE, TRUE);
		if (bringToFront) {
			window_graphics_dialog_set_properties(MainWindow.helpdialog);
			gtk_widget_show(MainWindow.helpdialog);
		}
	}
}

void
help_dialog_hide(GtkWidget *widget,
				gpointer *dialog)
{
	gtk_widget_hide(GTK_WIDGET(dialog));
}

void
help_dialog_delete_event(GtkWidget *widget,
				gpointer user_data)
{
	gtk_widget_hide(widget);
}

gchar *
help_generate_file_path(gchar *path, gchar *locale_id, gchar *filename)
{
	gchar *tmpFileName = NULL;
	FILE *fp;

	/* Test locale_id first */
	if (locale_id && strcmp("C", locale_id)) {
		if (filename) {
			tmpFileName = g_strdup_printf("%s/%s/%s", path, locale_id, filename);
		} else {
			tmpFileName = g_strdup_printf("%s/%s", path, locale_id);
		}

		if ((fp = fopen(tmpFileName, "r")) != NULL) {
			fclose(fp);
		} else {
			g_free(tmpFileName);
			tmpFileName = NULL;
		}
	}

	/* Did not find locale specific file so try for C Locale */
	if (!tmpFileName) {
		if (filename) {
			tmpFileName = g_strdup_printf("%s/C/%s", path, filename);
		} else {
			tmpFileName = g_strdup_printf("%s/C", path );
		}

		if ((fp = fopen(tmpFileName, "r")) != NULL) {
			fclose(fp);
		} else {
			g_free(tmpFileName);
			tmpFileName = NULL;
		}
	}

	/* Finally check if file exists without any locale information at all */
	/* e.g. install log etc */
	if (!tmpFileName) {
		if (filename) {
			tmpFileName = g_strdup_printf("%s/%s", path, filename);
		} else {
			tmpFileName = g_strdup_printf("%s", path);
		}

		if ((fp = fopen(tmpFileName, "r")) != NULL) {
			fclose(fp);
		} else {
			g_free(tmpFileName);
			tmpFileName = NULL;
		}
	}

	return (tmpFileName);
}

void
help_dialog_refresh(InstallScreen currScreen)
{
	/* Is Help Dialog displayed at the moment */
	if (GTK_WIDGET_VISIBLE(MainWindow.helpdialog)) {
		help_dialog_show(currScreen, FALSE);
	}
}
