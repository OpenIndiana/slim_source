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

#include <ctype.h>
#include <math.h>

#include "orchestrator-wrappers.h"
#include "callbacks.h"
#include "interface-globals.h"
#include "window-graphics.h"
#include "disk-block-order.h"
#include "installation-disk-screen.h"
#include "installation-profile.h"
#include "error-logging.h"

/* Uncomment these 2 lines to simulate Sparc behaviour on X86 */
/* #define __sparc */
/* #undef __i386 */

#define	ONE_DECIMAL(x)	((round((x) * 10)) / 10.0)

typedef enum {
	DISK_STATUS_OK = 0,  /* Disk is fine for installation */
	DISK_STATUS_CANT_PRESERVE, /* Partition table is unreadable */
	DISK_STATUS_TOO_SMALL, /* Disk is too small - unusable */
	DISK_STATUS_NO_MEDIA, /* If size (in kb or mb = 0) */
	DISK_STATUS_NO_DISKINFO, /* Indicates target discovery error */
	DISK_STATUS_WARNING, /* Disk warning error */
	DISK_STATUS_LARGE_WARNING /* Disk > 2TB warning */
} DiskStatus;

/* Num of target disks found, including unusable ones */
static gint numdisks = 0;
/* Currently selected disk */
static gint activedisk = -1;
static gboolean activediskisreadable = FALSE;

static DiskStatus *alldiskstatus = NULL;
/* Ptr array of all disks - linked lists suck for random access */
static disk_info_t **alldiskinfo = NULL;
/*
 * Original reference copy of actual disk layout
 * (or the default layout if unreadable)
 */
static disk_parts_t **originalpartitions = NULL;
/* Working copy of the above. Customisations written here */
static disk_parts_t **modifiedpartitions = NULL;
/* Points to either one of the above */
static disk_parts_t **proposedpartitions = NULL;
/* A suggested layout that has one Solaris2 partition for the entire disk */
static disk_parts_t **defaultpartitions = NULL;

/* primaryblkorder and logicalblkorder for each disk */
static DiskBlockOrder **originalprimaryblkorder;
static DiskBlockOrder **modifiedprimaryblkorder;
static DiskBlockOrder **originallogicalblkorder;
static DiskBlockOrder **modifiedlogicalblkorder;

/* Max Width of Disk Combos */
static gint max_combo_width = 0;

/* Whether extended/logical Spinner has focus or not. */
static gboolean spinner_has_focus = FALSE;

/*
 * Signal handler id storage so we can easily block/unblock
 * the partitioning signal handlers that handle text insert
 * and delete events.
 */
static gulong spininserthandlers[FD_NUMPART] = {0, 0, 0, 0};
static gulong spindeletehandlers[FD_NUMPART] = {0, 0, 0, 0};

static GtkWidget *hbuttonbox;
static GtkWidget **diskbuttons;
static GtkAdjustment *viewportadjustment = NULL;
static GtkWidget *scanningbox = NULL;
static GtkIconTheme *icontheme;

/*
 * Partition type to string mappings.
 * Lifted straight out of fdisk.c
 */
static char Ostr[] = "Other OS"; /* New in fdisk.c */
static char Dstr[] = "DOS12";
static char D16str[] = "DOS16";
static char DDstr[] = "DOS-DATA";
static char EDstr[] = "EXT-DOS";
static char DBstr[] = "DOS-BIG";
static char PCstr[] = "PCIX";
static char Ustr[] = "UNIX System";
static char SUstr[] = "Solaris";
static char SU2str[] = "Solaris2";
static char X86str[] = "x86 Boot";
static char DIAGstr[] = "Diagnostic";
static char IFSstr[] = "IFS: NTFS";
static char AIXstr[] = "AIX Boot";
static char AIXDstr[] = "AIX Data";
static char OS2str[] = "OS/2 Boot";
static char WINstr[] = "Win95 FAT32";
static char EWINstr[] = "Ext Win95";
static char FAT95str[] = "FAT16 LBA";
static char EXTLstr[] = "EXT LBA";
static char LINUXstr[] = "Linux";
static char CPMstr[] = "CP/M";
static char NOV2str[] = "Netware 286"; /* New in fdisk.c */
static char NOVstr[] = "Netware 3.x+";
static char QNXstr[] = "QNX 4.x";
static char QNX2str[] = "QNX part 2";
static char QNX3str[] = "QNX part 3";
static char LINNATstr[] = "Linux native";
static char NTFSVOL1str[] = "NT volset 1";
static char NTFSVOL2str[] = "NT volset 2";
static char BSDstr[] = "BSD OS";
static char NEXTSTEPstr[] = "NeXTSTEP";
static char BSDIFSstr[] = "BSDI FS";
static char BSDISWAPstr[] = "BSDI swap"; /* New in fdisk.c */
static char EFIstr[] = "EFI";
static char Actvstr[] = "Active";   /* New in fdisk.c */
static char NAstr[] = "      "; /* New in fdisk.c */
static char Unused[] = "Unused";
static char Extended[] = "Extended";

static gchar *WarningLabelMarkup = N_("<span size=\"smaller\"><span font_desc=\"Bold\">Warning: </span> The data in this partition will be erased.</span>");

/* Forward declaration */

static void
disk_selection_set_active_disk(int disknum);

static void
disk_partitioning_set_from_parts_data(disk_info_t *diskinfo,
	disk_parts_t *partitions);

static GtkWidget *
create_diskbutton_icon(DiskStatus status, disk_info_t *diskinfo);

static void
set_diskbutton_icon(GtkWidget *button, GtkWidget *image);

static GtkWidget*
disk_toggle_button_new_with_label(const gchar *label,
	DiskStatus status, disk_info_t *diskinfo);

static void
disk_viewport_ui_init(GtkViewport *viewport);

static gchar*
disk_viewport_create_disk_tiptext(guint disknum);

static gchar *
disk_viewport_create_disk_label(guint disknum);

static void
disk_viewport_diskbuttons_init(GtkViewport *viewport);

static void
disk_comboboxes_ui_init(void);

static void
disk_comboboxes_ui_reset(void);

static void
disk_combobox_ui_init(GtkComboBox *combobox, gboolean is_primary);

static void
disk_combobox_ui_reset(GtkComboBox *combobox, gboolean is_primary);

static void
disk_partitioning_block_all_handlers(void);

static void
disk_partitioning_unblock_all_handlers(void);

static void
disk_partitioning_block_combox_handler(guint partindex);

static void
disk_partitioning_unblock_combox_handler(guint partindex);

static void
disk_partitioning_block_combobox_handlers(gint mask);

static void
disk_partitioning_unblock_combobox_handlers(gint mask);

static void
disk_partitioning_block_spinbox_handlers(gint mask);

static void
disk_partitioning_unblock_spinbox_handlers(gint mask);

static void
spinners_insert_text_filter(GtkEntry *widget,
	const gchar *newtext,
	gint length,
	gint *position,
	gpointer user_data);

static void
spinners_delete_text_filter(GtkEditable *widget,
	gint start_pos,
	gint end_pos,
	gpointer user_data);

static void
logical_spinners_insert_text_filter(GtkEntry *widget,
	const gchar *newtext,
	gint length,
	gint *position,
	gpointer user_data);

static void
logical_spinners_delete_text_filter(GtkEditable *widget,
	gint start_pos,
	gint end_pos,
	gpointer user_data);

static void
disk_partitioning_set_sensitive(gboolean sensitive);

static gboolean
disk_partitioning_button_focus_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data);

static void
viewport_adjustment_changed(GtkAdjustment *adjustment,
	gpointer user_data);

static void
init_disk_status(void);

static DiskStatus
get_disk_status(guint disknum);

static disk_parts_t *
installation_disk_create_default_layout(disk_info_t *diskinfo);

static gboolean
partition_discovery_monitor(gpointer user_data);

static void
update_disk_partitions_from_ui(disk_parts_t *partitions);

static void
update_logical_disk_partitions_from_ui(disk_parts_t *partitions);

static gboolean
disk_is_too_big(disk_info_t *diskinfo);

static gint
get_max_cell_renderer_width(void);

static void
reset_primary_partitions(gboolean block_handlers);

static void
initialize_default_partition_layout(gint disknum);

static void
logical_partition_spinner_value_changed(GtkWidget *widget,
	gpointer user_data);

static void
logical_partition_combo_changed(GtkWidget *widget,
	gpointer user_data);

static gboolean
logical_partition_spinner_focus_in_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data);

static gboolean
logical_partition_spinner_focus_out_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data);

static void
set_logical_combo_sensitivity(gint pidx,
	gboolean sensitivity,
	gboolean set_all);

static gfloat
calculate_avail_space(DiskBlockOrder *startblkorder,
	gint partindex,
	partition_info_t *partinfo);

static LogicalPartition *
get_logical_partition_at_pos(gint index,
	LogicalPartition *startlogical);

static void
restore_unused_partitions(guint disk_num, disk_parts_t *partitions);

static void
update_primary_unused_partition_size_from_ui(disk_parts_t *partitions,
	gint pidx,
	gfloat diffgb);

static gfloat
get_extended_partition_min_size(disk_parts_t *partitions);

/* Real Glade XML referenced callbacks */
void
installationdisk_wholediskradio_toggled(GtkWidget *widget, gpointer user_data)
{
#if defined(__i386)
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		return;

	proposedpartitions[activedisk] = defaultpartitions[activedisk];
	gtk_widget_hide(
	    MainWindow.InstallationDiskWindow.custompartitioningvbox);
	gtk_widget_show(
	    MainWindow.InstallationDiskWindow.diskwarninghbox);
#endif
}

void
installationdisk_partitiondiskradio_toggled(GtkWidget *widget,
	gpointer user_data)
{
#if defined(__i386)
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		return;

	proposedpartitions[activedisk] = modifiedpartitions[activedisk];
	gtk_widget_hide(
	    MainWindow.InstallationDiskWindow.diskwarninghbox);
	gtk_widget_show(
	    MainWindow.InstallationDiskWindow.custompartitioningvbox);
#endif
}

static void
show_warning_message(GtkWidget *warning, gboolean show)
{
	if (show == TRUE)
		gtk_widget_show(warning);
	else
		gtk_widget_hide(warning);
}

static void
update_logical_data_loss_warnings(LogicalPartition *startlogical)
{
	LogicalPartition *curlogical;

	/*
	 * Cycle throught all logical partitions, showing/hiding
	 * Warning box as appropriate
	 */
	for (curlogical = startlogical;
	    curlogical != NULL;
	    curlogical = curlogical->next) {
		if (curlogical->sizechange || curlogical->typechange) {
			show_warning_message(curlogical->warningbox, TRUE);
		} else {
			show_warning_message(curlogical->warningbox, FALSE);
		}
	}
}

static void
update_data_loss_warnings(void)
{
	gint i = 0;
	static GtkWidget **warnings = NULL;

	if (!warnings) {
		warnings = g_new0(GtkWidget *, FD_NUMPART);
		for (i = 0; i < FD_NUMPART; i++)
			warnings[i] =
			    MainWindow.InstallationDiskWindow.partwarnbox[i];
	}

	/*
	 * Cycle through primary paritions, if partition type or size has
	 * changed then show warning message
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		if (MainWindow.InstallationDiskWindow.parttypechanges[i] ||
		    MainWindow.InstallationDiskWindow.partsizechanges[i]) {
			show_warning_message(warnings[i], TRUE);
		} else {
			show_warning_message(warnings[i], FALSE);
		}

		/*
		 * If there is logical partitions defined, cycle through these
		 * And update warnings if type or size have changed here aswell
		 */
		if (MainWindow.InstallationDiskWindow.startlogical[i] != NULL) {
			update_logical_data_loss_warnings(
			    MainWindow.InstallationDiskWindow.startlogical[i]);
		}
	}
}

static void
set_range_avail_from_value(GtkSpinButton *spinner,
	GtkLabel *availlabel,
	gfloat fromval,
	gfloat toval)
{
	gchar *sizestr;

	if (spinner == NULL && availlabel == NULL) {
		return;
	}

	sizestr = g_strdup_printf("%.1f", toval);

	if (spinner != NULL) {
		gtk_spin_button_set_range(spinner, fromval,
		    fromval == 0 ? 0 : strtod(sizestr, NULL));
	}

	if (availlabel != NULL) {
		gtk_label_set_text(availlabel, sizestr);
		gtk_widget_show(GTK_WIDGET(availlabel));
	}

	g_free(sizestr);
}

static void
set_size_widgets_from_value(GtkSpinButton *spinner,
	GtkLabel *availlabel,
	gfloat size)
{
	gchar *sizestr;

	if (spinner == NULL && availlabel == NULL) {
		return;
	}

	sizestr = g_strdup_printf("%.1f", size);

	if (spinner != NULL) {
		gtk_spin_button_set_value(spinner, strtod(sizestr, NULL));
	}

	if (availlabel != NULL) {
		gtk_label_set_text(availlabel, sizestr);
		gtk_widget_show(GTK_WIDGET(availlabel));
	}

	g_free(sizestr);
}

static void
logical_partition_init(guint pidx,
	LogicalPartition *logicalpartition,
	gint top_attach,
	gint bottom_attach)
{
	g_return_if_fail(
	    MainWindow.InstallationDiskWindow.numpartlogical[pidx] > 0);

	/* Create type alignment */
	logicalpartition->typealign = gtk_alignment_new(0.5, 0.5, 1.0, 1.0);
	gtk_alignment_set_padding(GTK_ALIGNMENT(logicalpartition->typealign),
	    0,		/* Top Padding */
	    0,		/* Bottom Padding */
	    LOGICAL_COMBOBOX_INDENT,		/* Left Padding */
	    0);		/* Right Padding */

	/* Create and initialize type combo */
	logicalpartition->typecombo = gtk_combo_box_new();
	disk_combobox_ui_init(
	    GTK_COMBO_BOX(logicalpartition->typecombo), FALSE);
	logicalpartition->partcombosaved = UNUSED_PARTITION;
	logicalpartition->combochangehandler = g_signal_connect(
	    G_OBJECT(logicalpartition->typecombo),
	    "changed",
	    G_CALLBACK(logical_partition_combo_changed),
	    logicalpartition);
	g_signal_handler_block(
	    (gpointer *)logicalpartition->typecombo,
	    logicalpartition->combochangehandler);

	if (spinner_has_focus == TRUE) {
		gtk_widget_set_sensitive(logicalpartition->typecombo, FALSE);
	}

	/* Pack type combo into type alignment container */
	gtk_container_add(GTK_CONTAINER(logicalpartition->typealign),
	    logicalpartition->typecombo);

	/* pack type alignment container into fdisktable */
	gtk_table_attach(
	    GTK_TABLE(MainWindow.InstallationDiskWindow.fdisktable),
	    logicalpartition->typealign,
	    0,			/* Left Attach */
	    1,			/* Right Attach */
	    top_attach, /* Top attach */
	    bottom_attach,	/* Bottom Attach */
	    GTK_FILL,	/* Horizontal Options */
	    0,			/* Vertical Options */
	    0,			/* XPadding */
	    0);			/* YPadding */

	/* Create size spinner */
	logicalpartition->sizespinner = gtk_spin_button_new(
	    GTK_ADJUSTMENT(gtk_adjustment_new(0.0, 0.0, 1.0, 1.0, 0.0, 0.0)),
	    0.10,	/* Climb Rate */
	    1);		/* Digits */

	/* Added value changed signal handler */
	logicalpartition->spinnerchangehandler = g_signal_connect(
	    G_OBJECT(logicalpartition->sizespinner),
	    "value_changed",
	    G_CALLBACK(logical_partition_spinner_value_changed),
	    logicalpartition);
	logicalpartition->spinnerinserthandler = g_signal_connect(
	    G_OBJECT(logicalpartition->sizespinner),
	    "insert-text",
	    G_CALLBACK(logical_spinners_insert_text_filter),
	    logicalpartition);
	logicalpartition->spinnerdeletehandler = g_signal_connect(
	    G_OBJECT(logicalpartition->sizespinner),
	    "delete-text",
	    G_CALLBACK(logical_spinners_delete_text_filter),
	    logicalpartition);
	g_signal_connect(G_OBJECT(logicalpartition->sizespinner),
	    "focus-in-event",
	    G_CALLBACK(logical_partition_spinner_focus_in_handler),
	    logicalpartition);
	g_signal_connect(G_OBJECT(logicalpartition->sizespinner),
	    "focus-out-event",
	    G_CALLBACK(logical_partition_spinner_focus_out_handler),
	    logicalpartition);

	g_signal_handler_block(
	    (gpointer *)logicalpartition->sizespinner,
	    logicalpartition->spinnerchangehandler);
	g_signal_handler_block(
	    (gpointer *)logicalpartition->sizespinner,
	    logicalpartition->spinnerinserthandler);
	g_signal_handler_block(
	    (gpointer *)logicalpartition->sizespinner,
	    logicalpartition->spinnerdeletehandler);

	gtk_spin_button_set_range(
	    GTK_SPIN_BUTTON(logicalpartition->sizespinner),
	    0, 0);
	gtk_spin_button_set_value(
	    GTK_SPIN_BUTTON(logicalpartition->sizespinner),
	    0);
	gtk_widget_set_sensitive(logicalpartition->sizespinner, FALSE);

	/* pack size spinner into fdisktable */
	gtk_table_attach(
	    GTK_TABLE(MainWindow.InstallationDiskWindow.fdisktable),
	    logicalpartition->sizespinner,
	    1,			/* Left Attach */
	    2,			/* Right Attach */
	    top_attach, /* Top attach */
	    bottom_attach,	/* Bottom Attach */
	    GTK_FILL,	/* Horizontal Options */
	    0,			/* Vertical Options */
	    0,			/* XPadding */
	    0);			/* YPadding */

	/* Create size avail label */
	logicalpartition->availlabel = gtk_label_new("0.0");
	gtk_label_set_justify(GTK_LABEL(logicalpartition->availlabel),
	    GTK_JUSTIFY_RIGHT);
	gtk_misc_set_alignment(GTK_MISC(logicalpartition->availlabel),
	    0.0, 0.5);

	/* pack size avail label into fdisktable */
	gtk_table_attach(
	    GTK_TABLE(MainWindow.InstallationDiskWindow.fdisktable),
	    logicalpartition->availlabel,
	    2,			/* Left Attach */
	    3,			/* Right Attach */
	    top_attach, /* Top attach */
	    bottom_attach,	/* Bottom Attach */
	    GTK_FILL,	/* Horizontal Options */
	    GTK_FILL,	/* Vertical Options */
	    0,			/* XPadding */
	    0);			/* YPadding */

	/* Create warning hbox container */
	logicalpartition->warningbox = gtk_hbox_new(FALSE, 5);

	/* Create warning image */
	logicalpartition->warningimage = gtk_image_new_from_stock(
	    GTK_STOCK_DIALOG_WARNING,
	    GTK_ICON_SIZE_MENU);

	/* Create warning label */
	logicalpartition->warninglabel = gtk_label_new(_(WarningLabelMarkup));
	gtk_label_set_use_markup(
	    GTK_LABEL(logicalpartition->warninglabel), TRUE);

	/* Pack warning image and warning label into warning hbox container */
	gtk_box_pack_start(GTK_BOX(logicalpartition->warningbox),
	    logicalpartition->warningimage,
	    FALSE,
	    FALSE,
	    0);
	gtk_box_pack_start(GTK_BOX(logicalpartition->warningbox),
	    logicalpartition->warninglabel,
	    FALSE,
	    FALSE,
	    0);
	gtk_widget_show(logicalpartition->warningimage);
	gtk_widget_show(logicalpartition->warninglabel);

	/* Pack warning hbox container into fdisktable */
	gtk_table_attach(
	    GTK_TABLE(MainWindow.InstallationDiskWindow.fdisktable),
	    logicalpartition->warningbox,
	    3,			/* Left Attach */
	    4,			/* Right Attach */
	    top_attach, /* Top attach */
	    bottom_attach,	/* Bottom Attach */
	    GTK_FILL,	/* Horizontal Options */
	    GTK_FILL,	/* Vertical Options */
	    0,			/* XPadding */
	    0);			/* YPadding */

	/* Show all the widgets */
	gtk_widget_show_all(logicalpartition->typealign);
	gtk_widget_show_all(logicalpartition->sizespinner);
	gtk_widget_show_all(logicalpartition->availlabel);
	gtk_widget_hide(logicalpartition->warningbox);

	logicalpartition->sizechange = FALSE;
	logicalpartition->typechange = FALSE;
}

static void
resize_fdisk_table(guint num_rows)
{
	MainWindow.InstallationDiskWindow.fdisktablerows += num_rows;

	gtk_table_resize(
	    GTK_TABLE(MainWindow.InstallationDiskWindow.fdisktable),
	    MainWindow.InstallationDiskWindow.fdisktablerows,
	    4);
}

static void
relocate_widget(GtkWidget *container,
	GtkWidget *child,
	guint top_attach,
	guint bottom_attach)
{
	GValue gvalue_top_attach = {0};
	GValue gvalue_bottom_attach = {0};

	g_value_init(&gvalue_top_attach, G_TYPE_INT);
	g_value_init(&gvalue_bottom_attach, G_TYPE_INT);

	g_value_set_int(&gvalue_top_attach, top_attach);
	g_value_set_int(&gvalue_bottom_attach, bottom_attach);

	gtk_container_child_set_property(
	    GTK_CONTAINER(container),
	    child,
	    "top-attach",
	    &gvalue_top_attach);
	gtk_container_child_set_property(
	    GTK_CONTAINER(container),
	    child,
	    "bottom-attach",
	    &gvalue_bottom_attach);
}

static void
relocate_static_widgets(guint totalrows)
{
	/* Relocate Reset Button */
	relocate_widget(
	    MainWindow.InstallationDiskWindow.fdisktable,
	    MainWindow.InstallationDiskWindow.resetbutton,
	    totalrows-1,
	    totalrows);
}

static void
relocate_logical_widgets(LogicalPartition *curpartlogical,
	guint logical_attach)
{
	relocate_widget(
	    MainWindow.InstallationDiskWindow.fdisktable,
	    curpartlogical->typealign,
	    logical_attach,
	    logical_attach+1);

	relocate_widget(
	    MainWindow.InstallationDiskWindow.fdisktable,
	    curpartlogical->sizespinner,
	    logical_attach,
	    logical_attach+1);

	relocate_widget(
	    MainWindow.InstallationDiskWindow.fdisktable,
	    curpartlogical->availlabel,
	    logical_attach,
	    logical_attach+1);

	relocate_widget(
	    MainWindow.InstallationDiskWindow.fdisktable,
	    curpartlogical->warningbox,
	    logical_attach,
	    logical_attach+1);
}

static void
relocate_partition_widgets(guint pidx, guint num_rows)
{
	guint i = 0;
	LogicalPartition *curpartlogical = NULL;
	guint partlogicals = 0;
	guint logical_attach = 0;

	/* Primary partitions located from 0->3 */
	for (i = pidx+1; i < FD_NUMPART; i++) {
		/* Increase row number of this primary partition */
		MainWindow.InstallationDiskWindow.partrow[i] =
		    MainWindow.InstallationDiskWindow.partrow[i]+num_rows;

		/* Has this primary got any logical children */
		if (MainWindow.InstallationDiskWindow.startlogical[i] != NULL) {
			/*
			 * Loop through all children and shift down by num_rows
			 * The row to which this logical is attached is calculated
			 * MainWindow.InstallationDiskWindow.partrow[i], which has
			 * been adjusted already by num_rows.
			 */

			partlogicals = 0;
			curpartlogical =
			    MainWindow.InstallationDiskWindow.startlogical[i];
			for (; curpartlogical != NULL;
			    curpartlogical = curpartlogical->next) {
				partlogicals = partlogicals + 1;
				logical_attach =
				    MainWindow.InstallationDiskWindow.partrow[i] +
				    partlogicals;

				relocate_logical_widgets(
				    curpartlogical, logical_attach);
			}
		}

		/* partcombo[pidx] */
		relocate_widget(
		    MainWindow.InstallationDiskWindow.fdisktable,
		    MainWindow.InstallationDiskWindow.partcombo[i],
		    MainWindow.InstallationDiskWindow.partrow[i],
		    MainWindow.InstallationDiskWindow.partrow[i]+1);

		/* partspin[pidx] */
		relocate_widget(
		    MainWindow.InstallationDiskWindow.fdisktable,
		    MainWindow.InstallationDiskWindow.partspin[i],
		    MainWindow.InstallationDiskWindow.partrow[i],
		    MainWindow.InstallationDiskWindow.partrow[i]+1);

		/* partavail[pidx] */
		relocate_widget(
		    MainWindow.InstallationDiskWindow.fdisktable,
		    MainWindow.InstallationDiskWindow.partavail[i],
		    MainWindow.InstallationDiskWindow.partrow[i],
		    MainWindow.InstallationDiskWindow.partrow[i]+1);

		/* partwarnbox[pidx] */
		relocate_widget(
		    MainWindow.InstallationDiskWindow.fdisktable,
		    MainWindow.InstallationDiskWindow.partwarnbox[i],
		    MainWindow.InstallationDiskWindow.partrow[i],
		    MainWindow.InstallationDiskWindow.partrow[i]+1);
	}
}

static void
relocate_extended_widgets(gint pidx,	/* Primary pinfo index 0-3 */
	gint lidx,	/* Index into startlogicals  0-N */
	gint num_rows)
{
	guint i = 0;
	LogicalPartition *curpartlogical = NULL;
	guint partlogidx = 0;
	guint top_attach = 0;
	guint initial_top_attach = 0;
	guint bottom_attach = 0;
	gint log_partrow = 0;

	/* Relocate logical partitions starting at lidx+1 up to last logical */
	/* by number of num_rows */

	if (MainWindow.InstallationDiskWindow.startlogical[pidx] != NULL) {
		/* Loop through all children until we get to lidx item */
		partlogidx = 0;
		curpartlogical =
		    MainWindow.InstallationDiskWindow.startlogical[pidx];
		for (; curpartlogical != NULL;
		    curpartlogical = curpartlogical->next) {
			if (partlogidx > lidx) {
				log_partrow =
				    MainWindow.InstallationDiskWindow.partrow[pidx] +
				    partlogidx + 1;

				top_attach = log_partrow + num_rows;

				relocate_logical_widgets(
				    curpartlogical, top_attach);
			}
			partlogidx = partlogidx + 1;
		}
	}
}

static void
logical_partition_destroy_ui(LogicalPartition *rmlogical)
{
	g_return_if_fail(rmlogical != NULL);

	gtk_widget_destroy(rmlogical->typecombo);
	gtk_widget_destroy(rmlogical->typealign);
	gtk_widget_destroy(rmlogical->sizespinner);
	gtk_widget_destroy(rmlogical->availlabel);
	gtk_widget_destroy(rmlogical->warningimage);
	gtk_widget_destroy(rmlogical->warninglabel);
	gtk_widget_destroy(rmlogical->warningbox);
}

static void
logical_partitions_destroy_ui(LogicalPartition *startlogical)
{
	LogicalPartition *curlogical;
	LogicalPartition *logicaltofree;

	for (curlogical = startlogical; curlogical != NULL; ) {
		logical_partition_destroy_ui(curlogical);
		logicaltofree = curlogical;
		curlogical = curlogical->next;
		g_free(logicaltofree);
	}
}

static DiskBlockOrder *
logical_partition_remove(disk_parts_t *partitions,
	DiskBlockOrder *rmblkorder,
	gboolean ret_next_item)
{
	gint lidx = 0;
	partition_info_t *partinfo = NULL;
	DiskBlockOrder *retblkorder;
	gboolean moditem_found = FALSE;

	/* Firstly reset element in modifiedpartitions[activedisk] */
	/* Collapse logicals down to remove this single item */
	for (lidx = FD_NUMPART; lidx < OM_NUMPART; lidx++) {
		partinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, lidx);

		if (moditem_found == TRUE) {
			if (partinfo) {
				partitions->pinfo[lidx] =
				    partitions->pinfo[lidx+1];
				if (partitions->pinfo[lidx].partition_order > 0)
					partitions->pinfo[lidx].partition_order--;
			} else {
				break;
			}
		} else {
			if (partinfo && partinfo->partition_id ==
			    rmblkorder->partinfo.partition_id) {
				moditem_found = TRUE;
				partitions->pinfo[lidx] =
				    partitions->pinfo[lidx+1];
				if (partitions->pinfo[lidx].partition_order > 0)
					partitions->pinfo[lidx].partition_order--;
			} else if (!partinfo) {
				break;
			}
		}
	}

	/* Remove blkorder item for this logical */
	retblkorder = installationdisk_blkorder_remove(
	    FALSE,
	    &modifiedlogicalblkorder[activedisk],
	    rmblkorder,
	    ret_next_item);

	/* Return the next/prev blkorder, after this one was removed */
	return (retblkorder);
}

static void
logical_partition_remove_ui(disk_parts_t *partitions,
	partition_info_t *rmpartinfo,
	gint pidx, /* Primary partition display index 0-3 */
	gint lidx) /* Logical indx into startlogical of item to remove, 0-N */
{
	LogicalPartition *curlogical = NULL;
	LogicalPartition *prevlogical = NULL;
	gint idx = 0;

	/* Get matching logicalpart in this primary partitions */
	/* linked list of logical partitions */
	/* Destroy widgets, and remove from linked list of logicalparts */

	/* Relocate existing logical partitions greater */
	/* than one being removed */
	relocate_extended_widgets(pidx, lidx, -1);

	/* Get logicalpart starting point */
	curlogical = MainWindow.InstallationDiskWindow.startlogical[pidx];
	g_return_if_fail(curlogical != NULL);

	/* Cycle through until we get to matching index then free this item */
	for (; curlogical != NULL; curlogical = curlogical->next, idx++) {
		if (idx == lidx) {
			if (idx == 0) {
				/* At start of the list */
				MainWindow.InstallationDiskWindow.startlogical[pidx] =
				    curlogical->next;
			} else {
				prevlogical->next = curlogical->next;
			}
			logical_partition_destroy_ui(curlogical);
			g_free(curlogical);
			break;
		}
		prevlogical = curlogical;
	}

	/* Need to reset logpartindex's in items after the one that was */
	/* removed */
	if (idx <= 0) {
		curlogical =
		    MainWindow.InstallationDiskWindow.startlogical[pidx];
	} else {
		curlogical = prevlogical->next;
	}
	for (; curlogical != NULL; curlogical = curlogical->next) {
		curlogical->logpartindex--;
	}

	/* Relocate primary partitions if necessary */
	relocate_partition_widgets(pidx, -1);
	relocate_static_widgets(
	    MainWindow.InstallationDiskWindow.fdisktablerows-1);

	/* Resize Fdisk table by -1 */
	resize_fdisk_table(-1);

	/* Reduce numpartlogical, and possible reset startlogical */
	MainWindow.InstallationDiskWindow.numpartlogical[pidx] -= 1;
	if (MainWindow.InstallationDiskWindow.numpartlogical[pidx]  == 0) {
		MainWindow.InstallationDiskWindow.startlogical[pidx] = NULL;
	}
}

static void
logical_partitions_remove_all()
{
	DiskBlockOrder *curblkorder = NULL;
	DiskBlockOrder *tmpblkorder = NULL;
	gint partindex;
	partition_info_t *partinfo;
	disk_parts_t *partitions;

	/* Remove logical elements from blkorder linked list */
	installationdisk_blkorder_free_list(
	    modifiedlogicalblkorder[activedisk]);
	modifiedlogicalblkorder[activedisk] = NULL;

	/*
	 * Process modifiedpartions elements for all logicals and
	 * reset them back default values
	 */
	partitions = modifiedpartitions[activedisk];
	for (partindex = FD_NUMPART; partindex < OM_NUMPART; partindex++) {
		partinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, partindex);

		if (partinfo) {
			/* Set this partition_info to 0's and defaults */
			orchestrator_om_set_partition_info(
			    partinfo, 0, 0, 0, 0);
		} else {
			break;
		}
	}
}

static void
logical_partitions_remove_all_ui(guint partindex)
{
	/* Destroy all logical widgets and relocate other widges up */
	logical_partitions_destroy_ui(
	    MainWindow.InstallationDiskWindow.startlogical[partindex]);

	/* relocate_primary_partitions */
	relocate_partition_widgets(partindex,
	    MainWindow.InstallationDiskWindow.numpartlogical[partindex]*-1);
	relocate_static_widgets(
	    MainWindow.InstallationDiskWindow.fdisktablerows-1);

	/* Reduce fdisk table by number of logical disks */
	resize_fdisk_table(
	    MainWindow.InstallationDiskWindow.numpartlogical[partindex]*-1);

	/* Initialize startlogical, and numpartlogical */
	MainWindow.InstallationDiskWindow.startlogical[partindex] = NULL;
	MainWindow.InstallationDiskWindow.numpartlogical[partindex] = 0;
}

static void
revert_partcombo_value(guint partindex)
{
	disk_partitioning_block_combox_handler(partindex);
	gtk_combo_box_set_active(GTK_COMBO_BOX(
	    MainWindow.InstallationDiskWindow.partcombo[partindex]),
	    MainWindow.InstallationDiskWindow.partcombosaved[partindex]);
	disk_partitioning_unblock_combox_handler(partindex);
}

static void
logicalpart_append(LogicalPartition *startlogical,
	LogicalPartition *newlogical)
{
	LogicalPartition *curlogical;

	for (curlogical = startlogical;
	    curlogical != NULL;
	    curlogical = curlogical->next) {
		if (curlogical->next == NULL) {
			curlogical->next = newlogical;
			break;
		}
	}
}

static void
logicalpart_insert_after(LogicalPartition *startlogical,
	gint lidx,			/* Index into pinfo[] 4->35 */
	LogicalPartition *newlogical)
{
	LogicalPartition *curlogical;
	gint logindex = FD_NUMPART-1;
	gboolean incrementing = FALSE;

	for (curlogical = startlogical;
	    curlogical != NULL;
	    curlogical = curlogical->next) {
		logindex++;
		if (logindex == lidx) {
			newlogical->next = curlogical->next;
			curlogical->next = newlogical;
			newlogical->logpartindex = curlogical->logpartindex+1;
		} else if (curlogical == newlogical) {
			incrementing = TRUE;
		} else if (incrementing == TRUE) {
			curlogical->logpartindex++;
		}
	}
}

/* Paramaters */
/* lidx : Index into pinfo[] 4->35 */
/* blkorder : If blkorder item is displayed or not */
static partition_info_t *
create_logical_partition(gint lidx,
	gboolean displayed)
{
	partition_info_t *partinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	disk_parts_t *partitions;
	DiskBlockOrder *newblkorder;
	DiskBlockOrder *curblkorder;
	uint8_t partition_order = 0;
	gint tidx = 0;
	gboolean inserting = FALSE;

	partitions = modifiedpartitions[activedisk];

	/* If lidx is negative, get the last logical as we are appending */
	/* to end of logicals, lidx is index into pinfo e.g. 4->35 */
	if (lidx == -1) {
		/*
		 * cycle through all logical partinfos, getting last index
		 * Add 1 to get next unused order item, this will return
		 * FD_NUMPART-1 if none exist.
		 */
		lidx = orchestrator_om_get_last_logical_index(partitions);
	}

	logpartinfo = orchestrator_om_get_part_by_blkorder(partitions, lidx);
	if (logpartinfo != NULL) {
		/*
		 * Item already exists at this order point in array, therefore
		 * we are inserting into the array after this point, cycle
		 * through remainer items in modifiedpartitions shifting them
		 * down one place and popping a new item in.
		 */
		inserting = TRUE;

		tidx = orchestrator_om_get_last_logical_index(partitions)+1;
		for (; tidx > lidx+1; tidx--) {
			partitions->pinfo[tidx] = partitions->pinfo[tidx-1];
			partitions->pinfo[tidx].partition_order++;
		}
		orchestrator_om_set_partition_info(&partitions->pinfo[lidx+1],
		    0, 0, 0, 0);

		/* set modifiedpartitiond[] logical item to next unused pid */
		partinfo = orchestrator_om_find_unused_logical_partition(
		    partitions,
		    UNUSED, lidx+1);
	} else {
		/* set modifiedpartitiond[] logical item to next unused pid */
		partinfo = orchestrator_om_find_unused_logical_partition(
		    partitions,
		    UNUSED, lidx);
	}

	if (partinfo) {
		/*
		 * Ensure we have an entry in modifiedblkorder[activedisk]
		 */
		newblkorder = g_new0(DiskBlockOrder, 1);
		newblkorder->displayed = displayed;
		newblkorder->next = NULL;
		(void) memcpy(&newblkorder->partinfo, partinfo,
		    sizeof (partition_info_t));

		if (modifiedlogicalblkorder[activedisk] == NULL) {
			modifiedlogicalblkorder[activedisk] = newblkorder;
			curblkorder = modifiedlogicalblkorder[activedisk];
		} else {
			if (inserting == FALSE) {
				/* Add new logical to end of blkorder list */
				curblkorder = installationdisk_blkorder_getlast(
				    modifiedlogicalblkorder[activedisk]);
				installationdisk_blkorder_insert_after(
				    modifiedlogicalblkorder[activedisk],
				    curblkorder, newblkorder, FALSE);
				curblkorder = newblkorder;
			} else {
				/* Insering new item into curblkorder list */
				curblkorder =
				    installationdisk_blkorder_get_by_partition_order(
				    modifiedlogicalblkorder[activedisk],
				    lidx+1);
				g_assert(curblkorder);
				installationdisk_blkorder_insert_after(
				    modifiedlogicalblkorder[activedisk],
				    curblkorder, newblkorder, TRUE);
				curblkorder = newblkorder;
			}
		}
		return (partinfo);
	} else {
		return (NULL);
	}
}

static LogicalPartition *
create_logical_partition_ui(gint pidx,	/* Primary pinfo[] index 0-3 */
	gint lidx,	/* Logical pinfo[] index 4->35 */
	gboolean appendatend)
{
	LogicalPartition *newlogicalpart = NULL;
	guint top_attach = 0;
	guint bottom_attach = 0;

	if (MainWindow.InstallationDiskWindow.startlogical[pidx] == NULL) {
		MainWindow.InstallationDiskWindow.numpartlogical[pidx] = 1;
	} else {
		MainWindow.InstallationDiskWindow.numpartlogical[pidx] += 1;
	}

	/* Add extra row */
	resize_fdisk_table(1);

	/* Relocate static widgets */
	relocate_static_widgets(
	    MainWindow.InstallationDiskWindow.fdisktablerows);

	/* Push all other partitions down one row in table */
	relocate_partition_widgets(pidx, 1);

	if (appendatend == TRUE) {
		top_attach =
		    MainWindow.InstallationDiskWindow.partrow[pidx] +
		    MainWindow.InstallationDiskWindow.numpartlogical[pidx];
		bottom_attach =
		    MainWindow.InstallationDiskWindow.partrow[pidx] +
		    MainWindow.InstallationDiskWindow.numpartlogical[pidx] + 1;
	} else {
		/* Relocate all widgets down one that exist after lidx */
		relocate_extended_widgets(pidx, (lidx-FD_NUMPART), 1);
		/* Top attach = primaryrow + (startlogical+1) + 1 */
		top_attach = MainWindow.InstallationDiskWindow.partrow[pidx] +
		    (lidx - FD_NUMPART + 1) + 1;
		bottom_attach = top_attach + 1;
	}

	newlogicalpart = g_new0(LogicalPartition, 1);
	newlogicalpart->next = NULL;

	/* Add new LogicalPartition element to linked list */
	if (MainWindow.InstallationDiskWindow.startlogical[pidx] == NULL) {
		/* Create first logical partition */
		MainWindow.InstallationDiskWindow.startlogical[pidx] =
		    newlogicalpart;
	} else {
		/* Create next one in line */
		if (appendatend == TRUE) {
			logicalpart_append(
			    MainWindow.InstallationDiskWindow.startlogical[pidx],
			    newlogicalpart);
		} else {
			if (lidx == -1) {
				/* Insert at start of list */
				newlogicalpart->next =
				    MainWindow.InstallationDiskWindow.startlogical[pidx];
				MainWindow.InstallationDiskWindow.startlogical[pidx] =
				    newlogicalpart;
				lidx = FD_NUMPART;
			} else {
				/* insert after lidx item */
				logicalpart_insert_after(
				    MainWindow.InstallationDiskWindow.startlogical[pidx],
				    lidx, newlogicalpart);
			}
		}
	}

	/* Create the new indented logical partition */
	logical_partition_init(pidx, newlogicalpart, top_attach, bottom_attach);

	return (newlogicalpart);
}

static void
primary_update_avail_space(disk_parts_t *partitions)
{
	partition_info_t *partinfo = NULL;
	gint pidx = 0;
	GtkLabel *availlabel = NULL;
	GtkSpinButton *spinner = NULL;
	gint parttype;
	gfloat avail_size = 0;

	/*
	 * Cycle through each defined primary partition, and update the
	 * Avail space column based the rest of the items
	 */
	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		partinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, pidx);
		g_assert(partinfo != NULL);

		spinner = GTK_SPIN_BUTTON
		    (MainWindow.InstallationDiskWindow.partspin[pidx]);
		availlabel = GTK_LABEL
		    (MainWindow.InstallationDiskWindow.partavail[pidx]);
		parttype = orchestrator_om_get_partition_type(partinfo);

		avail_size = calculate_avail_space(
		    modifiedprimaryblkorder[activedisk],
		    -1, partinfo);

		if (IS_EXT_PAR(parttype)) {
			/*
			 * For extended partitions the lowest value we can
			 * spin down to is the lowest size of it's containing
			 * logicals minus any unused space at the end.
			 */
			set_range_avail_from_value(spinner, availlabel,
			    get_extended_partition_min_size(partitions),
			    avail_size);
		} else {
			set_range_avail_from_value(spinner, availlabel,
			    parttype == UNUSED ? 0 : 0.1,
			    avail_size);
		}
	}
}

static void
logical_update_avail_space(disk_parts_t *partitions)
{
	partition_info_t *partinfo = NULL;
	partition_info_t *primpartinfo = NULL;
	gint pidx = 0;
	gint lidx = 0;
	GtkLabel *availlabel = NULL;
	GtkSpinButton *spinner = NULL;
	gint parttype;
	LogicalPartition *logicalpart;
	gfloat avail_size = 0;

	/* Get primary extended startlogical */
	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		if (MainWindow.InstallationDiskWindow.startlogical[pidx] != NULL)
			break;
	}

	if (pidx == FD_NUMPART) {
		g_warning("Logical partition starting element not found\n");
		return;
	}

	/*
	 * Cycle through each defined primary partition, find the appropriate
	 * Extended partition, and for each of it's logical children update
	 * their avail space
	 */
	for (lidx = FD_NUMPART; lidx < OM_NUMPART; lidx++) {
		partinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, lidx);

		if (partinfo) {
			parttype = orchestrator_om_get_partition_type(partinfo);
			logicalpart = get_logical_partition_at_pos(
			    (lidx+1)-FD_NUMPART,
			    MainWindow.InstallationDiskWindow.startlogical[pidx]);
			spinner = GTK_SPIN_BUTTON(logicalpart->sizespinner);
			availlabel = GTK_LABEL(logicalpart->availlabel);

			/*
			 * Get free space either side of this logical partition
			 * For logical partitions this is only pertinent for
			 * Solaris partitions
			 */
			avail_size = calculate_avail_space(
			    modifiedlogicalblkorder[activedisk],
			    -1, partinfo);
			set_range_avail_from_value(spinner, availlabel,
			    parttype == UNUSED ? 0 : 0.1,
			    avail_size);
		} else {
			break;
		}
	}

	/* We need to set the spinner range for the parent extended partition */
	primpartinfo = orchestrator_om_get_part_by_blkorder(partitions, pidx);

	if (primpartinfo != NULL) {
		gtk_spin_button_set_range(GTK_SPIN_BUTTON(
		    MainWindow.InstallationDiskWindow.partspin[pidx]),
		    get_extended_partition_min_size(partitions),
		    ONE_DECIMAL(calculate_avail_space(
		    modifiedprimaryblkorder[activedisk], -1, primpartinfo)));
	} else {
		g_warning(
		    "logical_update_avail_space() : Failed to get extended %d",
		    pidx);
	}
}

static void
logical_partition_combo_changed(GtkWidget *widget,
	gpointer user_data)
{
#if defined(__i386)
	LogicalPartition *logicalpart = (LogicalPartition *)user_data;
	GtkSpinButton *spinner = NULL;
	GtkComboBox *combo = GTK_COMBO_BOX(widget);
	GtkLabel *avail = NULL;
	gpointer objectdata = NULL;
	int index;
	gchar *errorprimarytext = NULL;
	gchar *errorsecondarytext = NULL;
	guint i = 0;
	partition_info_t *partinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	disk_parts_t *partitions;
	gfloat avail_size = 0;

	/* Block all other handlers why we set a few things */
	disk_partitioning_block_all_handlers();

	spinner = GTK_SPIN_BUTTON(logicalpart->sizespinner);
	avail = GTK_LABEL(logicalpart->availlabel);

	partitions = modifiedpartitions[activedisk];

	/* Get partition_info_t for this logical partition */
	partinfo = orchestrator_om_get_part_by_blkorder(partitions,
	    logicalpart->logpartindex);

	index = gtk_combo_box_get_active(combo);

	if (index == UNUSED_PARTITION) { /* Denotes Unused partition */
		gtk_spin_button_set_range(spinner, 0, 0);
		gtk_widget_set_sensitive(GTK_WIDGET(spinner), FALSE);

		/* Avail size should be set to just this partitions size */
		set_size_widgets_from_value(spinner, avail,
		    orchestrator_om_round_mbtogb(partinfo->partition_size));
	} else {
		/*
		 * If Changing from Unused we need re-calculate the AVAIL space
		 * And set spin button ranges and value
		 */
		if (logicalpart->partcombosaved == UNUSED) {
			avail_size = calculate_avail_space(
			    modifiedlogicalblkorder[activedisk],
			    logicalpart->logpartindex, partinfo);
			set_range_avail_from_value(spinner, avail, 0.1, avail_size);
			set_size_widgets_from_value(spinner, NULL,
			    orchestrator_om_get_partition_sizegb(partinfo));
		}
		gtk_widget_set_sensitive(GTK_WIDGET(spinner), TRUE);
	}

	logicalpart->partcombosaved = index;

	if (activediskisreadable == TRUE) {
		logicalpart->typechange = TRUE;
		logicalpart->sizechange = TRUE;
		update_data_loss_warnings();
	}

	update_disk_partitions_from_ui(partitions);
	logical_update_avail_space(partitions);

	g_object_set_data(G_OBJECT(diskbuttons[activedisk]),
	    "modified",
	    GINT_TO_POINTER(TRUE));
	gtk_widget_set_sensitive(GTK_WIDGET(
	    MainWindow.InstallationDiskWindow.resetbutton),
	    TRUE);

	/* Unblock all the handlers */
	disk_partitioning_unblock_all_handlers();

	g_debug("Logical Partition Combo Changed");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);
	print_gui(MainWindow.InstallationDiskWindow);
#endif
}

static void
primary_update_combo_sensitivity(disk_parts_t *partitions)
{
	partition_info_t *partinfo = NULL;
	gint parttype = 0;
	gint pidx = 0;
	GtkSpinButton *primspinner = NULL;
	GtkComboBox *primcombo = NULL;
	GtkLabel *primavail = NULL;
	gfloat spinval = 0;
	gdouble avail_size = 0;
	gint comboidx = 0;

	/*
	 * Scan through all 4 primary partitions
	 * If this partition is unused and there is no available
	 * space, don't allow the user to change the partition type.
	 */
	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		primcombo = GTK_COMBO_BOX
		    (MainWindow.InstallationDiskWindow.partcombo[pidx]);
		primspinner = GTK_SPIN_BUTTON
		    (MainWindow.InstallationDiskWindow.partspin[pidx]);
		primavail = GTK_LABEL
		    (MainWindow.InstallationDiskWindow.partavail[pidx]);

		spinval = (gfloat)gtk_spin_button_get_value(primspinner);
		avail_size = strtod(gtk_label_get_text(primavail), NULL);
		comboidx = gtk_combo_box_get_active(primcombo);

		if (comboidx == UNUSED_PARTITION) {
			if (avail_size > 0) {
				gtk_widget_set_sensitive(
				    GTK_WIDGET(primcombo), TRUE);
			} else {
				gtk_widget_set_sensitive(
				    GTK_WIDGET(primcombo), FALSE);
			}
		}
	}
}

static void
primary_partition_combo_changed(GtkWidget *widget,
    gint partindex,
    gpointer user_data)
{
#if defined(__i386)
	GtkSpinButton *spinner = NULL;
	GtkComboBox *combo = GTK_COMBO_BOX(widget);
	GtkLabel *avail = NULL;
	gpointer objectdata = NULL;
	int index;
	gchar *errorprimarytext = NULL;
	gchar *errorsecondarytext = NULL;
	guint i = 0;
	LogicalPartition *logicalpart = NULL;
	partition_info_t *partinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	disk_parts_t *partitions;
	gfloat partsize = 0;
	gfloat avail_size = 0;

	/* Block all other handlers why we set a few things */
	disk_partitioning_block_all_handlers();

	spinner = GTK_SPIN_BUTTON(
	    MainWindow.InstallationDiskWindow.partspin[partindex]);
	avail = GTK_LABEL(
	    MainWindow.InstallationDiskWindow.partavail[partindex]);
	partitions = modifiedpartitions[activedisk];
	partinfo = orchestrator_om_get_part_by_blkorder(partitions, partindex);

	index = gtk_combo_box_get_active(combo);
	if (index == UNUSED_PARTITION) { /* Denotes Unused partition */
		/* If previous logicals exist, remove and collapse down */
		if (MainWindow.InstallationDiskWindow.startlogical[partindex]
		    != NULL) {
			logical_partitions_remove_all_ui(partindex);
			logical_partitions_remove_all(partindex);
		}

		gtk_spin_button_set_range(spinner, 0, 0);
		gtk_widget_set_sensitive(GTK_WIDGET(spinner), FALSE);

		/* Avail size should be set to just this partitions size */
		avail_size = calculate_avail_space(
		    modifiedprimaryblkorder[activedisk],
		    partindex, partinfo);

		set_size_widgets_from_value(spinner, avail, avail_size);

		/* Partition size gets nuked so set the flag */
		if (activediskisreadable)
			MainWindow.InstallationDiskWindow.partsizechanges[partindex]
			    = TRUE;
	} else if (index == EXTENDED_PARTITION) {
		/* Determine if extended partition already exists */
		for (i = 0; i < FD_NUMPART; i++) {
			if (MainWindow.InstallationDiskWindow.startlogical[i]
			    != NULL &&
			    i != partindex) {
				/*
				 * Extended partition already defined, can only
				 * have one Warn user that they need to change
				 * other one first.
				 */
				errorprimarytext =
				    g_strdup(_("Only one extended partition can exist."));
				errorsecondarytext =
				    g_strdup(_("Choose another type."));
				gui_install_prompt_dialog(FALSE, FALSE, FALSE,
				    GTK_MESSAGE_ERROR,
				    errorprimarytext,
				    errorsecondarytext);
				g_free(errorprimarytext);
				g_free(errorsecondarytext);
				revert_partcombo_value(partindex);
				disk_partitioning_unblock_all_handlers();
				return;
			}
		}

		/*
		 * If Changing from Unused we need re-calculate the AVAIL space
		 * And set spin button ranges and value
		 */
		if (MainWindow.InstallationDiskWindow.partcombosaved[partindex] ==
		    UNUSED) {
			partsize =
			    orchestrator_om_get_partition_sizegb(partinfo);
			if (partsize == 0.0) {
				/*
				 * This partition is going to be assigned 0.1 in
				 * size so we need to remove 0.1 from the first
				 * unused item either below or above this item.
				 * There will always be unused space somewhere
				 * at this point otherwise the combobox would
				 * not have been active.
				 */
				update_primary_unused_partition_size_from_ui(
				    partitions, partindex, 0.1);
			}
			avail_size = calculate_avail_space(
			    modifiedprimaryblkorder[activedisk],
			    partindex, partinfo);
			set_range_avail_from_value(spinner, avail, 0.1, avail_size);
			set_size_widgets_from_value(spinner, NULL,
			    orchestrator_om_get_partition_sizegb(partinfo));
		} else {
			set_size_widgets_from_value(spinner, NULL,
			    orchestrator_om_get_partition_sizegb(partinfo));
		}
		gtk_widget_set_sensitive(GTK_WIDGET(spinner), TRUE);

		if (MainWindow.InstallationDiskWindow.startlogical[partindex]
		    == NULL) {
			/*
			 * Physically create first logical partition within
			 * modifiedpartitions and modifiedlogicalblkorder
			 */
			logpartinfo =
			    create_logical_partition(FD_NUMPART, FALSE);
			logpartinfo->partition_size =
			    partinfo->partition_size;
			logpartinfo->partition_offset =
			    partinfo->partition_offset;

			update_blkorder_from_partinfo(
			    modifiedlogicalblkorder[activedisk],
			    logpartinfo);

			/*
			 * This should never happen, as there should be
			 * no logicals in Existence at this point, this
			 * should be the only one !!
			 */
			g_assert(logpartinfo);

			/* Create new logical partition */
			logicalpart = create_logical_partition_ui(partindex,
			    FD_NUMPART, TRUE);
			logicalpart->logpartindex = FD_NUMPART;
			set_size_widgets_from_value(
			    GTK_SPIN_BUTTON(logicalpart->sizespinner),
			    GTK_LABEL(logicalpart->availlabel),
			    orchestrator_om_get_partition_sizegb(partinfo));
			logicalpart->typechange = TRUE;
			logicalpart->partcombosaved = UNUSED_PARTITION;
		}

		/* Partition will get nuked so set the flag */
		if (activediskisreadable)
			MainWindow.InstallationDiskWindow.partsizechanges[partindex]
			    = TRUE;
	} else {

		/* If previous logicals exist, remove and collapse down */
		if (MainWindow.InstallationDiskWindow.startlogical[partindex]
		    != NULL) {
			logical_partitions_remove_all_ui(partindex);
			logical_partitions_remove_all(partindex);
		}

		/*
		 * If Changing from Unused we need re-calculate the AVAIL space
		 * And set spin button ranges and value
		 */
		if (MainWindow.InstallationDiskWindow.partcombosaved[partindex] ==
		    UNUSED) {
			partsize =
			    orchestrator_om_get_partition_sizegb(partinfo);
			if (partsize == 0.0) {
				/*
				 * This partition is going to be assigned 0.1
				 * in size so we need to remove 0.1 from the
				 * first unused item either below or above
				 * this item. There will always be unused
				 * space somewhere at this point otherwise
				 * the combobox would not have been active.
				 */
				update_primary_unused_partition_size_from_ui(
				    partitions, partindex, 0.1);
			}

			avail_size = calculate_avail_space(
			    modifiedprimaryblkorder[activedisk],
			    partindex, partinfo);

			set_range_avail_from_value(spinner, avail, 0.1, avail_size);
			set_size_widgets_from_value(spinner, NULL,
			    orchestrator_om_get_partition_sizegb(partinfo));
		}
		gtk_widget_set_sensitive(GTK_WIDGET(spinner), TRUE);

		/* Partition size also changes from 0 to 1gb so set the flag */
		if (activediskisreadable)
			MainWindow.InstallationDiskWindow.partsizechanges[partindex]
			    = TRUE;
	}

	MainWindow.InstallationDiskWindow.partcombosaved[partindex] = index;

	objectdata = g_object_get_data(G_OBJECT(combo), "extra_fs");
	if ((objectdata != NULL) && (GPOINTER_TO_INT(objectdata) == TRUE)) {
		gtk_combo_box_remove_text(combo,
		    NUM_DEFAULT_PARTITIONS);
		g_object_set_data(G_OBJECT(combo),
		    "extra_fs",
		    GINT_TO_POINTER(FALSE));
	}

	if (activediskisreadable == TRUE) {
		MainWindow.InstallationDiskWindow.parttypechanges[partindex]
		    = TRUE;
		MainWindow.InstallationDiskWindow.initialsizechange[partindex]
		    = FALSE;
		update_data_loss_warnings();
	}

	update_disk_partitions_from_ui(partitions);
	primary_update_avail_space(partitions);
	primary_update_combo_sensitivity(partitions);

	g_object_set_data(G_OBJECT(diskbuttons[activedisk]),
	    "modified",
	    GINT_TO_POINTER(TRUE));
	gtk_widget_set_sensitive(GTK_WIDGET(
	    MainWindow.InstallationDiskWindow.resetbutton),
	    TRUE);

	/* Unblock all the handlers, including any new logicals created */
	disk_partitioning_unblock_all_handlers();

	g_debug("Primary Partition Combo Changed");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);
	print_gui(MainWindow.InstallationDiskWindow);
#endif
}

static gboolean
disk_is_too_big(disk_info_t *diskinfo)
{
	return orchestrator_om_get_total_disk_sizemb(diskinfo) >
	    orchestrator_om_get_disk_sizemb(diskinfo) ? TRUE : FALSE;
}

void
partition_0_combo_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_combo_changed(widget, 0, user_data);
}

void
partition_1_combo_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_combo_changed(widget, 1, user_data);
}

void
partition_2_combo_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_combo_changed(widget, 2, user_data);
}

void
partition_3_combo_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_combo_changed(widget, 3, user_data);
}

static void
turn_on_partsizechanges(guint index)
{
	if (activediskisreadable == TRUE) {
		MainWindow.InstallationDiskWindow.partsizechanges[index] = TRUE;
	}
}
static void
update_extended_partition(disk_parts_t *partitions,
	gint pidx,
	gfloat diffgb)
{
	partition_info_t *modpartinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	LogicalPartition *logicalpart = NULL;
	gint parttype = 0;
	DiskBlockOrder *curblkorder = NULL;
	guint lidx = 0;

	/* if changing the size of an extended partition, then all it's */
	/* existing partitions are essentially being destroyed, so collapse */
	/* any logicals, reseting the modifiedlogicalblkorder list aswell */
	/* If an extended partitions size has already been modified, then add */
	/* Free space to unused item at end list of logicals, creating one if */
	/* necessary */

	modpartinfo =
	    orchestrator_om_get_part_by_blkorder(partitions, pidx);
	g_assert(modpartinfo != NULL);

	parttype = orchestrator_om_get_partition_type(modpartinfo);

	/* Only interested if this partition is an extended partition */
	if (IS_EXT_PAR(parttype)) {
		if (MainWindow.InstallationDiskWindow.initialsizechange[pidx]
		    == TRUE) {
			/* Regardless of whether size increased or decreased */
			/* First time resetting size */
			MainWindow.InstallationDiskWindow.initialsizechange[pidx]
			    = FALSE;

			/* Collapse all existing logicals and destroy widgets */
			logical_partitions_remove_all_ui(pidx);

			/* Clean out modifiedlogicalblkorder */
			/* Reset all partitions[FD_NUMPART+] */
			logical_partitions_remove_all(pidx);

			/* Add one new logical back to modifiedpartitions and */
			/* to modifiedlogicalblkorder aswell */
			logpartinfo =
			    create_logical_partition(FD_NUMPART, FALSE);
			logpartinfo->partition_size =
			    modpartinfo->partition_size;
			logpartinfo->partition_offset =
			    modpartinfo->partition_offset;
			update_blkorder_from_partinfo(
			    modifiedlogicalblkorder[activedisk],
			    logpartinfo);

			/*
			 * This should never happen, as there should be
			 * no logicals in Existence at this point, this
			 * should be the only one !!
			 */
			g_assert(logpartinfo);

			/* Create one unused logical partition ui component */
			logicalpart = create_logical_partition_ui(pidx,
			    FD_NUMPART, TRUE);
			logicalpart->logpartindex = FD_NUMPART;
			logicalpart->sizechange = TRUE;
			logicalpart->typechange = TRUE;
			set_size_widgets_from_value(
			    GTK_SPIN_BUTTON(logicalpart->sizespinner),
			    GTK_LABEL(logicalpart->availlabel),
			    orchestrator_om_get_partition_sizegb(logpartinfo));
			logicalpart->typechange = TRUE;
			logicalpart->partcombosaved = UNUSED_PARTITION;

			update_data_loss_warnings();
		} else if (diffgb != 0) {
			/*
			 * Size has changed before, so any logicals
			 * currently defined are newly defined user ones,
			 * simply add new unused space to end of logicals
			 * Or remove from unused space currently there If
			 * no unused space left.
			 */

			if (diffgb > 0) {
				/*
				 * Primary Extended increased in size, so
				 * add this difference To Last unused item
				 */
				curblkorder = installationdisk_blkorder_getlast(
				    modifiedlogicalblkorder[activedisk]);

				parttype = orchestrator_om_get_partition_type(
				    &curblkorder->partinfo);
				if (parttype == UNUSED) {
					curblkorder->partinfo.partition_size +=
					    orchestrator_om_gbtomb(diffgb);
					if (update_partinfo_from_blkorder(
					    FALSE, curblkorder,
					    partitions) == FALSE) {
						g_warning("Failed updating last unused logical " \
						    "partinfo from blkorder after extended changed " \
						    "in size by : %f, blkorder partition_id is %d",
						    orchestrator_om_gbtomb(diffgb),
						    curblkorder->partinfo.partition_id);
						print_partinfos(activedisk,
						    alldiskinfo, modifiedpartitions);
						print_blkorder(alldiskinfo[activedisk],
						    modifiedprimaryblkorder[activedisk],
						    modifiedlogicalblkorder[activedisk]);
					}
				} else {
					/*
					 * Last item is not unused, so we need
					 * to add a new one And display it as
					 * unused
					 */
					logpartinfo =
					    create_logical_partition(-1, FALSE);
					logpartinfo->partition_size =
					    orchestrator_om_gbtomb(diffgb);
					update_blkorder_from_partinfo(
					    modifiedlogicalblkorder[activedisk],
					    logpartinfo);

					/* Create new logical widgets */
					logicalpart =
					    create_logical_partition_ui(pidx,
					    logpartinfo->partition_order-1,
					    TRUE);
					logicalpart->logpartindex =
					    logpartinfo->partition_order-1;

					set_size_widgets_from_value(
					    GTK_SPIN_BUTTON(logicalpart->sizespinner),
					    GTK_LABEL(logicalpart->availlabel),
					    orchestrator_om_get_partition_sizegb(logpartinfo));

					logicalpart->typechange = TRUE;
					logicalpart->partcombosaved =
					    UNUSED_PARTITION;
				}
			} else {
				/*
				 * diffgb < 0, therefore Primary Extended
				 * partition has been decreased in size.
				 * We need to reduce one or more of it's
				 * logical partitions. Starting at the
				 * last partition, starting reducing sizes.
				 * If last logical partition does not
				 * contain enough to be reduced, then it
				 * is dropped completely, and the next
				 * partition up is reduced by the remainder
				 * amount. This sequence continues until
				 * all space has been reduced.
				 */

				curblkorder = installationdisk_blkorder_getlast(
				    modifiedlogicalblkorder[activedisk]);
				for (; curblkorder != NULL; ) {

					if (diffgb == 0)
						break;

					parttype =
					    orchestrator_om_get_partition_type(
					    &curblkorder->partinfo);

					/*
					 * Regardless of partition type we must
					 * reduce it's size. As partition is
					 * reduced from the bottom up.
					 */
					if ((orchestrator_om_gbtomb(diffgb)*-1) <=
					    curblkorder->partinfo.partition_size) {
						/* Just need to reduce this single partitions size */
						curblkorder->partinfo.partition_size +=
						    orchestrator_om_gbtomb(diffgb);
						if (orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size) == 0) {
							curblkorder->partinfo.partition_size = 0;
						}
						diffgb = 0;
					} else {
						diffgb = diffgb + orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size);
						curblkorder->partinfo.partition_size = 0;
					}

					/* Update equivalent item in modifiedpartitions array */
					if (update_partinfo_from_blkorder(FALSE, curblkorder,
					    partitions) == FALSE) {
						g_warning("Failed updating logical partinfo " \
						    "from blkorder after extended changed " \
						    "in size by : %f, blkorder partition_id is %d",
						    orchestrator_om_gbtomb(diffgb),
						    curblkorder->partinfo.partition_id);
						print_partinfos(activedisk,
						    alldiskinfo, modifiedpartitions);
						print_blkorder(alldiskinfo[activedisk],
						    modifiedprimaryblkorder[activedisk],
						    modifiedlogicalblkorder[activedisk]);
					}

					/* If partition_Size is now zero, remove from lists */
					if (curblkorder->partinfo.partition_size == 0) {
						/* Get linked list item number */
						lidx = installationdisk_blkorder_get_index(
						    modifiedlogicalblkorder[activedisk], curblkorder);

						/* Remove this widget from display */
						logical_partition_remove_ui(partitions,
						    &curblkorder->partinfo, pidx, lidx);

						/* Remove from partitions */
						curblkorder = logical_partition_remove(partitions,
						    curblkorder, FALSE);
						continue;
					}
					curblkorder = installationdisk_blkorder_getprev(
					    modifiedlogicalblkorder[activedisk], curblkorder);
				}
			}
		}
		/* redisplay avail space for all logicals */
		logical_update_avail_space(partitions);
	}
}

static gint
get_primary_extended_index(disk_parts_t *partitions)
{
	partition_info_t *partinfo = NULL;
	gint parttype = 0;
	gint pidx = 0;
	gint retpidx = -1;

	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		partinfo = orchestrator_om_get_part_by_blkorder(partitions, pidx);

		if (partinfo) {
			parttype =
			    orchestrator_om_get_partition_type(partinfo);
			if (IS_EXT_PAR(parttype)) {
				retpidx = pidx;
				break;
			}
		}
	}

	return (retpidx);
}

static void
update_logical_unused_partition_size_from_ui(disk_parts_t *partitions,
	LogicalPartition *logicalpart,
	gfloat diffgb)
{
	partition_info_t *modpartinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	gint parttype;
	gboolean moditem_found = FALSE;
	DiskBlockOrder *curblkorder = NULL;
	LogicalPartition *newlogicalpart = NULL;
	gint lidx = 0;
	gint pidx = 0;
	gfloat avail_size = 0;

	/*
	 * Size spinner has either increased or decreased
	 * If eating into availspace from either below or above
	 * Need to reduce/increase the partition_size of appropriate elements
	 * above and below, doing this will allow for calculation of available
	 * space for other widgets.
	 * Only spinners of type Solaris or Extended can change their spinner.
	 */

	modpartinfo = orchestrator_om_get_part_by_blkorder(partitions,
	    logicalpart->logpartindex);
	g_assert(modpartinfo != NULL);

	/* Get primary index of extended partition */
	pidx = get_primary_extended_index(partitions);

	g_return_if_fail(diffgb != 0);

	/*
	 * Scan for unused items after changed partition, and reduce it's
	 * size first, if cannot reduce any more, then need to scan again
	 * for items above this that are unused and reduce them by left over
	 * amount
	 */
	/* First scan is for unused items after current modified item */
	moditem_found = FALSE;
	for (curblkorder = modifiedlogicalblkorder[activedisk];
	    curblkorder != NULL; TRUE) {
		if (curblkorder->partinfo.partition_id == modpartinfo->partition_id) {
			moditem_found = TRUE;
		} else if (moditem_found == TRUE) {
			parttype =
			    orchestrator_om_get_partition_type(&curblkorder->partinfo);
			if (parttype == UNUSED &&
			    ((curblkorder->partinfo.partition_size > 0 && diffgb > 0) ||
			    (diffgb < 0))) {
				/* We have an unused block of disc */
				if (diffgb > 0) {
					if (orchestrator_om_gbtomb(diffgb) <=
					    curblkorder->partinfo.partition_size) {
						curblkorder->partinfo.partition_size -=
						    orchestrator_om_gbtomb(diffgb);
						diffgb = 0;
						if (orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size) == 0) {
							curblkorder->partinfo.partition_size = 0;
						}
					} else {
						diffgb = diffgb - orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size);
						curblkorder->partinfo.partition_size = 0;
					}
				} else {
					curblkorder->partinfo.partition_size -=
					    orchestrator_om_gbtomb(diffgb);
					diffgb = 0;
				}

				if (update_partinfo_from_blkorder(FALSE, curblkorder,
				    partitions) == FALSE) {
					g_warning("Failed updating logical partinfo " \
					    "after current from blkorder after extended changed " \
					    "in size by : %f, blkorder partition_id is %d",
					    orchestrator_om_gbtomb(diffgb),
					    curblkorder->partinfo.partition_id);
					print_partinfos(activedisk,
					    alldiskinfo, modifiedpartitions);
					print_blkorder(alldiskinfo[activedisk],
					    modifiedprimaryblkorder[activedisk],
					    modifiedlogicalblkorder[activedisk]);
				}

				/* Unused item and ZERO so remove from blkorder */
				if (curblkorder->partinfo.partition_size == 0) {

					/* Get the linked list index number */
					lidx = installationdisk_blkorder_get_index(
					    modifiedlogicalblkorder[activedisk], curblkorder);

					/* Remove this widget from display */
					logical_partition_remove_ui(partitions,
					    &curblkorder->partinfo, pidx, lidx);

					/* Remove from partitions & blkorder */
					curblkorder = logical_partition_remove(partitions,
					    curblkorder, TRUE);
					continue;
				}
			} else {
				break;
			}

			if (diffgb == 0) {
				break;
			}
		}
		curblkorder = curblkorder->next;
	}

	if (diffgb < 0) {
		/* Space has been removed from current logical, and there is no */
		/* Unused just after it to add the unused space to, so we need to */
		/* create a new unused item after the current one, to contain this */
		/* unused space */
		lidx = FD_NUMPART-1;
		for (curblkorder = modifiedlogicalblkorder[activedisk];
		    curblkorder != NULL;
		    curblkorder = curblkorder->next) {
			lidx++;
			if (curblkorder->partinfo.partition_id ==
			    modpartinfo->partition_id) {

				/* Create a new logical partition at this location */
				logpartinfo = create_logical_partition(lidx, FALSE);
				logpartinfo->partition_size =
				    orchestrator_om_gbtomb(diffgb * -1);

				update_blkorder_from_partinfo(
				    modifiedlogicalblkorder[activedisk], logpartinfo);

				/*
				 * This will only ever happen if we have consumed all 32
				 * logicals... possible but highly unusual
				 */
				g_assert(logpartinfo);

				/* Create new logical partition */
				newlogicalpart = create_logical_partition_ui(
				    pidx, lidx, FALSE);
				newlogicalpart->logpartindex = lidx+1;

				set_size_widgets_from_value(
				    GTK_SPIN_BUTTON(newlogicalpart->sizespinner),
				    GTK_LABEL(newlogicalpart->availlabel),
				    orchestrator_om_get_partition_sizegb(logpartinfo));

				newlogicalpart->typechange = TRUE;
				newlogicalpart->partcombosaved = UNUSED_PARTITION;
				diffgb = 0;
				break;
			}
		}
	} else if (diffgb > 0) {
		/* No more free space avail after current item so must search */
		/* Before current item and reduce first available unused item */
		moditem_found = FALSE;
		curblkorder = installationdisk_blkorder_getlast(
		    modifiedlogicalblkorder[activedisk]);
		for (; curblkorder != NULL; ) {
			if (curblkorder->partinfo.partition_id ==
			    modpartinfo->partition_id) {
				moditem_found = TRUE;
			} else if (moditem_found == TRUE) {
				parttype =
				    orchestrator_om_get_partition_type(&curblkorder->partinfo);
				if (parttype == UNUSED &&
				    (curblkorder->partinfo.partition_size > 0 && diffgb > 0)) {
					if (diffgb > 0) {
						if (orchestrator_om_gbtomb(diffgb) <=
						    curblkorder->partinfo.partition_size) {
							curblkorder->partinfo.partition_size -=
							    orchestrator_om_gbtomb(diffgb);
							diffgb = 0;
							if (orchestrator_om_round_mbtogb(
							    curblkorder->partinfo.partition_size) == 0) {
								curblkorder->partinfo.partition_size = 0;
							}
						} else {
							diffgb = diffgb - orchestrator_om_round_mbtogb(
							    curblkorder->partinfo.partition_size);
							curblkorder->partinfo.partition_size = 0;
						}
					} else {
						/* See prev section as to why just setting to 0 */
						curblkorder->partinfo.partition_size -=
						    orchestrator_om_gbtomb(diffgb);
						diffgb = 0;
					}

					if (update_partinfo_from_blkorder(FALSE, curblkorder,
					    partitions) == FALSE) {
						g_warning("Failed updating logical partinfo before " \
						    "current from blkorder after extended changed " \
						    "in size by : %f, blkorder partition_id is %d",
						    orchestrator_om_gbtomb(diffgb),
						    curblkorder->partinfo.partition_id);
						print_partinfos(activedisk,
						    alldiskinfo, modifiedpartitions);
						print_blkorder(alldiskinfo[activedisk],
						    modifiedprimaryblkorder[activedisk],
						    modifiedlogicalblkorder[activedisk]);
					}

					/* Unused item and ZERO so remove from blkorder */
					/* and remove from display */
					if (curblkorder->partinfo.partition_size == 0) {
						/* Get the linked list index number */
						lidx = installationdisk_blkorder_get_index(
						    modifiedlogicalblkorder[activedisk], curblkorder);

						/* Remove this widget from display */
						logical_partition_remove_ui(partitions,
						    &curblkorder->partinfo, pidx, lidx);

						/* Remove from partitions & blkorder */
						curblkorder = logical_partition_remove(partitions,
						    curblkorder, FALSE);
						continue;
					}
				} else {
					break;
				}

				if (diffgb == 0) {
					break;
				}
			}
			curblkorder = installationdisk_blkorder_getprev(
			    modifiedlogicalblkorder[activedisk], curblkorder);
		}
	}

	if (diffgb != 0) {
		g_warning("Some unused space not reduced.\n");
	}
}

static void
update_primary_unused_partition_size_from_ui(disk_parts_t *partitions,
	gint pidx,
	gfloat diffgb)
{
	partition_info_t *modpartinfo = NULL;
	gint parttype;
	gboolean moditem_found = FALSE;
	DiskBlockOrder *curblkorder = NULL;
	DiskBlockOrder *modblkorder = NULL;

	/*
	 * Size spinner has either increased or decreased
	 * If eating into availspace from either below or above
	 * Need to reduce/increase the partition_size of appropriate elements
	 * above and below, doing this will allow for calculation of available
	 * space for other widgets.
	 * Only spinners of type Solaris or Extended can change their spinner.
	 */

	modpartinfo =
	    orchestrator_om_get_part_by_blkorder(partitions, pidx);
	g_assert(modpartinfo != NULL);

	g_return_if_fail(diffgb != 0);

	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);

	/* First verify that a blkorder item exists for this partition_id */
	modblkorder = installationdisk_blkorder_get_by_partition_id(
	    modifiedprimaryblkorder[activedisk],
	    modpartinfo->partition_id);

	if (modblkorder == NULL) {
		/*
		 * We need to add a new blkorder item for this partition
		 * Question is where exactly.
		 */
		/*
		 * Likely scenario in this case is where we've just
		 * made a unused primary solaris or extended.
		 * And there is no primary partition matching this one
		 * located in the curblkorder layout.
		 * We need to insert a new blkorder item to match newly
		 * displayed partinfo, we also need to reduce unused space
		 * from below first and then above both blkorder list
		 * and partinfo array.
		 */
		/* Add a new item to curblklayout */
		DiskBlockOrder *gap = NULL;

		gap = g_new0(DiskBlockOrder, 1);
		gap->displayed = TRUE;
		gap->next = NULL;

		orchestrator_om_set_partition_info(&gap->partinfo,
		    orchestrator_om_gbtomb(diffgb), 0, 0, 0);

		modpartinfo->partition_size = gap->partinfo.partition_size;
		gap->partinfo.partition_id = modpartinfo->partition_id;
		gap->partinfo.partition_order = modpartinfo->partition_order;

		installationdisk_blkorder_insert_displayed(
		    modifiedprimaryblkorder[activedisk], gap);
	}

	/*
	 * Scan for unused items after changed partition, and reduce it's
	 * size first, if cannot reduce any more, then need to scan again
	 * for items above this that are unused and reduce them by left over
	 * amount
	 */
	/* First scan is for unused items after current modified item */
	moditem_found = FALSE;
	for (curblkorder = modifiedprimaryblkorder[activedisk];
	    curblkorder != NULL; TRUE) {

		if (curblkorder->partinfo.partition_id == modpartinfo->partition_id) {
			moditem_found = TRUE;
		} else if (moditem_found == TRUE) {
			parttype =
			    orchestrator_om_get_partition_type(&curblkorder->partinfo);
			/*
			 * If partition_size > 0 and diffgb > 0, means we
			 * are consuming used space, and there is some in
			 * this unused partition to consume from.
			 * If diffgb < 0, we are returning some space to
			 * the unused pool, there fore we are not bothered
			 * what partition_size is, as we just add to it.
			 */
			if (parttype == UNUSED &&
			    ((curblkorder->partinfo.partition_size > 0 && diffgb > 0) ||
			    (diffgb < 0))) {
				/* We have an unused block of disc */
				if (diffgb > 0) {
					if (orchestrator_om_gbtomb(diffgb) <=
					    curblkorder->partinfo.partition_size) {
						curblkorder->partinfo.partition_size -=
						    orchestrator_om_gbtomb(diffgb);
						diffgb = 0;
						if (orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size) == 0) {
							curblkorder->partinfo.partition_size = 0;
						}
					} else {
						diffgb = diffgb - orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size);
						curblkorder->partinfo.partition_size = 0;
					}
				} else {
					curblkorder->partinfo.partition_size -=
					    orchestrator_om_gbtomb(diffgb);
					diffgb = 0;
				}

				if (curblkorder->partinfo.partition_id != 0) {
					/* Item displayed is one of 4 primaries */
					/* Update equivelent item in modifiedpartitions array */
					if (update_partinfo_from_blkorder(TRUE, curblkorder,
					    partitions) == FALSE) {
						g_warning("Failed updating primary partinfo before " \
						    "current from blkorder after primary changed " \
						    "in size by : %f, blkorder partition_id is %d",
						    orchestrator_om_gbtomb(diffgb),
						    curblkorder->partinfo.partition_id);
						print_partinfos(activedisk,
						    alldiskinfo, modifiedpartitions);
						print_blkorder(alldiskinfo[activedisk],
						    modifiedprimaryblkorder[activedisk],
						    modifiedlogicalblkorder[activedisk]);
					}
				} else {
					/* Unused item and not listed in primary, if size is */
					/* ZERO remove from curblkorder as no longer valid */
					if (curblkorder->partinfo.partition_size == 0) {
						curblkorder = installationdisk_blkorder_remove(
						    TRUE,
						    &modifiedprimaryblkorder[activedisk],
						    curblkorder, TRUE);
						continue;
					} else if (orchestrator_om_round_mbtogb(
					    curblkorder->partinfo.partition_size) > 0) {
						/*
						 * In this scenario, partition size is large
						 * enough to be displayed but is not currently
						 * Check if there is a non displayed primary
						 * where this gap could be displayed.
						 */
						update_partinfo_from_blkorder_and_display(
						    partitions,
						    modpartinfo,
						    curblkorder);
					}
				}
			} else if (parttype != UNUSED) {
				break;
			}

			if (diffgb == 0) {
				break;
			}
		}
		curblkorder = curblkorder->next;
	}

	if (diffgb < 0) {
		/* Remove space from current partition, and there is no unused */
		/* block to add this to, create one in the linked list just after */
		/* The current item */
		for (curblkorder = modifiedprimaryblkorder[activedisk];
		    curblkorder != NULL;
		    curblkorder = curblkorder->next) {
			if (curblkorder->partinfo.partition_id ==
			    modpartinfo->partition_id) {
				/* Add a new item to curblklayout */
				DiskBlockOrder *gap = NULL;

				gap = g_new0(DiskBlockOrder, 1);
				gap->displayed = FALSE;
				gap->next = NULL;

				orchestrator_om_set_partition_info(&gap->partinfo,
				    orchestrator_om_gbtomb(diffgb*-1), 0, 0, 0);
				installationdisk_blkorder_insert_after(
				    modifiedprimaryblkorder[activedisk],
				    curblkorder, gap, FALSE);

				/*
				 * If we are adding a new piece of unsued space into the
				 * blkorder structure, check if all primary partition
				 * elements if partinfo[s] is assigned. If some are not
				 * assigned, possible then assign one with this free space
				 */
				installationdisk_blkorder_empty_partinfo_sync(
				    modifiedpartitions[activedisk],
				    modifiedprimaryblkorder[activedisk],
				    curblkorder, gap);
				diffgb = 0;
				break;
			}
		}
	} else if (diffgb > 0) {
		/* No more free space avail after current item so must search */
		/* Before current item and reduce first available unused item */
		moditem_found = FALSE;
		curblkorder = installationdisk_blkorder_getlast(
		    modifiedprimaryblkorder[activedisk]);
		print_partinfo(pidx, modpartinfo, TRUE);
		for (; curblkorder != NULL; ) {
			print_partinfo(pidx, &curblkorder->partinfo, FALSE);
			if (curblkorder->partinfo.partition_id ==
			    modpartinfo->partition_id) {
				moditem_found = TRUE;
			} else if (moditem_found == TRUE) {
				parttype =
				    orchestrator_om_get_partition_type(&curblkorder->partinfo);
				if (parttype == UNUSED &&
				    (curblkorder->partinfo.partition_size > 0 && diffgb > 0)) {
					if (diffgb > 0) {
						if (orchestrator_om_gbtomb(diffgb) <=
						    curblkorder->partinfo.partition_size) {
							curblkorder->partinfo.partition_size -=
							    orchestrator_om_gbtomb(diffgb);
							diffgb = 0;
							if (orchestrator_om_round_mbtogb(
							    curblkorder->partinfo.partition_size) == 0) {
								curblkorder->partinfo.partition_size = 0;
							}
						} else {
							diffgb = diffgb - orchestrator_om_round_mbtogb(
							    curblkorder->partinfo.partition_size);
							curblkorder->partinfo.partition_size = 0;
						}
					} else {
						/* See prev section as to why just setting to 0 */
						curblkorder->partinfo.partition_size -=
						    orchestrator_om_gbtomb(diffgb);
						diffgb = 0;
					}
					if (curblkorder->partinfo.partition_id != 0) {
						/* Item displayed is one of 4 primaries */
						/* Update equivelent item in modifiedpartitions array */
						if (update_partinfo_from_blkorder(TRUE, curblkorder,
						    partitions) == FALSE) {
							g_warning("Failed updating primary partinfo " \
							    "after current from blkorder after primary " \
							    "changed in size by : %f, " \
							    "blkorder partition_id is %d",
							    orchestrator_om_gbtomb(diffgb),
							    curblkorder->partinfo.partition_id);
							print_partinfos(activedisk,
							    alldiskinfo, modifiedpartitions);
							print_blkorder(alldiskinfo[activedisk],
							    modifiedprimaryblkorder[activedisk],
							    modifiedlogicalblkorder[activedisk]);
						}
					} else {
						/* Unused item and not listed in primary, if size is */
						/* ZERO remove from curblkorder as no longer valid */
						if (curblkorder->partinfo.partition_size == 0) {
							curblkorder = installationdisk_blkorder_remove(
							    TRUE,
							    &modifiedprimaryblkorder[activedisk],
							    curblkorder, FALSE);
							continue;
						} else if (orchestrator_om_round_mbtogb(
						    curblkorder->partinfo.partition_size) > 0) {
							/*
							 * In this scenario, partition size is large
							 * enough to be displayed but is not currently
							 * Check if there is a non displayed primary
							 * where this gap could be displayed.
							 */
							update_partinfo_from_blkorder_and_display(
							    partitions,
							    modpartinfo,
							    curblkorder);
						}
					}
				} else if (parttype != UNUSED) {
					break;
				}

				if (diffgb == 0) {
					break;
				}
			}
			curblkorder = installationdisk_blkorder_getprev(
			    modifiedprimaryblkorder[activedisk], curblkorder);
		}
	}

	if (diffgb != 0) {
		g_warning("Some unused space not reduced : %f.\n", diffgb);
	}
}

static void
logical_partition_spinner_focus_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gboolean sensitivity,
	gpointer user_data)
{
	gint pidx = 0;

	if (sensitivity == FALSE)
		spinner_has_focus = TRUE;
	else
		spinner_has_focus = FALSE;

	/* Find starting point of logicalpartitions. */
	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		if (MainWindow.InstallationDiskWindow.startlogical[pidx] != NULL) {
			set_logical_combo_sensitivity(pidx, sensitivity, FALSE);
		}
	}
}

static gboolean
logical_partition_spinner_focus_in_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
#if defined(__i386)
	logical_partition_spinner_focus_handler(
	    widget, event, FALSE, user_data);
#endif
	return (FALSE);
}

static gboolean
logical_partition_spinner_focus_out_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
#if defined(__i386)
	logical_partition_spinner_focus_handler(
	    widget, event, TRUE, user_data);
#endif
	return (FALSE);
}

static void
logical_partition_spinner_value_changed(GtkWidget *widget,
	gpointer user_data)
{
#if defined(__i386)
	LogicalPartition *logicalpart = (LogicalPartition *)user_data;
	partition_info_t *modpartinfo = NULL;
	gfloat spinval = 0;
	gfloat partsize = 0;
	gfloat diffgb = 0;

	/* Block all other handlers why we set a few things */
	disk_partitioning_block_all_handlers();

	modpartinfo =
	    orchestrator_om_get_part_by_blkorder(modifiedpartitions[activedisk],
	    logicalpart->logpartindex);
	g_assert(modpartinfo != NULL);

	/* Get difference between actual size and spinner size */
	spinval = (gfloat)gtk_spin_button_get_value(
	    GTK_SPIN_BUTTON(logicalpart->sizespinner));
	partsize = orchestrator_om_round_mbtogb(modpartinfo->partition_size);

	/* Single click of spin button would leave this as -1 or +1 */
	/* but user can enter text manually so can be > 1 or < -1 */
	diffgb = spinval - partsize;

	logicalpart->sizechange = TRUE;
	update_data_loss_warnings();

	update_logical_unused_partition_size_from_ui(
	    modifiedpartitions[activedisk], logicalpart, diffgb);

	update_disk_partitions_from_ui(modifiedpartitions[activedisk]);
	logical_update_avail_space(modifiedpartitions[activedisk]);

	g_object_set_data(G_OBJECT(diskbuttons[activedisk]),
	    "modified",
	    GINT_TO_POINTER(TRUE));
	gtk_widget_set_sensitive(GTK_WIDGET(
	    MainWindow.InstallationDiskWindow.resetbutton),
	    TRUE);

	/* Block all other handlers why we set a few things */
	disk_partitioning_unblock_all_handlers();

	g_debug("Logical Partition Spinner Changed");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);
	print_gui(MainWindow.InstallationDiskWindow);
#endif
}

static void
primary_partition_spinner_value_changed(GtkWidget *widget,
	gint index,
	gpointer user_data)
{
#if defined(__i386)
	partition_info_t *modpartinfo = NULL;
	GtkSpinButton *spinner = NULL;
	gfloat spinval = 0;
	gfloat partsize = 0;
	gfloat diffgb = 0;

	/* Block all other handlers why we set a few things */
	disk_partitioning_block_all_handlers();

	modpartinfo = orchestrator_om_get_part_by_blkorder(
	    modifiedpartitions[activedisk],
	    index);
	g_assert(modpartinfo != NULL);

	/* Get difference between actual size and spinner size */
	spinner = GTK_SPIN_BUTTON(
	    MainWindow.InstallationDiskWindow.partspin[index]);
	spinval = (gfloat)gtk_spin_button_get_value(spinner);
	partsize = orchestrator_om_round_mbtogb(modpartinfo->partition_size);

	/* Single click of spin button would leave this as -1 or +1 */
	/* but user can enter text manually so can be > 1 or < -1 */
	diffgb = ONE_DECIMAL(spinval - partsize);

	turn_on_partsizechanges(index);
	update_data_loss_warnings();

	if (diffgb != 0) {
		update_primary_unused_partition_size_from_ui(
		    modifiedpartitions[activedisk], index, diffgb);
	}

	update_disk_partitions_from_ui(modifiedpartitions[activedisk]);
	primary_update_avail_space(modifiedpartitions[activedisk]);
	primary_update_combo_sensitivity(modifiedpartitions[activedisk]);

	if (diffgb != 0) {
		update_extended_partition(
		    modifiedpartitions[activedisk],
		    index, diffgb);
	}

	g_object_set_data(G_OBJECT(diskbuttons[activedisk]),
	    "modified",
	    GINT_TO_POINTER(TRUE));
	gtk_widget_set_sensitive(GTK_WIDGET(
	    MainWindow.InstallationDiskWindow.resetbutton),
	    TRUE);

	/* Block all other handlers why we set a few things */
	disk_partitioning_unblock_all_handlers();

	g_debug("Primary Partition Spinner Changed");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);
	print_gui(MainWindow.InstallationDiskWindow);
#endif
}

static void
set_logical_combo_sensitivity(gint pidx,
	gboolean sensitivity,
	gboolean set_all)
{
	LogicalPartition *curlogical;
	int comboindex = 0;

	for (curlogical = MainWindow.InstallationDiskWindow.startlogical[pidx];
	    curlogical != NULL;
	    curlogical = curlogical->next) {
		if (set_all == TRUE) {
			gtk_widget_set_sensitive(curlogical->typecombo, sensitivity);
		} else {
			/* Only set the unused items */
			comboindex = gtk_combo_box_get_active(
			    GTK_COMBO_BOX(curlogical->typecombo));
			if (comboindex == UNUSED_PARTITION) {
				gtk_widget_set_sensitive(curlogical->typecombo, sensitivity);
			}
		}
	}
}

static void
primary_partition_spinner_focus_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gint index,
	gboolean sensitivity,
	gpointer user_data)
{
#if defined(__i386)
	partition_info_t *modpartinfo;
	gint parttype;

	modpartinfo = orchestrator_om_get_part_by_blkorder(
	    modifiedpartitions[activedisk],
	    index);
	g_assert(modpartinfo != NULL);

	parttype = orchestrator_om_get_partition_type(modpartinfo);
	if (IS_EXT_PAR(parttype)) {
		/* If editing an extended partition for the first time we */
		/* want to de-sensitize all logicals */
		if (MainWindow.InstallationDiskWindow.initialsizechange[index]
		    == TRUE) {
			/* Scan through logicals and disable all logical combos */
			set_logical_combo_sensitivity(index, sensitivity, TRUE);
		} else {
			/* Just de-sensitize the last unused item, as it has */
			/* the potential of being destroyed */
			if (MainWindow.InstallationDiskWindow.numpartlogical[index] == 1) {
				/* Set to sensitive all the time regardless */
				set_logical_combo_sensitivity(index, TRUE, FALSE);
			} else {
				/* Just set the unused items */
				set_logical_combo_sensitivity(index, sensitivity, FALSE);
			}
		}
	}
#endif
}

void
partition_0_spinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_spinner_value_changed(widget, 0, user_data);
}

void
partition_1_spinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_spinner_value_changed(widget, 1, user_data);
}

void
partition_2_spinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_spinner_value_changed(widget, 2, user_data);
}

void
partition_3_spinner_value_changed(GtkWidget *widget, gpointer user_data)
{
	primary_partition_spinner_value_changed(widget, 3, user_data);
}

gboolean
partition_0_spinner_focus_in_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 0, FALSE, user_data);
	return (FALSE);
}

gboolean
partition_1_spinner_focus_in_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 1, FALSE, user_data);
	return (FALSE);
}

gboolean
partition_2_spinner_focus_in_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 2, FALSE, user_data);
	return (FALSE);
}

gboolean
partition_3_spinner_focus_in_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 3, FALSE, user_data);
	return (FALSE);
}

gboolean
partition_0_spinner_focus_out_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 0, TRUE, user_data);
	return (FALSE);
}

gboolean
partition_1_spinner_focus_out_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 1, TRUE, user_data);
	return (FALSE);
}

gboolean
partition_2_spinner_focus_out_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 2, TRUE, user_data);
	return (FALSE);
}

gboolean
partition_3_spinner_focus_out_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	primary_partition_spinner_focus_handler(
	    widget, event, 3, TRUE, user_data);
	return (FALSE);
}

static void
reset_primary_partitions(gboolean block_handlers)
{
	gint i = 0;
	gpointer objectdata = NULL;

	if (block_handlers)
		disk_partitioning_block_all_handlers();

	disk_comboboxes_ui_reset();

	/*
	 * Traverse through all partitions, if logical partitions created
	 * Remove them and collapse back to just 4 primary partitions
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		if (MainWindow.InstallationDiskWindow.startlogical[i] != NULL) {
			logical_partitions_remove_all_ui(i);
		}
		/* Initialize some other variables aswell */
		MainWindow.InstallationDiskWindow.partcombosaved[i] = UNUSED_PARTITION;
		MainWindow.InstallationDiskWindow.parttypechanges[i] = FALSE;
		MainWindow.InstallationDiskWindow.partsizechanges[i] = FALSE;
		MainWindow.InstallationDiskWindow.initialsizechange[i] = TRUE;
		MainWindow.InstallationDiskWindow.partrow[i] = i+1;
		MainWindow.InstallationDiskWindow.startlogical[i] = NULL;
		MainWindow.InstallationDiskWindow.numpartlogical[i] = 0;

		gtk_spin_button_set_range(GTK_SPIN_BUTTON(
		    MainWindow.InstallationDiskWindow.partspin[i]),
		    0, 0);
		gtk_spin_button_set_value(GTK_SPIN_BUTTON(
		    MainWindow.InstallationDiskWindow.partspin[i]), 0);

		gtk_label_set_text(GTK_LABEL(
		    MainWindow.InstallationDiskWindow.partavail[i]), "0.0");

		/*
		 * Remove any items previously added to display
		 * existing, unmodifiable partition types
		 */
		objectdata = g_object_get_data(G_OBJECT(
		    MainWindow.InstallationDiskWindow.partcombo[i]), "extra_fs");
		if ((objectdata != NULL) && (GPOINTER_TO_INT(objectdata) == TRUE)) {
			g_object_set_data(G_OBJECT(
			    MainWindow.InstallationDiskWindow.partcombo[i]),
			    "extra_fs",
			    GINT_TO_POINTER(FALSE));
		}
	}

	update_data_loss_warnings();

	if (block_handlers)
		disk_partitioning_unblock_all_handlers();
}

void
disk_partitioning_reset_button_clicked(GtkWidget *widget, gpointer user_data)
{
#if defined(__i386)
	gint i = 0;

	if (activedisk < 0)
		return;

	om_free_disk_partition_info(omhandle, modifiedpartitions[activedisk]);
	switch (get_disk_status(activedisk)) {
		case DISK_STATUS_OK:
			modifiedpartitions[activedisk] =
			    orchestrator_om_partitions_dup(originalpartitions[activedisk]);
			break;
		case DISK_STATUS_CANT_PRESERVE:
			modifiedpartitions[activedisk] =
			    orchestrator_om_partitions_dup(defaultpartitions[activedisk]);
			break;
		case DISK_STATUS_TOO_SMALL:
			g_warning("It shouldn't have been possible to"
			    "partition a disk that's too small\n");
			break;
	}

	/* Pretty sure we don't need this guy */
	/* installationdisk_get_original_blkorder_layout(activedisk); */
	installationdisk_blkorder_free_list(modifiedprimaryblkorder[activedisk]);
	installationdisk_blkorder_free_list(modifiedlogicalblkorder[activedisk]);
	modifiedprimaryblkorder[activedisk] =
	    installationdisk_blkorder_dup(originalprimaryblkorder[activedisk]);
	modifiedlogicalblkorder[activedisk] =
	    installationdisk_blkorder_dup(originallogicalblkorder[activedisk]);
	initialize_default_partition_layout(activedisk);

	update_data_loss_warnings();
	/* Flag for the the reset button to be disabled */
	g_object_set_data(G_OBJECT(diskbuttons[activedisk]),
	    "modified",
	    GINT_TO_POINTER(FALSE));
	disk_selection_set_active_disk(activedisk);

	g_debug("reset button pressed");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);
	print_gui(MainWindow.InstallationDiskWindow);
#endif /* (__i386) */
}

/* Internally referenced callbacks */

static void
installationdisk_diskbutton_toggled(GtkWidget *widget, gpointer user_data)
{
	gint disknum;
	disknum = GPOINTER_TO_INT(user_data);
	if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)) == FALSE)
		return;

	/*
	 * Set new disk selected, this will initialize display back to
	 * default then set it to contents of this disks modifiedpartitions
	 */
	disk_selection_set_active_disk(disknum);

	g_debug("After Current Active Disk :");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
	print_blkorder(alldiskinfo[activedisk],
	    modifiedprimaryblkorder[activedisk],
	    modifiedlogicalblkorder[activedisk]);
	print_gui(MainWindow.InstallationDiskWindow);
}

/* Toggle the button when it gets the focus. */
static gboolean
installationdisk_diskbutton_focused(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),
	    !gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget)));
	return (FALSE);
}

/* UI initialisation functoins */
void
installationdisk_xml_init(void)
{
	gint i = 0;

	MainWindow.installationdiskwindowxml =
	    glade_xml_new(GLADEDIR "/" INSTALLATIONDISKFILENAME,
	    DISKNODE, NULL);
	MainWindow.InstallationDiskWindow.diskselectiontoplevel =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "diskselectiontoplevel");
	MainWindow.InstallationDiskWindow.custompartitioningvbox =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "custompartitioningvbox");
	MainWindow.InstallationDiskWindow.disksviewport =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "disksviewport");
	MainWindow.InstallationDiskWindow.diskselectionhscrollbar =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "diskselectionhscrollbar");
	MainWindow.InstallationDiskWindow.diskerrorimage =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "diskerrorimage");
	MainWindow.InstallationDiskWindow.diskwarningimage =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "diskwarningimage");
	MainWindow.InstallationDiskWindow.diskstatuslabel =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "diskstatuslabel");
	MainWindow.InstallationDiskWindow.diskwarninghbox =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "diskwarninghbox");

	MainWindow.InstallationDiskWindow.fdiskscrolledwindow =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "fdiskscrolledwindow");
	MainWindow.InstallationDiskWindow.fdiskviewport =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "fdiskviewport");
	MainWindow.InstallationDiskWindow.fdisktable =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "fdisktable");
	MainWindow.InstallationDiskWindow.fdisktablerows =
	    GUI_INSTALL_FDISK_TABLE_ROWS;

	/* Partition combo boxes */
	MainWindow.InstallationDiskWindow.partcombo[0] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition0combo");
	MainWindow.InstallationDiskWindow.partcombo[1] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition1combo");
	MainWindow.InstallationDiskWindow.partcombo[2] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition2combo");
	MainWindow.InstallationDiskWindow.partcombo[3] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition3combo");

	/* Partition spin buttons */
	MainWindow.InstallationDiskWindow.partspin[0] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition0spinner");
	MainWindow.InstallationDiskWindow.partspin[1] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition1spinner");
	MainWindow.InstallationDiskWindow.partspin[2] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition2spinner");
	MainWindow.InstallationDiskWindow.partspin[3] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition3spinner");

	/* Partition avail labels */
	MainWindow.InstallationDiskWindow.partavail[0] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition0avail");
	MainWindow.InstallationDiskWindow.partavail[1] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition1avail");
	MainWindow.InstallationDiskWindow.partavail[2] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition2avail");
	MainWindow.InstallationDiskWindow.partavail[3] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition3avail");

	/* Partition warning messages */
	MainWindow.InstallationDiskWindow.partwarnbox[0] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition0warninghbox");
	MainWindow.InstallationDiskWindow.partwarnbox[1] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition1warninghbox");
	MainWindow.InstallationDiskWindow.partwarnbox[2] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition2warninghbox");
	MainWindow.InstallationDiskWindow.partwarnbox[3] =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partition3warninghbox");

	MainWindow.InstallationDiskWindow.resetbutton =
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "fdiskresetbutton");

	/* Initialize widgets to default values */
	reset_primary_partitions(TRUE);
}

/*
 * Update the disk icons to match the new icon theme
 */
void
icon_theme_changed(GtkIconTheme *theme, gpointer user_data)
{
	gint disknum;
	DiskStatus status;

	for (disknum = 0; disknum < numdisks; disknum++) {
		status = get_disk_status(disknum);
		if (status == DISK_STATUS_NO_DISKINFO)
			continue;

		set_diskbutton_icon(GTK_WIDGET(diskbuttons[disknum]),
		    create_diskbutton_icon(status, alldiskinfo[disknum]));
	}
}

/* For each combobox, update width if need be because of style change */
static void
disk_update_combobox_widths(void)
{
	gint i = 0;
	GtkCellRenderer *renderer = NULL;
	GList *cells = NULL;
	gint new_width = 0;
	gint cur_width = 0;
	gint cur_height = 0;
	LogicalPartition *curlogical = NULL;

	new_width = get_max_cell_renderer_width();
	new_width = new_width+1;
	if (new_width == max_combo_width)
		return;

	max_combo_width = new_width;

	/*
	 * Loop through each primary partition combobox setting new max width
	 * if required
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		cells = gtk_cell_layout_get_cells(
		    GTK_CELL_LAYOUT(MainWindow.InstallationDiskWindow.partcombo[i]));

		/*
		 * We know there's only one cell renderer per combobox so just use
		 * first list element
		 */
		renderer = (GtkCellRenderer *)cells->data;
		gtk_cell_renderer_set_fixed_size(renderer,
		    max_combo_width+LOGICAL_COMBOBOX_INDENT, -1);
		g_list_free(cells);

		if (MainWindow.InstallationDiskWindow.startlogical[i] != NULL) {
			/* Cycle through all logical partition combo's */
			for (curlogical = MainWindow.InstallationDiskWindow.startlogical[i];
			    curlogical != NULL;
			    curlogical = curlogical->next) {
				cells = gtk_cell_layout_get_cells(
				    GTK_CELL_LAYOUT(curlogical->typecombo));

				renderer = (GtkCellRenderer *)cells->data;
				gtk_cell_renderer_set_fixed_size(renderer, max_combo_width, -1);
				g_list_free(cells);
			}
		}

	}
}

void
combobox_style_set(GtkWidget *widget, GtkStyle *style, gpointer user_data)
{
	GtkCellRenderer *renderer = (GtkCellRenderer *)user_data;

	if (style != NULL) {
		/*
		 * Possible reset all gtkcombo box widths
		 * ensures against label clipping if font size has changed
		 */
		disk_update_combobox_widths();
	}
}

void
installationdisk_ui_init(void)
{
	GdkColor backcolour;
	gchar *minsizetext = NULL;
	gint i = 0;

	icontheme = gtk_icon_theme_get_default();

	minsizetext = g_strdup_printf(_("Recommended size: %lldGB Minimum: %.1fGB"),
	    orchestrator_om_get_recommended_sizegb(),
	    orchestrator_om_get_mininstall_sizegb(TRUE));
	gtk_label_set_text(
	    GTK_LABEL(MainWindow.screentitlesublabel2), minsizetext);
	g_free(minsizetext);

	disk_viewport_ui_init(
	    GTK_VIEWPORT(MainWindow.InstallationDiskWindow.disksviewport));
	disk_comboboxes_ui_init();

	gtk_box_pack_start(GTK_BOX(MainWindow.screencontentvbox),
	    MainWindow.InstallationDiskWindow.diskselectiontoplevel,
	    TRUE,
	    TRUE,
	    0);

	gdk_color_parse(WHITE_COLOR, &backcolour);
	gtk_widget_modify_bg(MainWindow.InstallationDiskWindow.disksviewport,
	    GTK_STATE_NORMAL, &backcolour);
	gtk_widget_modify_bg(MainWindow.InstallationDiskWindow.fdiskviewport,
	    GTK_STATE_NORMAL, &backcolour);

	/* Initially hide all partitioning controls until a disk is selected */
	gtk_widget_hide(glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partitioningvbox"));
	/* Custom partitioning is not shown initially */
	gtk_widget_hide(MainWindow.InstallationDiskWindow.custompartitioningvbox);

	/* Connect up scrollbar's adjustment to the viewport */
	viewportadjustment = gtk_range_get_adjustment(GTK_RANGE
	    (MainWindow.InstallationDiskWindow.diskselectionhscrollbar));
	g_signal_connect(G_OBJECT(viewportadjustment),
	    "changed",
	    G_CALLBACK(viewport_adjustment_changed),
	    (gpointer)MainWindow.InstallationDiskWindow.diskselectionhscrollbar);
	gtk_viewport_set_hadjustment(GTK_VIEWPORT
	    (MainWindow.InstallationDiskWindow.disksviewport),
	    viewportadjustment);

	/* Filter keyboard input on spinbuttons */
	for (i = 0; i < FD_NUMPART; i++) {
		spininserthandlers[i] = g_signal_connect(G_OBJECT
		    (MainWindow.InstallationDiskWindow.partspin[i]),
		    "insert-text",
		    G_CALLBACK(spinners_insert_text_filter),
		    GINT_TO_POINTER(i));
		spindeletehandlers[i] = g_signal_connect(G_OBJECT
		    (MainWindow.InstallationDiskWindow.partspin[i]),
		    "delete-text",
		    G_CALLBACK(spinners_delete_text_filter),
		    GINT_TO_POINTER(i));
	}
	glade_xml_signal_autoconnect(MainWindow.installationdiskwindowxml);

	g_signal_connect(G_OBJECT(
	    MainWindow.InstallationDiskWindow.diskstatuslabel), "style-set",
	    G_CALLBACK(combobox_style_set),
	    (gpointer) NULL);

	if (MainWindow.MileStoneComplete[OM_UPGRADE_TARGET_DISCOVERY] == FALSE) {
		g_timeout_add(200, partition_discovery_monitor, NULL);
	} else { /* Go straight to disk display function */
		partition_discovery_monitor(NULL);
	}
}

static void
initialize_default_partition_layout(gint disknum)
{
	disk_info_t *diskinfo = alldiskinfo[disknum];
	disk_parts_t *partitions = modifiedpartitions[disknum];
	gint logpartindex, primpartindex;
	partition_info_t *primpartinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	partition_info_t *freepartinfo = NULL;
	gboolean haveunused = FALSE;
	gint primparttype, logparttype;

	/*
	 * Disks are to be displayed in Block Order which is determined via
	 * pinfo->partition_order
	 * Process 4 possible primary partitions if one array entry is not set
	 * then allocate an unused block to this partition entry.
	 */
	for (primpartindex = 0;
	    primpartindex < FD_NUMPART;
	    primpartindex++) {
		primpartinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, primpartindex);
		if (!primpartinfo) {
			/*
			 * Partition not found for this order point,
			 * Indicates partition does not exist at this order point or we
			 * have gone beyond the maximum
			 * Either way, as we as calculating primary partitions here and
			 * 4 primary partitions are always displayed, set this pinfo[i]
			 * Element to unused, so it can be displayed, and assign a free
			 * block of space if it exists
			 */
			primpartinfo =
			    orchestrator_om_find_unused_primary_partition(partitions,
			    UNUSED,
			    primpartindex);
			g_assert(primpartinfo != NULL);
			freepartinfo = installationdisk_get_largest_free_block(
			    disknum, TRUE,
			    modifiedprimaryblkorder[disknum],
			    primpartinfo);
			if (freepartinfo) {
				primpartinfo->partition_size = freepartinfo->partition_size;
				primpartinfo->partition_offset = freepartinfo->partition_offset;
				primpartinfo->content_type = freepartinfo->content_type;
				primpartinfo->active = freepartinfo->active;
				primpartinfo->partition_offset_sec =
				    freepartinfo->partition_offset_sec;
				primpartinfo->partition_size_sec =
				    freepartinfo->partition_size_sec;
				haveunused = TRUE;
			}
		} else {
			primparttype = orchestrator_om_get_partition_type(primpartinfo);
			if (IS_EXT_PAR(primparttype)) {
				/*
				 * Process Logical disks, ensuring all gaps are filled in
				 * And ensuring we have at least one disk
				 */
				for (logpartindex = FD_NUMPART;
				    logpartindex < OM_NUMPART;
				    logpartindex++) {
					logpartinfo =
					    orchestrator_om_get_part_by_blkorder(partitions,
					    logpartindex);

					if (!logpartinfo) {
						/*
						 * For each of the unused blocks between logicals
						 * Add a new item, if space at the end add one
						 */
						while (installationdisk_get_largest_free_block(
						    disknum, FALSE,
						    modifiedlogicalblkorder[disknum], NULL) != NULL) {
							/*
							 * Found a usable free block, allocate a new
							 * Logical element to cater for this free block
							 */
							logpartinfo =
							    orchestrator_om_find_unused_logical_partition(
							    partitions,
							    UNUSED,
							    logpartindex);
							g_assert(logpartinfo != NULL);
							freepartinfo =
							    installationdisk_get_largest_free_block(
							    disknum, TRUE,
							    modifiedlogicalblkorder[disknum],
							    logpartinfo);
							g_assert(freepartinfo != NULL);
							logpartinfo->partition_size =
							    freepartinfo->partition_size;
							logpartinfo->partition_offset =
							    freepartinfo->partition_offset;
							logpartinfo->content_type =
							    freepartinfo->content_type;
							logpartinfo->active = freepartinfo->active;
							logpartinfo->partition_offset_sec =
							    freepartinfo->partition_offset_sec;
							logpartinfo->partition_size_sec =
							    freepartinfo->partition_size_sec;
							haveunused = TRUE;

							/*
							 * Increment logical partition id, and validate
							 * to within logical limits of < OM_NUMPART
							 */
							if (++logpartindex >= OM_NUMPART)
								break;
						}
						break;
					}
				}
			}
		}
	}

	if (haveunused == TRUE) {
		installationdisk_reorder_to_blkorder(partitions,
		    modifiedprimaryblkorder[disknum],
		    modifiedlogicalblkorder[disknum]);
	}
}

/* Initialises UI widgets for the selected disk */
static void
disk_selection_set_active_disk(int disknum)
{
	GtkToggleButton *usewholediskradio;
	DiskStatus status;
	gchar *markup;
	gint partitionsmodified = 0;
	gint i = 0;

	disk_partitioning_block_all_handlers();

	activedisk = disknum;
	/* First see if the disk is large enough for installation */
	status = get_disk_status(disknum);
	switch (status) {
		case DISK_STATUS_OK:
			/*
			 * If disk is too big, display warning
			 * in icon message area
			 */
			if (disk_is_too_big(alldiskinfo[disknum])) {
				markup = g_strdup_printf("<span font_desc="
				    "\"Bold\">%s</span>",
				    _("Usable size limited to 2TB"));

				gtk_widget_show(MainWindow.
				    InstallationDiskWindow.diskwarningimage);

				gtk_label_set_markup(GTK_LABEL
				    (MainWindow.InstallationDiskWindow.diskstatuslabel),
				    markup);
				gtk_widget_show(
				    MainWindow.InstallationDiskWindow.diskstatuslabel);
			} else {
				markup = g_strdup(" ");
				gtk_widget_hide(MainWindow.
				    InstallationDiskWindow.diskwarningimage);
				gtk_label_set_text(GTK_LABEL
				    (MainWindow.InstallationDiskWindow.diskstatuslabel),
				    markup);
			}

			disk_partitioning_set_sensitive(TRUE);
			gtk_widget_hide(MainWindow.InstallationDiskWindow.diskerrorimage);
			activediskisreadable = TRUE;
			gtk_widget_hide(
			    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
			    "unreadablepartsouterhbox"));
			gtk_widget_show(
			    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
			    "partsfoundlabel"));
			g_free(markup);
			break;
		case DISK_STATUS_TOO_SMALL:
			markup = g_strdup_printf("<span font_desc=\"Bold\">%s</span>",
			    _("This disk is too small"));
			disk_partitioning_set_sensitive(FALSE);
			gtk_label_set_markup(GTK_LABEL
			    (MainWindow.InstallationDiskWindow.diskstatuslabel),
			    markup);
			g_free(markup);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskstatuslabel);
			gtk_widget_hide(MainWindow.InstallationDiskWindow.diskwarningimage);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskerrorimage);
			break;
		case DISK_STATUS_NO_MEDIA:
			markup = g_strdup_printf("<span font_desc=\"Bold\">%s</span>",
			    _("This storage device contains no media"));
			disk_partitioning_set_sensitive(FALSE);
			gtk_label_set_markup(GTK_LABEL
			    (MainWindow.InstallationDiskWindow.diskstatuslabel),
			    markup);
			g_free(markup);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskstatuslabel);
			gtk_widget_hide(MainWindow.InstallationDiskWindow.diskwarningimage);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskerrorimage);
			break;
		case DISK_STATUS_CANT_PRESERVE:
			/*
			 * If disk is too big and/or partition
			 * configuration can't be preserved, display
			 * warning in icon message area
			 */
			if (disk_is_too_big(alldiskinfo[disknum])) {
				markup = g_strdup_printf("<span font_desc="
				    "\"Bold\">%s</span>",
				    _("The entire disk will be erased, "
				    "usable size limited to 2TB"));
			} else {
				markup = g_strdup_printf("<span font_desc="
				    "\"Bold\">%s</span>",
				    _("The entire disk will be erased"));
			}

			disk_partitioning_set_sensitive(TRUE);
			gtk_label_set_markup(GTK_LABEL
			    (MainWindow.InstallationDiskWindow.diskstatuslabel),
			    markup);
			g_free(markup);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskstatuslabel);
			gtk_widget_hide(MainWindow.InstallationDiskWindow.diskerrorimage);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskwarningimage);
			break;
		case DISK_STATUS_LARGE_WARNING:
			markup = g_strdup_printf("<span font_desc=\"Bold\">%s</span>",
			    _("Usable size limited to 2TB"));
			disk_partitioning_set_sensitive(TRUE);
			gtk_label_set_markup(GTK_LABEL
			    (MainWindow.InstallationDiskWindow.diskstatuslabel),
			    markup);
			g_free(markup);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskstatuslabel);
			gtk_widget_show(MainWindow.InstallationDiskWindow.diskwarningimage);
			gtk_widget_hide(MainWindow.InstallationDiskWindow.diskerrorimage);
			break;
	}

#if defined(__i386)
	/* Create a default, single partition layout for the disk */
	if (defaultpartitions[disknum] == NULL) {
		defaultpartitions[disknum] =
		    installation_disk_create_default_layout(alldiskinfo[disknum]);
	}

	/* Create initial partitioning layouts if necessary */
	if (originalpartitions[disknum] == NULL) {
		if (status == DISK_STATUS_OK) {
			disk_parts_t *partitions;
			char *diskname = g_strdup(alldiskinfo[disknum]->disk_name);
			partitions = orchestrator_om_get_disk_partitions(omhandle,
			    diskname);
			originalpartitions[disknum] =
			    orchestrator_om_partitions_dup(partitions);
			om_free_disk_partition_info(omhandle, partitions);
			g_free(diskname);
			modifiedpartitions[disknum] =
			    orchestrator_om_partitions_dup(originalpartitions[disknum]);
		} else if (status == DISK_STATUS_CANT_PRESERVE) {
			/*
			 * No original partitions can be read so just set it to the
			 * the default partitioning layout.
			 */
			originalpartitions[disknum] =
			    orchestrator_om_partitions_dup(defaultpartitions[disknum]);
			modifiedpartitions[disknum] =
			    orchestrator_om_partitions_dup(defaultpartitions[disknum]);
		} else {
			originalpartitions[disknum] =
			    orchestrator_om_partitions_dup(defaultpartitions[disknum]);
			modifiedpartitions[disknum] =
			    orchestrator_om_partitions_dup(defaultpartitions[disknum]);
		}

		/*
		 * Process modifiedpartitions[disknum], and set up what an initial
		 * display should look like including free gaps etc..
		 */
		g_debug("Before Set Current Active Disk :");
		print_partinfos(activedisk, alldiskinfo, modifiedpartitions);

		/* Get original block order layout */
		installationdisk_get_blkorder_layout(
		    alldiskinfo[disknum],
		    originalpartitions[disknum],
		    &originalprimaryblkorder[disknum],
		    &originallogicalblkorder[disknum]);

		modifiedprimaryblkorder[disknum] = installationdisk_blkorder_dup(
		    originalprimaryblkorder[disknum]);
		modifiedlogicalblkorder[disknum] = installationdisk_blkorder_dup(
		    originallogicalblkorder[disknum]);

		initialize_default_partition_layout(disknum);
	}

	if (status == DISK_STATUS_CANT_PRESERVE) {
		activediskisreadable = FALSE;
		gtk_widget_hide(
		    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
		    "partsfoundlabel"));
		gtk_widget_show(
		    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
		    "unreadablepartsouterhbox"));
	}

	if ((status == DISK_STATUS_OK) || (status == DISK_STATUS_CANT_PRESERVE)) {
		disk_partitioning_set_from_parts_data(alldiskinfo[disknum],
		    modifiedpartitions[disknum]);
		usewholediskradio = GTK_TOGGLE_BUTTON
		    (glade_xml_get_widget(MainWindow.installationdiskwindowxml,
		    "wholediskradio"));
		if (gtk_toggle_button_get_active(usewholediskradio) == TRUE)
			proposedpartitions[disknum] = defaultpartitions[disknum];
		else
			proposedpartitions[disknum] = modifiedpartitions[disknum];
	}

	update_data_loss_warnings();

	partitionsmodified = GPOINTER_TO_INT(
	    g_object_get_data(G_OBJECT
	    (diskbuttons[disknum]),
	    "modified"));
	gtk_widget_set_sensitive(GTK_WIDGET
	    (MainWindow.InstallationDiskWindow.resetbutton),
	    partitionsmodified);

#endif
	disk_partitioning_unblock_all_handlers();

	activedisk = disknum;
}

/*
 * Create an icon for the disk with an emblem if necessary
 */
static GtkWidget *
create_diskbutton_icon(DiskStatus status, disk_info_t *diskinfo)
{
	GtkIconInfo *diskiconinfo;
	GtkIconInfo *emblemiconinfo = NULL;

	GtkWidget *diskbaseimage;
	GdkPixbuf *diskbasepixbuf;
	GdkPixbuf *emblempixbuf;
	gint diskwidth, diskheight;
	gint emblemwidth, emblemheight;

	const gchar *diskfilename, *emblemfilename = NULL;

	/*
	 * Icon size has to be hardcoded to 48 rather than using
	 * GTK_ICON_SIZE_DIALOG or it looks too small.
	 */
	if (status == DISK_STATUS_NO_MEDIA) {
		diskiconinfo = gtk_icon_theme_lookup_icon(icontheme,
		    "gnome-dev-removable",
		    48,
		    0);
	} else {
		diskiconinfo = gtk_icon_theme_lookup_icon(icontheme,
		    "gnome-dev-harddisk",
		    48,
		    0);
	}
	diskfilename = gtk_icon_info_get_filename(diskiconinfo);

	diskbasepixbuf = gdk_pixbuf_new_from_file(diskfilename, NULL);

	diskwidth = gdk_pixbuf_get_width(diskbasepixbuf);
	diskheight = gdk_pixbuf_get_height(diskbasepixbuf);
	switch (status) {
		case DISK_STATUS_OK:
		case DISK_STATUS_CANT_PRESERVE:
			/*
			 * If disk is too big, mark icon with warning tag
			 */
			if (disk_is_too_big(diskinfo)) {
				emblemiconinfo =
				    gtk_icon_theme_lookup_icon(icontheme,
				    "dialog-warning",
				    16,
				    0);
			}
			break;
		case DISK_STATUS_TOO_SMALL:
			emblemiconinfo =
			    gtk_icon_theme_lookup_icon(icontheme,
			    "dialog-error",
			    16,
			    0);
			break;
		case DISK_STATUS_WARNING:
		case DISK_STATUS_LARGE_WARNING:
			emblemiconinfo =
			    gtk_icon_theme_lookup_icon(icontheme,
			    "dialog-warning",
			    16,
			    0);
			break;
		default:
			break;
	}
	if (emblemiconinfo != NULL) {
		emblemfilename = gtk_icon_info_get_filename(emblemiconinfo);
		emblempixbuf = gdk_pixbuf_new_from_file(emblemfilename, NULL);
		emblemwidth = gdk_pixbuf_get_width(emblempixbuf);
		emblemheight = gdk_pixbuf_get_height(emblempixbuf);

		gdk_pixbuf_composite(emblempixbuf,
		    diskbasepixbuf,
		    diskwidth - emblemwidth,
		    diskheight - emblemheight,
		    emblemwidth,
		    emblemheight,
		    diskwidth - emblemwidth,
		    diskheight - emblemheight,
		    1,
		    1,
		    GDK_INTERP_BILINEAR,
		    255);
	}
	diskbaseimage = gtk_image_new_from_pixbuf(diskbasepixbuf);
	g_object_unref(G_OBJECT(diskbasepixbuf));
	gtk_widget_show(diskbaseimage);

	return (diskbaseimage);
}

/* Set the image */
static void
set_diskbutton_icon(GtkWidget *button, GtkWidget *image)
{
	GtkWidget *vbox;
	GtkWidget *oldimage;

	vbox = g_object_get_data(G_OBJECT(button),
	    "iconvbox");
	oldimage = g_object_get_data(G_OBJECT(button),
	    "icon");
	if (oldimage != NULL)
		gtk_widget_destroy(oldimage);
	gtk_box_pack_start(GTK_BOX(vbox), image, TRUE, TRUE, 0);
	g_object_set_data(G_OBJECT(button),
	    "icon",
	    (gpointer)image);
}


/*
 * Create iconic radio buttons for the viewport scrollable area
 */
static GtkWidget *
disk_toggle_button_new_with_label(const gchar *label,
	DiskStatus status, disk_info_t *diskinfo)
{
	GtkRadioButton *button;
	GtkWidget *alignment;
	GtkWidget *vbox;
	GtkWidget *buttonlabel;
	GtkWidget *diskbaseimage;
	gint diskwidth, diskheight;
	gint emblemwidth, emblemheight;
	static GtkRadioButton *firstbutton = NULL;

	if (!firstbutton) {
		button = GTK_RADIO_BUTTON(gtk_radio_button_new(NULL));
		firstbutton = button;
	} else
		button = GTK_RADIO_BUTTON(
		    gtk_radio_button_new_from_widget(firstbutton));

	/* Don't draw the check box indicator of the normal radiobutton */
	g_object_set(G_OBJECT(button),
	    "draw-indicator", FALSE,
	    NULL);
	gtk_button_set_relief(GTK_BUTTON(button), GTK_RELIEF_NONE);

	alignment = gtk_alignment_new(0.5, 0.5, 0, 0);
	gtk_widget_show(alignment);
	gtk_container_add(GTK_CONTAINER(button), alignment);

	vbox = gtk_vbox_new(FALSE, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(alignment), vbox);
	g_object_set_data(G_OBJECT(button),
	    "iconvbox",
	    (gpointer)vbox);

	set_diskbutton_icon(GTK_WIDGET(button),
	    create_diskbutton_icon(status, diskinfo));

	buttonlabel = gtk_label_new(label);
	gtk_widget_show(buttonlabel);
	gtk_box_pack_end(GTK_BOX(vbox), buttonlabel, FALSE, FALSE, 0);

	return (GTK_WIDGET(button));
}

static void
render_partitiontype_name(GtkCellLayout *layout,
	GtkCellRenderer *cell,
	GtkTreeModel *model,
	GtkTreeIter *iter,
	gpointer user_data)
{
	gchar *text = NULL;
	gtk_tree_model_get(model, iter, 0, &text, -1);
	if (!text)
		return;
	g_object_set(cell, "text", text, NULL);
}

gchar *
installationdisk_parttype_to_string(partition_info_t *partinfo)
{
	gchar *type;
	gint parttype = orchestrator_om_get_partition_type(partinfo);

	switch (parttype) {
		case UNIXOS:
			type = Ustr;
			break;
		case SUNIXOS:
			if (partinfo->content_type == OM_CTYPE_LINUXSWAP)
				type = LINUXstr;
			else
				type = SUstr;
			break;
		case SUNIXOS2:
			type = SU2str;
			break;
		case X86BOOT:
			type = X86str;
			break;
		case DOSOS12:
			type = Dstr;
			break;
		case DOSOS16:
			type = D16str;
			break;
		case EXTDOS:
			type = EDstr;
			break;
		case DOSDATA:
			type = DDstr;
			break;
		case DOSHUGE:
			type = DBstr;
			break;
		case PCIXOS:
			type = PCstr;
			break;
		case DIAGPART:
			type = DIAGstr;
			break;
		case FDISK_IFS:
			type = IFSstr;
			break;
		case FDISK_AIXBOOT:
			type = AIXstr;
			break;
		case FDISK_AIXDATA:
			type = AIXDstr;
			break;
		case FDISK_OS2BOOT:
			type = OS2str;
			break;
		case FDISK_WINDOWS:
			type = WINstr;
			break;
		case FDISK_EXT_WIN:
			type = EWINstr;
			break;
		case FDISK_FAT95:
			type = FAT95str;
			break;
		case FDISK_EXTLBA:
			type = EXTLstr;
			break;
		case FDISK_LINUX:
			type = LINUXstr;
			break;
		case FDISK_CPM:
			type = CPMstr;
			break;
		case FDISK_NOVELL2:
			type = NOV2str;
			break;
		case FDISK_NOVELL3:
			type = NOVstr;
			break;
		case FDISK_QNX4:
			type = QNXstr;
			break;
		case FDISK_QNX42:
			type = QNX2str;
			break;
		case FDISK_QNX43:
			type = QNX3str;
			break;
		case FDISK_LINUXNAT:
			type = LINNATstr;
			break;
		case FDISK_NTFSVOL1:
			type = NTFSVOL1str;
			break;
		case FDISK_NTFSVOL2:
			type = NTFSVOL2str;
			break;
		case FDISK_BSD:
			type = BSDstr;
			break;
		case FDISK_NEXTSTEP:
			type = NEXTSTEPstr;
			break;
		case FDISK_BSDIFS:
			type = BSDIFSstr;
			break;
		case FDISK_BSDISWAP:
			type = BSDISWAPstr;
			break;
		case EFI_PMBR:
		case EFI_FS:
			type = EFIstr;
			break;
		case OTHEROS:
			type = Ostr;
			break;
		default:
			type = _("Unknown");
			break;
	}

	return (type);
}

static gfloat
calculate_avail_space(DiskBlockOrder *startblkorder,
	gint partindex,
	partition_info_t *partinfo)
{
	DiskBlockOrder *curblkorder = NULL;
	gfloat retavail = 0;
	gint parttype = 0;
	gfloat space_above = 0;
	gfloat space_below = 0;
	gfloat part_size = 0;
	gboolean break_on_next_unused = FALSE;
	GtkComboBox *combo;
	int comboindex = 0;

	/*
	 * If partition_id does not exist in blkorder, then unused and
	 * not displayed so therefore zero size avail
	 */
	if (installationdisk_blkorder_get_by_partition_id(
	    startblkorder, partinfo->partition_id) == NULL) {
		return (0);
	}

	/*
	 * if partindex != -1, function called from combo change signal
	 * handler, in this scenario we use the comboindex to determine
	 * partition type instead of the linked stored value.
	 */
	if (partindex != -1) {
		/* Get values for combobox, spinner and avail */
		combo = GTK_COMBO_BOX(
		    MainWindow.InstallationDiskWindow.partcombo[partindex]);
		comboindex = gtk_combo_box_get_active(combo);
	}

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {

		g_debug("A : %d  - %f - %f - %f - %f - %f",
		    curblkorder->partinfo.partition_id,
		    orchestrator_om_round_mbtogb(partinfo->partition_size),
		    orchestrator_om_round_mbtogb(curblkorder->partinfo.partition_size),
		    space_above,
		    part_size,
		    space_below);

		if (partindex != -1) {
			if (curblkorder->partinfo.partition_id == partinfo->partition_id) {
				/*
				 * Called by combobox change signal so type is new combobox
				 * Type not yet stored in this linked list.
				 */
				switch (comboindex) {
					case SOLARIS_PARTITION :
						parttype = SUNIXOS2;
						break;
					case EXTENDED_PARTITION :
						parttype = EXTDOS;
						break;
					default :
						parttype = UNUSED;
						break;
				}
			} else {
				parttype =
				    orchestrator_om_get_partition_type(&curblkorder->partinfo);
			}
		} else {
			parttype =
			    orchestrator_om_get_partition_type(&curblkorder->partinfo);
		}

		/* Now process as per normal */
		if (parttype == UNUSED) {
			if (curblkorder->partinfo.partition_id == partinfo->partition_id) {
				part_size =
				    orchestrator_om_round_mbtogb(partinfo->partition_size);
				break_on_next_unused = TRUE;
			} else {
				/* Unused block and not current so accumulate available space */
				if (break_on_next_unused == TRUE) {
					space_below += orchestrator_om_round_mbtogb(
					    curblkorder->partinfo.partition_size);
				} else {
					space_above += orchestrator_om_round_mbtogb(
					    curblkorder->partinfo.partition_size);
				}
			}
		} else {
			if (break_on_next_unused == TRUE) {
				/* Break out now as reached either end of disk or next used */
				break;
			}

			if (curblkorder->partinfo.partition_id == partinfo->partition_id) {
				part_size =
				    orchestrator_om_round_mbtogb(partinfo->partition_size);

				/*
				 * If EXTENDED or Solaris partition then we want to continue
				 * And calculate free space below it
				 */
				if (IS_EXT_PAR(parttype) ||
				    IS_SOLARIS_PAR(parttype, partinfo->content_type)) {
					break_on_next_unused = TRUE;
				} else {
					/* Non Solaris so therefore no extra free space to show */
					space_above = 0;
					space_below = 0;
					break;
				}
			} else {
				/*
				 * We've hit a partition thats not the one we want and it's
				 * also not free so reset the space_above to 0
				 */
				space_above = 0;
			}
		}

		g_debug("B : %d  - %f - %f - %f - %f - %f",
		    curblkorder->partinfo.partition_id,
		    orchestrator_om_round_mbtogb(partinfo->partition_size),
		    orchestrator_om_round_mbtogb(curblkorder->partinfo.partition_size),
		    space_above,
		    part_size,
		    space_below);
	}
	retavail = space_above + part_size + space_below;

	g_debug("Calc space avail : %d : %f (%f + %f + %f)",
	    partinfo->partition_id, retavail, space_above,
	    part_size, space_below);

	return (retavail);
}

static gfloat
get_extended_partition_min_size(disk_parts_t *partitions)
{
	gint lidx = 0;
	gfloat ret_size = 0.0;
	gfloat unused_to_remove = 0.0;
	partition_info_t *logpartinfo;
	gint parttype;

	/* Scan through logicals getting their total size */
	for (lidx = FD_NUMPART; lidx < OM_NUMPART; lidx++) {
		logpartinfo =
		    orchestrator_om_get_part_by_blkorder(partitions,
		    lidx);

		if (logpartinfo) {
			parttype = orchestrator_om_get_partition_type(logpartinfo);

			if (parttype == UNUSED) {
				unused_to_remove += ONE_DECIMAL(
				    orchestrator_om_get_partition_sizegb(logpartinfo));
			} else {
				ret_size += ONE_DECIMAL(
				    orchestrator_om_get_partition_sizegb(logpartinfo));
				if (unused_to_remove > 0) {
					ret_size += unused_to_remove;
					unused_to_remove = 0;
				}
			}
		} else {
			break;
		}
	}

	/* Minimum size to be returned is 0.1 */
	if (ret_size <= 0) {
		ret_size = 0.1;
	}

	return (ret_size);
}

static void
disk_partitioning_set_from_parts_data(disk_info_t *diskinfo,
	disk_parts_t *partitions)
{
	partition_info_t *primpartinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	LogicalPartition *logicalpart;
	GtkComboBox *primcombo, *logcombo;
	GtkSpinButton *primspinner, *logspinner;
	GtkLabel *primavail, *logavail;
	gchar *primtypestr, *logtypestr;
	gint logpartindex, primpartindex;
	gint primparttype, logparttype;
	gdouble spinvalue = 0;
	gfloat avail_space = 0;
	gfloat from_val = 0;

	/* Initialize GUI back to default */
	reset_primary_partitions(FALSE);

	print_from_parts(TRUE, NULL, 0, NULL, 0, NULL, 0);

	for (primpartindex = 0; primpartindex < FD_NUMPART; primpartindex++) {
		partition_info_t *origprimpartinfo = NULL;

		/* Should always return a valid partinfo struct */
		primpartinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, primpartindex);
		g_assert(primpartinfo != NULL);

		/*
		 * Attempt to get original equivalent as partitions may contain
		 * partinfo's for unsued spaces the partition_order will differ
		 * from the original partinfo array returned during target discovery
		 * To get original equivalent we must use partition_id
		 */
		origprimpartinfo =
		    orchestrator_om_get_part_by_partition_id(
		    originalpartitions[activedisk],
		    primpartinfo->partition_id);

		/* Set partition type of each fdisk partition in the comboboxes. */
		primcombo = GTK_COMBO_BOX
		    (MainWindow.InstallationDiskWindow.partcombo[primpartindex]);
		primspinner = GTK_SPIN_BUTTON
		    (MainWindow.InstallationDiskWindow.partspin[primpartindex]);
		primavail = GTK_LABEL
		    (MainWindow.InstallationDiskWindow.partavail[primpartindex]);
		primparttype = orchestrator_om_get_partition_type(primpartinfo);

		primtypestr = installationdisk_parttype_to_string(primpartinfo);

		/*
		 * Check for changes in the paritition type from original, if
		 * so flag it as a warning.
		 */
		if (origprimpartinfo != NULL &&
		    primparttype !=
		    orchestrator_om_get_partition_type(origprimpartinfo)) {
			MainWindow.InstallationDiskWindow.
			    parttypechanges[primpartindex] = TRUE;
		}

		if (primparttype == UNUSED) {
			gtk_combo_box_set_active(primcombo, UNUSED_PARTITION);
			MainWindow.InstallationDiskWindow.partcombosaved[primpartindex] =
			    UNUSED_PARTITION;
		} else if (IS_SOLARIS_PAR(primparttype, primpartinfo->content_type)) {
			gtk_combo_box_set_active(primcombo, SOLARIS_PARTITION);
			MainWindow.InstallationDiskWindow.partcombosaved[primpartindex] =
			    SOLARIS_PARTITION;
			/*
			 * Solaris partitions will always be erased because
			 * that's what we install onto, and we don't permit
			 * more than one solaris partition per disk. Solaris
			 * partitions can also be created/resized so they
			 * shouldn't be set to Unused:0.0GB like others.
			 * So always set it's size change flag to TRUE
			 */
			if (activediskisreadable == TRUE) {
				MainWindow.InstallationDiskWindow.partsizechanges[primpartindex] = TRUE;
			}
		} else if (IS_EXT_PAR(primparttype)) {
			/* Extended Partition */
			gtk_combo_box_set_active(primcombo, EXTENDED_PARTITION);
			MainWindow.InstallationDiskWindow.partcombosaved[primpartindex] =
			    EXTENDED_PARTITION;

			/*
			 * Check for changes in the paritition type from
			 * original, if so flag it as a warning.
			 */
			if (origprimpartinfo != NULL &&
			    primpartinfo->partition_size !=
			    orchestrator_om_get_partition_sizemb(
			    origprimpartinfo)) {
				MainWindow.InstallationDiskWindow.
				    partsizechanges[primpartindex] = TRUE;
			}
			/*
			 * Now we need to check for logical disks, and if there are any
			 * Dynamically create intended widgets to display these
			 * As only one extended partition allowed per disk, logical disks
			 * start at position 4 (id==5) in pinfo struct
			 */
			for (logpartindex = FD_NUMPART;
			    logpartindex < OM_NUMPART;
			    logpartindex++) {
				logpartinfo =
				    orchestrator_om_get_part_by_blkorder(
				    partitions,
				    logpartindex);

				/* Logical partition found so display */
				if (logpartinfo) {
					partition_info_t *origlogpartinfo = NULL;

					/*
					 * Attempt to get original equivalent as partitions may
					 * contain partinfo's for unsued spaces the partition_order
					 * will differ from the original partinfo array returned
					 * during target discovery. To get original equivalent we
					 * must use partition_id
					 */
					origlogpartinfo =
					    orchestrator_om_get_part_by_partition_id(
					    originalpartitions[activedisk],
					    logpartinfo->partition_id);

					/* Create new logical widgets for this logical partition */
					logicalpart = create_logical_partition_ui(primpartindex,
					    logpartindex, TRUE);
					logicalpart->logpartindex = logpartindex;

					logcombo = GTK_COMBO_BOX(logicalpart->typecombo);
					logspinner = GTK_SPIN_BUTTON(logicalpart->sizespinner);
					logavail = GTK_LABEL(logicalpart->availlabel);
					logparttype =
					    orchestrator_om_get_partition_type(logpartinfo);
					logtypestr =
					    installationdisk_parttype_to_string(logpartinfo);

					if (origlogpartinfo != NULL) {
						if (logparttype !=
						    orchestrator_om_get_partition_type(
						    origlogpartinfo)) {
							logicalpart->typechange = TRUE;
						}
					}

					if (logparttype == UNUSED) {
						gtk_combo_box_set_active(logcombo, UNUSED_PARTITION);
						logicalpart->partcombosaved = UNUSED_PARTITION;
					} else if (IS_SOLARIS_PAR(logparttype,
					    logpartinfo->content_type)) {
						gtk_combo_box_set_active(logcombo, SOLARIS_PARTITION);
						logicalpart->partcombosaved = SOLARIS_PARTITION;
						/*
						 * Solaris partitions will always be erased because
						 * that's what we install onto, and we don't permit
						 * more than one solaris partition per disk. Solaris
						 * partitions can also be created/resized so they
						 * shouldn't be set to Unused:0.0GB like others.
						 * So always set it's size change flag to TRUE
						 */
						if (activediskisreadable == TRUE) {
							logicalpart->sizechange = TRUE;
						}
					} else {
						gtk_combo_box_append_text(logcombo, logtypestr);

						/* Using EXTENDED_PARTITION just as enum reference */
						gtk_combo_box_set_active(logcombo, EXTENDED_PARTITION);
						logicalpart->partcombosaved = EXTENDED_PARTITION;
						g_object_set_data(G_OBJECT(logcombo),
						    "extra_fs",
						    GINT_TO_POINTER(TRUE));

						/*
						 * Check for changes in the paritition type from original, if
						 * so flag it as a warning.
						 */
						if (origlogpartinfo != NULL) {
							if (logpartinfo->partition_size !=
							    orchestrator_om_get_partition_sizemb(origlogpartinfo)) {
								logicalpart->sizechange = TRUE;
							}
						}

					}

					/*
					 * Get free space either side of this logical partition
					 * For logical partitions this is only pertinent for
					 * Solaris partitions
					 */
					avail_space = calculate_avail_space(
					    modifiedlogicalblkorder[activedisk],
					    -1, logpartinfo);

					set_range_avail_from_value(logspinner, logavail,
					    logparttype == UNUSED ? 0 : 0.1, avail_space);

					set_size_widgets_from_value(logspinner, NULL,
					    orchestrator_om_get_partition_sizegb(logpartinfo));

					print_from_parts(FALSE, "Logical", logpartindex,
					    logpartinfo,
					    orchestrator_om_get_partition_sizegb(logpartinfo),
					    logspinner, avail_space);

					/* For now, these types are all we support */
					if (IS_SOLARIS_PAR(logparttype, logpartinfo->content_type))
						gtk_widget_set_sensitive(GTK_WIDGET(logspinner), TRUE);
					else
						gtk_widget_set_sensitive(GTK_WIDGET(logspinner), FALSE);

					/* Set the avail space column */
				} else {
					/* Reached end of logicals so break out of loop */
					break;
				}
			}
		} else {
			/* Append exact partition type to combo and set this as active */
			gtk_combo_box_append_text(primcombo, primtypestr);
			gtk_combo_box_set_active(primcombo, NUM_DEFAULT_PARTITIONS);
			MainWindow.InstallationDiskWindow.partcombosaved[primpartindex] =
			    NUM_DEFAULT_PARTITIONS;

			/* Add extra_fs data object */
			g_object_set_data(G_OBJECT(primcombo),
			    "extra_fs",
			    GINT_TO_POINTER(TRUE));

			/*
			 * Check for changes in the paritition type from
			 * original, if so flag it as a warning.
			 */
			if (origprimpartinfo != NULL &&
			    primpartinfo->partition_size !=
			    orchestrator_om_get_partition_sizemb(origprimpartinfo)) {

				MainWindow.InstallationDiskWindow.
				    partsizechanges[primpartindex] = TRUE;
			}
		}

		/*
		 * Set the partition size of each partition
		 *
		 * This pre-rounding is necessary because the spin button will
		 * round the value before inserting it into the display and
		 * ocassionally cause the insert_text filter to spit it out
		 */

		/*
		 * Calculate free space either side of this particular partition
		 * This is only really pertinent if the current partition is a
		 * Solaris partition or an extended partition.
		 * Other partitions and UNUSED partitions cannot be resized
		 */
		avail_space = calculate_avail_space(
		    modifiedprimaryblkorder[activedisk],
		    -1, primpartinfo);

		if (IS_EXT_PAR(primparttype)) {
			/*
			 * For extended partitions the lowest value we can
			 * spin down to is the lowest size of it's containing
			 * logicals minus any unused space at the end.
			 */
			set_range_avail_from_value(primspinner, primavail,
			    get_extended_partition_min_size(partitions),
			    avail_space);
		} else {
			set_range_avail_from_value(primspinner, primavail,
			    primparttype == UNUSED ? 0 : 0.1,
			    avail_space);
		}

		set_size_widgets_from_value(primspinner, NULL,
		    orchestrator_om_get_partition_sizegb(primpartinfo));

		print_from_parts(FALSE, "Primary", primpartindex, primpartinfo,
		    orchestrator_om_round_mbtogb(primpartinfo->partition_size),
		    primspinner, avail_space);

		/* For now, these types are all we support */
		if (IS_SOLARIS_PAR(primparttype, primpartinfo->content_type)) {
			gtk_widget_set_sensitive(GTK_WIDGET(primcombo), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(primspinner), TRUE);
		} else if (IS_EXT_PAR(primparttype)) {
			gtk_widget_set_sensitive(GTK_WIDGET(primcombo), TRUE);
			gtk_widget_set_sensitive(GTK_WIDGET(primspinner), TRUE);
		} else {
			gtk_widget_set_sensitive(GTK_WIDGET(primspinner), FALSE);

			/* Check if avail/actual size > 0 */
			if (avail_space <= 0) {
				gtk_widget_set_sensitive(GTK_WIDGET(primcombo), FALSE);
			} else {
				gtk_widget_set_sensitive(GTK_WIDGET(primcombo), TRUE);
			}
		}
	}
	update_data_loss_warnings();
}

/* Get Maximum width of a GtkCellRender */
static gint
get_max_cell_renderer_width(void)
{
	GtkListStore *partitiontype_store;
	gint x, y, width, height;
	gint cur_max_width = 0, cur_max_height = 0;
	gint active_combo = 0;
	GtkWidget *dummy_combo = NULL;
	GtkCellRenderer *dummy_renderer = NULL;
	GtkTreeIter iter;

	/* Create combo box */
	dummy_combo = gtk_combo_box_new();

	/* Initialize dummy_combo */
	partitiontype_store = gtk_list_store_new(1, G_TYPE_STRING);
	gtk_list_store_append(partitiontype_store, &iter);
	gtk_list_store_set(partitiontype_store, &iter, 0, _(Unused), -1);

	gtk_combo_box_set_model(GTK_COMBO_BOX(dummy_combo),
	    GTK_TREE_MODEL(partitiontype_store));
	dummy_renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(dummy_combo),
	    dummy_renderer,
	    TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(dummy_combo),
	    dummy_renderer,
	    render_partitiontype_name,
	    NULL,
	    NULL);

	/*
	 * To get width, we need to attach item to parent so that combobox
	 * gets realized, but not displayed
	 */
	gtk_table_attach(GTK_TABLE(MainWindow.InstallationDiskWindow.fdisktable),
	    dummy_combo,
	    0,
	    1,
	    MainWindow.InstallationDiskWindow.fdisktablerows-1,
	    MainWindow.InstallationDiskWindow.fdisktablerows,
	    GTK_FILL,
	    0,
	    0,
	    0);

	/* Set to longest type string "NOVstr", and get size */
	g_object_set(dummy_renderer, "text", NOVstr, NULL);
	gtk_cell_renderer_get_size(dummy_renderer, GTK_WIDGET(dummy_combo), NULL,
	    &x, &y, &width, &height);

	gtk_widget_destroy(dummy_combo);

	return (width);
}

/* Populates the comboboxes with the supported fdisk partition types */
static void
disk_combobox_ui_init(GtkComboBox *combobox,
	gboolean is_primary)
{
	GtkCellRenderer *renderer;
	GtkListStore *partitiontype_store;
	GtkTreeIter iter;
	gint x, y, width, height;

	partitiontype_store = gtk_list_store_new(1, G_TYPE_STRING);

	/*
	 * The only valid partition *selectable* types are Unused & Solaris
	 * Everything else is is non selectable.
	 * Addendum, EXTENDED is selectable now.
	 */

	gtk_list_store_append(partitiontype_store, &iter);
	gtk_list_store_set(partitiontype_store, &iter, 0, _(Unused), -1);
	gtk_list_store_append(partitiontype_store, &iter);
	gtk_list_store_set(partitiontype_store, &iter, 0, SU2str, -1);

	if (is_primary == TRUE) {
		gtk_list_store_append(partitiontype_store, &iter);
		gtk_list_store_set(partitiontype_store, &iter, 0, _(Extended), -1);
	}

	gtk_combo_box_set_model(GTK_COMBO_BOX(combobox),
	    GTK_TREE_MODEL(partitiontype_store));

	renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combobox), renderer,  TRUE);
	gtk_cell_layout_set_cell_data_func(GTK_CELL_LAYOUT(combobox),
	    renderer,
	    render_partitiontype_name,
	    NULL,
	    NULL);

	/*
	 * Following code sets the max width of the cell within the combobox
	 * Doing For largest type string "NOVstr", ensures combobox does not
	 * Grow/Shrink in width when user selects different partition types
	 */

	if (max_combo_width == 0) {
		max_combo_width = get_max_cell_renderer_width();
	}

	if (is_primary == FALSE) {
		gtk_cell_renderer_set_fixed_size(renderer, max_combo_width, -1);
	} else {
		gtk_cell_renderer_set_fixed_size(renderer,
		    max_combo_width+LOGICAL_COMBOBOX_INDENT, -1);
	}

	gtk_combo_box_set_active(GTK_COMBO_BOX(combobox), UNUSED_PARTITION);
}

static void
disk_comboboxes_ui_init(void)
{
	gint i;

	for (i = 0; i < FD_NUMPART; i++) {
		disk_combobox_ui_init(GTK_COMBO_BOX
		    (MainWindow.InstallationDiskWindow.partcombo[i]),
		    TRUE);
	}
}
/* Reset the comboboxes with the supported fdisk partition types */
static void
disk_combobox_ui_reset(GtkComboBox *combobox,
	gboolean is_primary)
{
	GtkTreeModel *combomodel;
	gint n_children = 0, i = 0;

	/*
	 * Check of combobox already has a gtk_list_store
	 * If it does, Reset items to the default list of items
	 */
	combomodel = gtk_combo_box_get_model(GTK_COMBO_BOX(combobox));

	if (combomodel != NULL) {
		n_children = gtk_tree_model_iter_n_children(combomodel, NULL);
		for (i = n_children; i >= 0; i--) {
			gtk_combo_box_remove_text(GTK_COMBO_BOX(combobox), i);
		}
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), _(Unused));
		gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), SU2str);
		if (is_primary == TRUE) {
			gtk_combo_box_append_text(GTK_COMBO_BOX(combobox), _(Extended));
		}
	}
}

static void
disk_comboboxes_ui_reset(void)
{
	gint i;

	for (i = 0; i < FD_NUMPART; i++) {
		disk_combobox_ui_reset(GTK_COMBO_BOX
		    (MainWindow.InstallationDiskWindow.partcombo[i]),
		    TRUE);
	}
}

static void
init_disk_status(void)
{
	disk_parts_t *partitions;
	disk_info_t *diskinfo = NULL;
	DiskStatus *status = NULL;
	gint i = 0;

	g_return_if_fail(alldiskstatus != NULL);
	g_return_if_fail(numdisks > 0);
	for (i = 0; i < numdisks; i++) {
		diskinfo = alldiskinfo[i];
		status = &alldiskstatus[i];

		if (diskinfo == NULL) {
			g_warning("%d disks were detected but no information about disk "
			    "%d was found", numdisks, i);
			*status = DISK_STATUS_NO_DISKINFO;
			continue;
		}
		if (orchestrator_om_get_disk_sizemb(diskinfo) == 0) {
			*status = DISK_STATUS_NO_MEDIA;
			continue;
		}
		if (orchestrator_om_get_disk_sizegb(diskinfo) <
		    orchestrator_om_get_mininstall_sizegb(FALSE)) {
			g_warning("%s disk has %.1fGB (is too small)",
			    diskinfo->disk_name,
			    orchestrator_om_get_disk_sizegb(diskinfo));

			*status = DISK_STATUS_TOO_SMALL;
			continue;
		}

		if (diskinfo->label != OM_LABEL_VTOC &&
		    diskinfo->label != OM_LABEL_FDISK) {
			*status = DISK_STATUS_CANT_PRESERVE;
			continue;
		}
#if defined(__i386)
		partitions = orchestrator_om_get_disk_partitions(omhandle,
		    diskinfo->disk_name);
		if (!partitions) {
			g_message("Can't find disks partitions on device: %s",
			    diskinfo->disk_name);
			*status = DISK_STATUS_CANT_PRESERVE;
			continue;
		}
		om_free_disk_partition_info(omhandle, partitions);

		*status = DISK_STATUS_OK;
#else /* (__sparc) */
		/* On SPARC, the disk always gets wiped */
		*status = DISK_STATUS_CANT_PRESERVE;
#endif /* (__i386) */
	}
}

static DiskStatus
get_disk_status(guint disknum)
{
	g_return_val_if_fail(disknum < numdisks, -1);
	g_return_val_if_fail(alldiskstatus != NULL, -1);

	return (alldiskstatus[disknum]);
}

/*
 * Called when target discovery is complete and we're ready to query
 * the orchestrator for disk info
 */
static void
populate_data_from_orchestrator_discovery(void)
{
	gint i = 0, j = 0;

	alldiskinfo = orchestrator_om_get_disk_info(omhandle, &numdisks);

	g_return_if_fail(numdisks > 0);
	alldiskstatus = g_new0(DiskStatus, numdisks);

	originalpartitions = g_new0(disk_parts_t *, numdisks);
	modifiedpartitions = g_new0(disk_parts_t *, numdisks);
	proposedpartitions = g_new0(disk_parts_t *, numdisks);
	defaultpartitions  = g_new0(disk_parts_t *, numdisks);

	originalprimaryblkorder = g_new0(DiskBlockOrder *, numdisks);
	originallogicalblkorder = g_new0(DiskBlockOrder *, numdisks);
	modifiedprimaryblkorder = g_new0(DiskBlockOrder *, numdisks);
	modifiedlogicalblkorder = g_new0(DiskBlockOrder *, numdisks);

	init_disk_status();
}

static void
disk_viewport_diskbuttons_init(GtkViewport *viewport)
{
	gchar **disklabels;
	gchar *disktiptext;
	GtkTooltips *disktip;
	int disknum;
	DiskStatus status;

	/* Create the hbutton box first */
	hbuttonbox = gtk_hbutton_box_new();
	g_signal_connect(G_OBJECT(icontheme), "changed",
	    G_CALLBACK(icon_theme_changed),
	    (gpointer) hbuttonbox);

	gtk_button_box_set_spacing(hbuttonbox, 36);
	gtk_button_box_set_layout(GTK_BUTTON_BOX(hbuttonbox),
	    GTK_BUTTONBOX_START);

	diskbuttons = g_new0(GtkWidget *, numdisks);
	disklabels = g_new0(gchar *, numdisks);

	for (disknum = 0; disknum < numdisks; disknum++) {
		status = get_disk_status(disknum);
		if (status == DISK_STATUS_NO_DISKINFO) {
			g_warning("Skipping over installation target disk %d: "
			    "no disk info provided.",
			    disknum);
			continue;
		}
		disklabels[disknum] = disk_viewport_create_disk_label(disknum);
		disktip = gtk_tooltips_new();
		disktiptext = disk_viewport_create_disk_tiptext(disknum);
		diskbuttons[disknum] =
		    disk_toggle_button_new_with_label(disklabels[disknum],
		    status, alldiskinfo[disknum]);
		gtk_tooltips_set_tip(disktip, diskbuttons[disknum], disktiptext, NULL);
		g_free(disklabels[disknum]);
		g_free(disktiptext);
		gtk_widget_show(diskbuttons[disknum]);
		gtk_box_pack_start(GTK_BOX(hbuttonbox),
		    diskbuttons[disknum],
		    FALSE,
		    FALSE,
		    0);

		g_signal_connect(G_OBJECT(diskbuttons[disknum]),
		    "toggled",
		    G_CALLBACK(installationdisk_diskbutton_toggled),
		    GINT_TO_POINTER(disknum));
		g_signal_connect(G_OBJECT(diskbuttons[disknum]),
		    "focus",
		    G_CALLBACK(installationdisk_diskbutton_focused),
		    GINT_TO_POINTER(disknum));
		g_signal_connect(G_OBJECT(diskbuttons[disknum]),
		    "focus-in-event",
		    G_CALLBACK(disk_partitioning_button_focus_handler),
		    GINT_TO_POINTER(disknum));
	}

	g_free(disklabels);
	gtk_widget_show(hbuttonbox);
	gtk_container_add(GTK_CONTAINER(viewport), hbuttonbox);
}

/*
 * Return the index of the default disk
 * or -1 indicating an error.
 * The default disk should be the 1st
 * bootable disk, or the 1st usable disk,
 * or the 1st available disk.
 */
static gint
get_default_disk_index(void)
{
	gint chosendisk = -1;
	gint i = 0;

	for (i = 0; i < numdisks; i++) {
		if (get_disk_status(i) == DISK_STATUS_OK ||
		    get_disk_status(i) == DISK_STATUS_CANT_PRESERVE) {
			if (orchestrator_om_disk_is_bootdevice(alldiskinfo[i])) {
				/*
				 * If boot device is found
				 * and it's usable, look no further.
				 */
				chosendisk = i;
				break;
			} else if (chosendisk < 0)
				/*
				 * fall back to the 1st
				 * usable disk
				 */
				chosendisk = i;
		}
	}
	/* fall back to the 1st avaiable disk */
	if (numdisks > 0 && chosendisk < 0)
		chosendisk = 0;
	return (chosendisk);
}

static gboolean
partition_discovery_monitor(gpointer user_data)
{
	gchar *markup;
	gboolean bootdevfound = FALSE;
	gint chosendisk = -1;
	gint i = 0;

	/*
	 * Don't to anything until both target discovery and
	 * UI initialisation has been completed
	 */
	if (MainWindow.MileStoneComplete[OM_UPGRADE_TARGET_DISCOVERY] == FALSE)
		return (TRUE);

	GtkViewport *viewport;
	viewport = GTK_VIEWPORT(MainWindow.InstallationDiskWindow.disksviewport);
	populate_data_from_orchestrator_discovery();
	if (scanningbox) {
		gtk_widget_destroy(scanningbox);
	}

	if (numdisks == 0) {
		/* Display Info that no disks were found */
		markup = g_strdup_printf("<span font_desc=\"Bold\">%s</span>",
		    _("No disks were found."));

		gtk_label_set_markup(GTK_LABEL
		    (MainWindow.InstallationDiskWindow.diskstatuslabel),
		    markup);
		g_free(markup);

		gtk_widget_show(MainWindow.InstallationDiskWindow.diskerrorimage);
		gtk_widget_show(MainWindow.InstallationDiskWindow.diskstatuslabel);
	}

	disk_viewport_diskbuttons_init(viewport);

	/*
	 * Auto select the boot disk, or failing that, the first suitable disk
	 * and toggle the custom partitioning controls.
	 */
	chosendisk = get_default_disk_index();
	if (chosendisk >= 0) {
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
		    diskbuttons[chosendisk]),
		    TRUE);
		/*
		 * It's safe to call this on SPARC also since the callback
		 * is a no-op.
		 */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
		    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
		    "partitiondiskradio")),
		    TRUE);
		/* Force a toggle emission */
		gtk_toggle_button_toggled(GTK_TOGGLE_BUTTON(
		    diskbuttons[chosendisk]));
		if (GTK_WIDGET_VISIBLE(
		    MainWindow.InstallationDiskWindow.diskselectiontoplevel))
			gtk_widget_grab_focus(diskbuttons[chosendisk]);

#if defined(__i386)
		/* Show partitioning options on X86 only */
		gtk_widget_show(glade_xml_get_widget(
		    MainWindow.installationdiskwindowxml,
		    "partitioningvbox"));
#endif
	}

	return (FALSE);
}

static void
disk_viewport_ui_init(GtkViewport *viewport)
{
	GtkWidget *busyimage;
	GtkWidget *label;
	gchar *markup;

	/*
	 * XXX: Doesn't use image from the icon theme. Switch this to a stock
	 * animation in future releases when A11Y and theme support become a
	 * requirement.
	 */
	if (MainWindow.MileStoneComplete[OM_UPGRADE_TARGET_DISCOVERY] == FALSE) {
		markup = g_strdup_printf("<span font_desc=\"Bold\">%s</span>",
		    _("Finding Disks"));
		label = gtk_label_new(NULL);
		gtk_label_set_markup(GTK_LABEL(label), markup);
		g_free(markup);

		busyimage = gtk_image_new_from_file(PIXMAPDIR "/" "gnome-spinner.gif");
		gtk_widget_show(busyimage);

		scanningbox = gtk_vbox_new(FALSE, 0);
		gtk_box_pack_start(GTK_BOX(scanningbox), label, FALSE, FALSE, 0);
		gtk_box_pack_end(GTK_BOX(scanningbox), busyimage, FALSE, FALSE, 0);
		gtk_widget_show(label);
		gtk_widget_show(busyimage);
		gtk_widget_show(scanningbox);

		gtk_container_add(GTK_CONTAINER(viewport), scanningbox);
	}
}

static gchar*
disk_viewport_create_disk_tiptext(guint disknum)
{
	/*
	 * Tooltip consists of:
	 *  Size: <size>
	 *  Type: <usb|scsi|etc.>
	 *  Vendor: <vendor>
	 *  Devicename: <cXtXdX>
	 *  Bootdisk <Y/N>
	 *  <instance 1>
	 *  <instance 2>
	 *  <...>
	 */
	disk_info_t *diskinfo;
	upgrade_info_t *uinfos = NULL;
	upgrade_info_t *uinfo = NULL;
	guint16 ninstance;
	gfloat size;
	gchar *type;
	const gchar *vendor;
	gboolean isbootdisk;
	const gchar *devicename;
	gchar *newtiptext;
	gchar *instancetext;
	gchar *tiptext;
	gchar *units = "GB";

	diskinfo = alldiskinfo[disknum];
	orchestrator_om_get_upgrade_targets_by_disk(diskinfo,
	    &uinfos, &ninstance);

	size = orchestrator_om_get_total_disk_sizegb(diskinfo);
	/* if disk is bigger than 1TB, display disk size in TB */
	if (size > GBPERTB) {
		size /= GBPERTB;
		units = "TB";
	}

	type = orchestrator_om_get_disk_type(diskinfo);
	vendor = orchestrator_om_get_disk_vendor(diskinfo);
	devicename = orchestrator_om_get_disk_devicename(diskinfo);
	isbootdisk = orchestrator_om_disk_is_bootdevice(diskinfo);
	tiptext = g_strdup_printf(_("Size: %.1f%s\n"
	    "Type: %s\n"
	    "Vendor: %s\n"
	    "Device: %s\n"
	    "Boot device: %s"),
	    size,
	    units,
	    type,
	    vendor,
	    devicename,
	    isbootdisk ? _("Yes") : _("No"));

	uinfo = uinfos;
	while (uinfo) {
		instancetext =
		    orchestrator_om_upgrade_instance_get_release_name(uinfo);
		if (instancetext) {
			newtiptext = g_strconcat(tiptext, _("\n"),
			    instancetext, NULL);
			g_free(tiptext);
			tiptext = newtiptext;
		}
		uinfo = orchestrator_om_upgrade_instance_get_next(uinfo);
	}
	g_free(type);
	return (tiptext);
}

static gchar*
disk_viewport_create_disk_label(guint disknum)
{
	/* Label consists of: "<sizeinGB|TB>[GB|TB] <disktype> */
	gfloat disksize = 0;
	gchar *disktype  = NULL;
	gchar *disksizeunits = "GB";
	gchar *label = NULL;

	disktype = orchestrator_om_get_disk_type(alldiskinfo[disknum]);
	disksize = orchestrator_om_get_total_disk_sizegb(alldiskinfo[disknum]);

	/* if disk is bigger than 1TB, display disk size in TB */
	if (disksize > GBPERTB) {
		disksize /= GBPERTB;
		disksizeunits = "TB";
	}

	label = g_strdup_printf("%.1f%s %s", disksize, disksizeunits, disktype);
	g_free(disktype);
	return (label);
}


static void
disk_partitioning_block_all_handlers(void)
{
	gint i = 0;
	LogicalPartition *curlogical = NULL;

	gint mask = (1<<0 | 1<<1 | 1<<2 | 1<<3);
	disk_partitioning_block_spinbox_handlers(mask);
	disk_partitioning_block_combobox_handlers(mask);

	for (i = 0; i < FD_NUMPART; i++) {
		if (MainWindow.InstallationDiskWindow.startlogical[i] != NULL) {
			for (curlogical = MainWindow.InstallationDiskWindow.startlogical[i];
			    curlogical != NULL;
			    curlogical = curlogical->next) {

				/* Block spinner and combobox handler for logical partition */
				if (curlogical->combochangehandler > 0) {
					g_signal_handler_block(
					    (gpointer *)curlogical->typecombo,
					    curlogical->combochangehandler);
				}

				if (curlogical->spinnerchangehandler > 0) {
					g_signal_handler_block(
					    (gpointer *)curlogical->sizespinner,
					    curlogical->spinnerchangehandler);
				}

				if (curlogical->spinnerinserthandler > 0) {
					g_signal_handler_block(
					    (gpointer *)curlogical->sizespinner,
					    curlogical->spinnerinserthandler);
				}

				if (curlogical->spinnerdeletehandler > 0) {
					g_signal_handler_block(
					    (gpointer *)curlogical->sizespinner,
					    curlogical->spinnerdeletehandler);
				}
			}
		}
	}
}

static void
disk_partitioning_unblock_all_handlers(void)
{
	gint i = 0;
	LogicalPartition *curlogical = NULL;

	gint mask = (1<<0 | 1<<1 | 1<<2 | 1<<3);
	disk_partitioning_unblock_spinbox_handlers(mask);
	disk_partitioning_unblock_combobox_handlers(mask);

	for (i = 0; i < FD_NUMPART; i++) {
		if (MainWindow.InstallationDiskWindow.startlogical[i] != NULL) {
			for (curlogical = MainWindow.InstallationDiskWindow.startlogical[i];
			    curlogical != NULL;
			    curlogical = curlogical->next) {

				/* Block spinner and combobox handler for logical partition */
				if (curlogical->combochangehandler > 0) {
					g_signal_handler_unblock(
					    (gpointer *)curlogical->typecombo,
					    curlogical->combochangehandler);
				}

				if (curlogical->spinnerchangehandler > 0) {
					g_signal_handler_unblock(
					    (gpointer *)curlogical->sizespinner,
					    curlogical->spinnerchangehandler);
				}

				if (curlogical->spinnerinserthandler > 0) {
					g_signal_handler_unblock(
					    (gpointer *)curlogical->sizespinner,
					    curlogical->spinnerinserthandler);
				}

				if (curlogical->spinnerinserthandler > 0) {
					g_signal_handler_unblock(
					    (gpointer *)curlogical->sizespinner,
					    curlogical->spinnerdeletehandler);
				}
			}
		}
	}
}

static void
disk_partitioning_block_spinbox_handlers(gint mask)
{
	gint i = 0;

	if (mask == 0)
		return;

	for (i = 0; i < FD_NUMPART; i++) {
		if (mask & 1<<i) {
			if (spininserthandlers[i] > 0) {
				g_signal_handler_block(
				    (gpointer *)MainWindow.InstallationDiskWindow.partspin[i],
				    spininserthandlers[i]);
			}

			if (spindeletehandlers[i] > 0) {
				g_signal_handler_block(
				    (gpointer *)MainWindow.InstallationDiskWindow.partspin[i],
				    spindeletehandlers[i]);
			}
		}
	}
	if (mask & 1<<0)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[0],
		    (gpointer *)partition_0_spinner_value_changed,
		    NULL);

	if (mask & 1<<1)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[1],
		    (gpointer *)partition_1_spinner_value_changed,
		    NULL);
	if (mask & 1<<2)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[2],
		    (gpointer *)partition_2_spinner_value_changed,
		    NULL);
	if (mask & 1<<3)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[3],
		    (gpointer *)partition_3_spinner_value_changed,
		    NULL);
}

static void
disk_partitioning_unblock_spinbox_handlers(gint mask)
{
	gint i  = 0;

	if (mask == 0)
		return;
	for (i = 0; i < FD_NUMPART; i++) {
		if (mask & 1<<i) {
			if (spininserthandlers[i] > 0) {
				g_signal_handler_unblock(
				    (gpointer *)MainWindow.InstallationDiskWindow.partspin[i],
				    spininserthandlers[i]);
			}

			if (spindeletehandlers[i] > 0) {
				g_signal_handler_unblock(
				    (gpointer *)MainWindow.InstallationDiskWindow.partspin[i],
				    spindeletehandlers[i]);
			}
		}
	}
	if (mask & 1<<0)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[0],
		    (gpointer *)partition_0_spinner_value_changed,
		    NULL);
	if (mask & 1<<1)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[1],
		    (gpointer *)partition_1_spinner_value_changed,
		    NULL);
	if (mask & 1<<2)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[2],
		    (gpointer *)partition_2_spinner_value_changed,
		    NULL);
	if (mask & 1<<3)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partspin[3],
		    (gpointer *)partition_3_spinner_value_changed,
		    NULL);
}

static void
disk_partitioning_block_combobox_handlers(gint mask)
{
	if (mask == 0)
		return;
	if (mask & 1<<0)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[0],
		    (gpointer *)partition_0_combo_changed,
		    NULL);
	if (mask & 1<<1)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[1],
		    (gpointer *)partition_1_combo_changed,
		    NULL);
	if (mask & 1<<2)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[2],
		    (gpointer *)partition_2_combo_changed,
		    NULL);
	if (mask & 1<<3)
		g_signal_handlers_block_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[3],
		    (gpointer *)partition_3_combo_changed,
		    NULL);
}

static void
disk_partitioning_unblock_combobox_handlers(gint mask)
{
	if (mask == 0)
		return;

	if (mask & 1<<0)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[0],
		    (gpointer *)partition_0_combo_changed,
		    NULL);
	if (mask & 1<<1)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[1],
		    (gpointer *)partition_1_combo_changed,
		    NULL);
	if (mask & 1<<2)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[2],
		    (gpointer *)partition_2_combo_changed,
		    NULL);
	if (mask & 1<<3)
		g_signal_handlers_unblock_by_func(
		    (gpointer *)MainWindow.InstallationDiskWindow.partcombo[3],
		    (gpointer *)partition_3_combo_changed,
		    NULL);
}

static void disk_partitioning_block_combox_handler(guint partindex)
{
	/* Convenience function to block an individual combox handler */
	gint mask = 1<<partindex;
	disk_partitioning_block_combobox_handlers(mask);
}

static void disk_partitioning_unblock_combox_handler(guint partindex)
{
	/* Convenience function to unblock an individual combox handler */
	gint mask = 1<<partindex;
	disk_partitioning_unblock_combobox_handlers(mask);
}

static void
logical_spinners_insert_text_filter(GtkEntry *widget,
	const gchar *newtext,
	gint length,
	gint *position,
	gpointer user_data)
{
	/*
	 * Validation is the same for both logicals and primarys so simply
	 * call the already defined primary handler.
	 */
	spinners_insert_text_filter(widget, newtext, length, position, user_data);
}

static void
spinners_insert_text_filter(GtkEntry *widget,
	const gchar *newtext,
	gint length,
	gint *position,
	gpointer user_data)
{
	const gchar *currenttext;
	const gchar *textptr;
	gchar *newnumstring = NULL;
	gdouble newnum;
	gdouble min, max;
	gint newstrlen = 0;
	gint decimalplaces = 0;

	currenttext = gtk_entry_get_text(GTK_ENTRY(widget));
	gtk_spin_button_get_range(GTK_SPIN_BUTTON(widget), &min, &max);

	/*
	 * Need to generate newnumstring based on insertion position
	 */
	newstrlen = strlen(currenttext) + length;
	newnumstring = g_new0(gchar, newstrlen + 1);

	strncat(newnumstring, currenttext, *position);
	strncat(newnumstring, newtext, length);
	if (*position < strlen(currenttext)) {
		textptr = &currenttext[*position];
		strcat(newnumstring, textptr);
	}

	/*
	 * Check to make sure there's no more than 1 decimal
	 * place in the new number. Note that the decimal place
	 * character literal is dependant on locale environment,
	 * hence the use of "isdigit()" instead of checking for
	 * a "." or "," character.
	 */
	textptr = newnumstring;
	while (*textptr != '\0') {
		if (!isdigit(*textptr)) {
			decimalplaces = strlen(textptr+1);
			break;
		}
		textptr++;
	}
	newnum = strtod(newnumstring, NULL);

	if (newnum > max || decimalplaces > 1) {
		gdk_beep();
		g_signal_stop_emission_by_name(GTK_OBJECT(widget), "insert_text");
	}

	g_free(newnumstring);
}

static void
logical_spinners_delete_text_filter(GtkEditable *widget,
	gint start_pos,
	gint end_pos,
	gpointer user_data)
{
	/*
	 * Validation is the same for both logicals and primarys so simply
	 * call the already defined primary handler.
	 */
	spinners_delete_text_filter(widget, start_pos, end_pos, user_data);
}

static void
spinners_delete_text_filter(GtkEditable *widget,
	gint start_pos,
	gint end_pos,
	gpointer user_data)
{
	gchar *currenttext;
	const gchar *str1 = NULL, *str2 = NULL;
	gchar *newnumstring = NULL;
	gdouble newnum;
	gdouble min, max;

	currenttext = g_strdup(gtk_entry_get_text(GTK_ENTRY(widget)));
	if (strtod(currenttext, NULL) == 0) {
		g_free(currenttext);
		return;
	}

	gtk_spin_button_get_range(GTK_SPIN_BUTTON(widget), &min, &max);

	/*
	 * Need to generate newstring based on deletion span
	 */
	str1 = currenttext;
	currenttext[start_pos] = '\0';
	str2 = &currenttext[end_pos];
	newnumstring = g_strdup_printf("%s%s", str1, str2);
	newnum = strtod(newnumstring, NULL);

	if (newnum > max) {
		gdk_beep();
		g_signal_stop_emission_by_name(GTK_OBJECT(widget), "delete_text");
	}

	g_free(currenttext);
	g_free(newnumstring);
}

static void
disk_partitioning_set_sensitive(gboolean sensitive)
{
	if (sensitive == FALSE) {
		/* Collapse the custom partitioning controls */
		gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(
		    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
		    "wholediskradio")),
		    TRUE);
	}
	gtk_widget_set_sensitive(
	    glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "partitioningvbox"), sensitive);
}

/* Makes the scrollbar and viewport adjust to follow the focussed button */
static gboolean
disk_partitioning_button_focus_handler(GtkWidget *widget,
	GdkEventFocus *event,
	gpointer user_data)
{
	gint disknum = 0;
	gfloat newvalue = 0.0;
	gfloat buttonval = 0.0;
	gfloat buttonposition = 0.0;
	gdouble value, lower, upper, pagesize;
	gfloat buttonsize;

	disknum = GPOINTER_TO_INT(user_data);

	g_object_get(G_OBJECT(viewportadjustment), "value", &value, NULL);
	g_object_get(G_OBJECT(viewportadjustment), "lower", &lower, NULL);
	g_object_get(G_OBJECT(viewportadjustment), "upper", &upper, NULL);
	g_object_get(G_OBJECT(viewportadjustment), "page-size", &pagesize, NULL);

	/* double precision is too expensive and overkill - use float */
	buttonsize = (gfloat)((upper - lower)/numdisks);
	buttonposition = (gfloat)disknum / numdisks;
	buttonval = buttonposition * (gfloat)(upper - lower);

	/*
	 * Increment scrolling adjustment just enough to keep
	 * the button visible in the viewport
	 */
	if (value+pagesize <= buttonval+buttonsize) {
		newvalue = buttonval + buttonsize - pagesize;
		gtk_adjustment_set_value(viewportadjustment, (gdouble)newvalue);
		gtk_adjustment_value_changed(viewportadjustment);
	} else if (value >= buttonval) {
		newvalue = buttonval;
		gtk_adjustment_set_value(viewportadjustment, (gdouble)newvalue);
		gtk_adjustment_value_changed(viewportadjustment);
	}
	return (FALSE);
}

/* Hides the scrollbar if scrolling is not necessary */
static void
viewport_adjustment_changed(GtkAdjustment *adjustment, gpointer user_data)
{
	GtkWidget *scrollbar;
	gdouble lower, upper, pagesize;

	scrollbar = GTK_WIDGET(user_data);

	g_object_get(G_OBJECT(adjustment), "lower", &lower, NULL);
	g_object_get(G_OBJECT(adjustment), "upper", &upper, NULL);
	g_object_get(G_OBJECT(adjustment), "page-size", &pagesize, NULL);

	if ((upper - lower) <= pagesize)
		gtk_widget_hide(scrollbar);
	else
		gtk_widget_show(scrollbar);
}

static gboolean
disk_partitions_match(disk_parts_t *old, disk_parts_t *new)
{
	gboolean retval = TRUE;
	partition_info_t *parta, *partb;
	guint64 sizea = 0, sizeb = 0;
	gint i = 0;
	g_return_val_if_fail(old, FALSE);
	g_return_val_if_fail(new, FALSE);

	g_debug("Comparing partitioning requisition.....");
	for (i = 0; i < OM_NUMPART; i++) {
		parta = orchestrator_om_get_part_by_blkorder(old, i);
		partb = orchestrator_om_get_part_by_blkorder(new, i);

		if (parta != NULL || partb != NULL) {
			sizea = orchestrator_om_get_partition_sizemb(parta);
			sizeb = orchestrator_om_get_partition_sizemb(partb);
			/* Ignore small differences due to rounding: <= 100MB */
			if ((sizea - sizeb) > 100) {
				retval = FALSE;
				g_warning("Partition %d sizes don't match:", i+1);
			}
			g_debug("Part %d: Requested: %lld Received: %lld",
			    i, sizea, sizeb);
		}
	}
	return (retval);
}

/*
 * Takes a list of paritions, and re-populates them from the
 * modifiedprimaryblkorder and modifiedlogicalblkorder linked lists.
 */
static void
restore_unused_partitions(guint disknum, disk_parts_t *partitions)
{
	static void **last_restored = NULL;
	guint i = 0;

	/*
	 * Remember the partitions pointer here so that we don't regenerate the
	 * black order more than once for the same list of partitions.
	 *
	 * The Orchestrator always returns a new pointer, so if we see something
	 * new we will handle it, otherwise ignore it.
	 */
	if (last_restored == NULL) {
		/* First time, allocate the space for each known disk */
		last_restored = g_new0(void*, numdisks);
	}
	if (((void*)partitions) == last_restored[disknum]) {
		g_debug("Not doing a restore on partitions, already done (%08X)",
		    last_restored[disknum]);
		return;
	}
	last_restored[disknum] = (void*)partitions;

	g_debug("Before attempting to restore partitioning on device %s:",
	    partitions->disk_name ? partitions->disk_name : "NULL");
	for (i = 0; i < OM_NUMPART; i++) {
		g_debug("\tPartition %d: id: %d order: %d type: %d size: %d",
		    i,
		    (int)partitions->pinfo[i].partition_id,
		    (int)partitions->pinfo[i].partition_order,
		    partitions->pinfo[i].partition_type,
		    partitions->pinfo[i].partition_size);
	}

	print_partinfos(disknum, alldiskinfo, &partitions);
	print_blkorder(alldiskinfo[disknum],
	    modifiedprimaryblkorder[disknum],
	    modifiedlogicalblkorder[disknum]);

	/* Recalculate the disk's blockorder layout using new information */
	installationdisk_get_blkorder_layout(
	    alldiskinfo[disknum],
	    partitions,
	    &modifiedprimaryblkorder[disknum],
	    &modifiedlogicalblkorder[disknum]);

	g_debug("After recreating blkorder lists :");
	print_partinfos(disknum, alldiskinfo, &partitions);
	print_blkorder(alldiskinfo[disknum],
	    modifiedprimaryblkorder[disknum],
	    modifiedlogicalblkorder[disknum]);

	initialize_default_partition_layout(disknum);

	/* Force a redraw of the widgets */
	disk_selection_set_active_disk(disknum);

	g_debug("After attempting to restore partitioning on device %s:",
	    partitions->disk_name ? partitions->disk_name : "NULL");
	for (i = 0; i < OM_NUMPART; i++) {
		g_debug("\tPartition %d: id: %d order: %d type: %d size: %d",
		    i,
		    (int)partitions->pinfo[i].partition_id,
		    (int)partitions->pinfo[i].partition_order,
		    partitions->pinfo[i].partition_type,
		    partitions->pinfo[i].partition_size);
	}
	print_partinfos(disknum, alldiskinfo, &partitions);
	print_blkorder(alldiskinfo[disknum],
	    modifiedprimaryblkorder[disknum],
	    modifiedlogicalblkorder[disknum]);
}

static gboolean
installationdisk_partinfo_changed(partition_info_t *partinfo)
{
	partition_info_t *origpartinfo;
	disk_parts_t *partitions = originalpartitions[activedisk];
	gint i = 0;

	/* Check if a given partition has changed in type or size */
	for (i = 0; i < OM_NUMPART; i++) {
		origpartinfo = &partitions->pinfo[i];
		if (origpartinfo->partition_id == partinfo->partition_id) {
			if (origpartinfo->partition_size != partinfo->partition_size ||
			    origpartinfo->partition_type != partinfo->partition_type) {
				return (TRUE);
			}
			break;
		}
	}

	return (FALSE);
}

/*
 * Takes a list of partitons, which are usually sorted by partition_order
 * since that's how it's displayed in the GUI and do the following:
 *
 * - Primary partitions are re-ordered by partition_id to ensure they remain
 *   correctly layed out on the disk - i.e. an unchaged p3 remains as p3
 * - Logicals are first collapsed to remove gaps, but then sorted by
 *   partition_id (This is how fdisk logical partitions are handled by
 *   investigated OS-es).
 *
 */
static void
collapse_partitions(disk_parts_t *partitions)
{
	gint i = 0;
	gint j = 0;
	gint part_order = 0;
	partition_info_t *partition = NULL;
	partition_info_t *tmppartition = NULL;
	partition_info_t *unused_partition = NULL;
	partition_info_t *origpartinfo = NULL;

	g_debug("Before attempting to collapse partitioning on device %s:",
	    partitions->disk_name ? partitions->disk_name : "NULL");
	for (i = 0; i < OM_NUMPART; i++) {
		g_debug("\tPartition %d: id: %d order: %d type: %d size: %d",
		    i,
		    (int)partitions->pinfo[i].partition_id,
		    (int)partitions->pinfo[i].partition_order,
		    partitions->pinfo[i].partition_type,
		    partitions->pinfo[i].partition_size);
	}
	/*
	 * First re-sort the 4 Primary partitions by partition_id.
	 *
	 * The assumption is that the partition_ids are always valid and correct,
	 * this should be maintained by code that allocates spaces to them.
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (partition) {
			if (partition->partition_type == UNUSED) {
				/* Unused space */

				/* Reduce partition_order for all other partitions */
				for (j = 0; j < FD_NUMPART; j++) {
					if (tmppartition = &partitions->pinfo[j]) {
						if (tmppartition->partition_order >
						    partition->partition_order) {
							tmppartition->partition_order -= 1;
						}
					}
				}

				/* Zeroize all array elements for UNUSED */
				partition->partition_id = 0;
				partition->partition_order = 0;
				partition->partition_type = UNUSED;
				partition->partition_size = 0;
				partition->partition_offset = 0;
				partition->partition_size_sec = 0;
				partition->partition_offset_sec = 0;
			}
			/* Skip a zeroed partition_id */
			g_debug("Checking: %d != %d", (partition->partition_id - 1), i);
			if (partition->partition_id > 0 &&
			    (partition->partition_id - 1) != i) {
				/*
				 * If not in equivalent position in the array
				 * then Swap it with whatever is ocupying it's
				 * space.
				 */
				partition_info_t    temp_partition = {0};
				partition_info_t   *correct_partition =
				    &partitions->pinfo[partition->partition_id - 1];

				g_debug("Moving p%d (%d) to p%d(%d)",
				    i,
				    partition->partition_id,
				    partition->partition_id - 1,
				    correct_partition->partition_id);


				/* Copy out what's already there */
				(void) memcpy(&temp_partition, correct_partition,
				    sizeof (partition_info_t));
				/* Fill it with the correct partition id */
				(void) memcpy(correct_partition, partition,
				    sizeof (partition_info_t));
				/* Put old partition id, where we are now */
				(void) memcpy(partition, &temp_partition,
				    sizeof (partition_info_t));
			}
		}
	}

	/* First collapse the logical paritions, removing gaps in partition_order */
	unused_partition = NULL;
	part_order = 4;
	for (i = FD_NUMPART; i < OM_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (partition) {
			/*
			 * If a partitions type or size has changed then we
			 * zeroize offset values as they are meaningless we
			 * do this to ensure the orchestrator's validation
			 * function does not attempt to use these values.
			 */
			if (installationdisk_partinfo_changed(partition)) {
				partition->partition_offset = 0;
				partition->partition_size_sec = 0;
				partition->partition_offset_sec = 0;
			}
			if (partition->partition_type != UNUSED) {
				if (unused_partition) {
					/* Move to unused space (swap) */
					partition_info_t temp_partition = {0};

					partition->partition_order = ++part_order;

					(void) memcpy(&temp_partition, unused_partition,
					    sizeof (partition_info_t));
					(void) memcpy(unused_partition, partition,
					    sizeof (partition_info_t));
					(void) memcpy(partition, &temp_partition,
					    sizeof (partition_info_t));

					unused_partition = partition;
				} else {
					partition->partition_order =
					    ++part_order;
				}
			} else {
				/* Unused space */
				if (unused_partition == NULL) {
					/* move next allocated primary to here */
					unused_partition = partition;
				}
				/* Enusre unused size is 0 */
				partition->partition_size = 0;
				partition->partition_id = 0;
				partition->partition_order = 0;
			}
		}
	}

	/*
	 * Now re-sort the logical partitions by partition_id. This is required to
	 * ensure that they are orders as in the original layout.
	 */
	for (i = FD_NUMPART; i < OM_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (partition) {
			/* Skip a zeroed partition_id */
			if (partition->partition_id != 0 &&
			    partition->partition_id != (i+1)) {
				/*
				 * If not in equivalent position in the array then
				 * Swap it with whatever is ocupying it's space
				 */
				partition_info_t    temp_partition = {0};
				partition_info_t   *other_partition =
				    &partitions->pinfo[partition->partition_id - 1];

				(void) memcpy(&temp_partition, other_partition,
				    sizeof (partition_info_t));
				(void) memcpy(other_partition, partition,
				    sizeof (partition_info_t));
				(void) memcpy(partition, &temp_partition,
				    sizeof (partition_info_t));
			}
		}
	}

	g_debug("After attempting to collapse partitioning on device %s:",
	    partitions->disk_name ? partitions->disk_name : "NULL");
	for (i = 0; i < OM_NUMPART; i++) {
		g_debug("\tPartition %d: id: %d order: %d type: %d size: %d",
		    i,
		    (int)partitions->pinfo[i].partition_id,
		    (int)partitions->pinfo[i].partition_order,
		    partitions->pinfo[i].partition_type,
		    partitions->pinfo[i].partition_size);
	}
}

gboolean
installationdisk_validate()
{
	disk_parts_t *partitions = NULL;
	disk_parts_t *newpartitions = NULL;
	partition_info_t *partition = NULL;
	gchar *errorprimarytext = NULL;
	gchar *errorsecondarytext = NULL;
	gchar *warningprimarytext = NULL;
	gchar *warningsecondarytext = NULL;
	gint i = 0;
	gint numpartitions = 0;
	gfloat solarispartitionsize = 0;
	gint64 freespace = 0;
	guint64 diskusage = 0;
	guint64 diskcapacity = 0;
	guint64 extended_part_size = 0;
	guint64 logical_diskusage = 0;
	gint64 logical_freespace = 0;
	gboolean partitionsmatch = FALSE;
	gboolean prompt_retval = FALSE;
	GtkToggleButton *usewholediskradio;

	/* 1. No disk selected */
	if (activedisk < 0) {
		errorprimarytext = g_strdup(
		    _("No disk has been selected for OpenIndiana installation."));
		errorsecondarytext =
		    g_strdup(_("Select a disk."));
		goto errors;
	}

	/* 2. No suitable disk selected */
	/* Only condition I can think of is disk too small */
	diskcapacity = orchestrator_om_get_disk_sizemb(alldiskinfo[activedisk]);
	if (diskcapacity < orchestrator_om_get_mininstall_sizemb()) {
		errorprimarytext = g_strdup(
		    _("The selected disk is not suitable for OpenIndiana installation."));
		errorsecondarytext =
		    g_strdup(_("Select another disk."));
		goto errors;
	}

/* Partitioning related errors are not applicable to SPARC - yet */
#if defined(__i386)

	g_assert(proposedpartitions != NULL);
	g_assert(proposedpartitions[activedisk] != NULL);
	partitions = proposedpartitions[activedisk];
	/* 3. No Solaris partitions defined */
	numpartitions =
	    orchestrator_om_get_numparts_of_type(partitions, SUNIXOS2);
	numpartitions +=
	    orchestrator_om_get_numparts_of_type(partitions, SUNIXOS);

	for (i = 0; i < OM_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (partition && partition->partition_type != UNUSED) {
			/* Only count if it's not partition_type Unused */

			if (IS_EXT_PAR(partition->partition_type)) {
				/* Remember size of extended partition if found */
				extended_part_size =
				    orchestrator_om_get_partition_sizemb(partition);
			}

			if (i < FD_NUMPART) { /* Primary partition */
				diskusage +=
				    orchestrator_om_get_partition_sizemb(partition);
			} else { /* Locical parition */
				logical_diskusage +=
				    orchestrator_om_get_partition_sizemb(partition);
			}
		}
	}
	freespace = diskcapacity - diskusage;
	logical_freespace = extended_part_size - logical_diskusage;

	if (numpartitions == 0) {
		errorprimarytext =
		    g_strdup(
		    _("The selected disk contains no Solaris partitions."));
		errorsecondarytext =
		    g_strdup(
		    _("Create one Solaris partition or use the whole disk."));

		goto errors;

	/* 4. Must be only one Solaris partition */
	} else if (numpartitions > 1) {
		errorprimarytext =
		    g_strdup(_("There must be only one Solaris partition."));
		errorsecondarytext =
		    g_strdup(_("Change the extra Solaris partitions to another type."));
		goto errors;

	/* 5. Disk space over allocated */
	} else if ((freespace < -(MBPERGB / 10)) ||
	    (logical_freespace < -(MBPERGB / 10))) {
		errorprimarytext =
		    g_strdup(_("The disk space has been over allocated."));
		errorsecondarytext =
		    g_strdup(_("Reduce the size of one or more partitions "
		    "until the available disk space is zero."));
		goto errors;
	}
	/* 6. Check if the Solaris partition is too small */
	/* Find the first Solaris partition, should be the only one at this stage */
	for (i = 0; i < OM_NUMPART; i++) {
		partition =
		    orchestrator_om_get_part_by_blkorder(partitions, i);
		if (IS_SOLARIS_PAR(orchestrator_om_get_partition_type(partition),
		    partition->content_type)) {
			solarispartitionsize =
			    ONE_DECIMAL(orchestrator_om_get_partition_sizegb(partition));
			break;
		}
	}
	if (solarispartitionsize <
	    orchestrator_om_get_mininstall_sizegb(FALSE)) {
		errorprimarytext =
		    g_strdup(_("The Solaris partition is too "
		    "small for installation."));
		errorsecondarytext =
		    g_strdup(_("Increase the size of the Solaris partition."));
	}

#endif /* (__i386) */

errors:
	if (errorprimarytext != NULL) {
		gui_install_prompt_dialog(FALSE, FALSE, FALSE,
		    GTK_MESSAGE_ERROR,
		    errorprimarytext,
		    errorsecondarytext);
		g_free(errorprimarytext);
		g_free(errorsecondarytext);
		return (FALSE);
	}

	/* Non fatal test for disk size < recommended */
	/* Only display this warning if entire disk is being used */
	usewholediskradio = GTK_TOGGLE_BUTTON
	    (glade_xml_get_widget(MainWindow.installationdiskwindowxml,
	    "wholediskradio"));
	if (gtk_toggle_button_get_active(usewholediskradio) == TRUE &&
	    diskcapacity >= orchestrator_om_get_mininstall_sizemb() &&
	    diskcapacity < orchestrator_om_get_recommended_sizemb()) {
		warningprimarytext = g_strdup(
		    _("The selected disk is smaller than the recommended "
		    "minimum size."));
		warningsecondarytext =
		    g_strdup(_("You may have difficulties upgrading the system "
		    "software and/or installing and running additional "
		    "applications."));
		goto warnings;
	}

/* Partitioning related errors are not applicable to SPARC - yet */
#if defined(__i386)
	/* Non fatal test for solaris partition size < recommended */
	/* Perform test regardless of using whole disk or not */
	if (solarispartitionsize >=
	    orchestrator_om_get_mininstall_sizegb(FALSE) &&
	    solarispartitionsize <
	    orchestrator_om_get_recommended_sizegb()) {
		warningprimarytext = g_strdup(
		    _("The selected partition is smaller than the recommended "
		    "minimum size."));
		warningsecondarytext =
		    g_strdup(_("You may have difficulties upgrading the "
		    "system software and/or installing and running "
		    "additional applications."));
		goto warnings;
	}
#endif

warnings:
	if (warningprimarytext != NULL) {
		prompt_retval = gui_install_prompt_dialog(TRUE, FALSE, FALSE,
		    GTK_MESSAGE_WARNING,
		    warningprimarytext,
		    warningsecondarytext);
		g_free(warningprimarytext);
		g_free(warningsecondarytext);
		warningprimarytext = NULL;
		warningsecondarytext = NULL;
		if (prompt_retval == FALSE) {
			return (FALSE);
		}
	}

	/* Now check for non-fatal warning conditions */
	/* For X86 - unallocated disk space */
#if defined(__i386)
	g_debug("Original partitioning on device %s:",
	    partitions->disk_name ? partitions->disk_name : "NULL");
	for (i = 0; i < OM_NUMPART; i++) {
		g_debug("\tPartition %d: type: %d size: %d",
		    i,
		    originalpartitions[activedisk]->pinfo[i].partition_type,
		    originalpartitions[activedisk]->pinfo[i].partition_size);
	}
	g_debug("Attempting to set partitioning on device %s:",
	    partitions->disk_name ? partitions->disk_name : "NULL");
	for (i = 0; i < OM_NUMPART; i++) {
		g_debug("\tPartition %d: type: %d size: %d",
		    i, partitions->pinfo[i].partition_type,
		    partitions->pinfo[i].partition_size);
	}

	collapse_partitions(partitions);

	g_debug("Partinfos before om_validate");
	print_partinfos(activedisk, alldiskinfo, modifiedpartitions);

	newpartitions = om_validate_and_resize_disk_partitions(omhandle,
	    partitions, GUI_allocation);

	/* new partitions will be positioned automatically */
	if (newpartitions == NULL) {
		gchar *warningcode = NULL;
		int16_t error;

		g_warning("Orchestrator not happy with partitioning");
		error = om_get_error();
		switch (error) {
			case OM_UNSUPPORTED_CONFIG:
				warningcode =
				    g_strdup("OM_UNSUPPORTED_CONFIG");
				break;
			case OM_NO_DISKS_FOUND:
				warningcode =
				    g_strdup("OM_NO_DISKS_FOUND");
				break;
			case OM_NO_SPACE:
				warningcode =
				    g_strdup("OM_NO_SPACE");
				break;
			case OM_INVALID_DISK_PARTITION:
				warningcode =
				    g_strdup("OM_INVALID_DISK_PARTITION");
				break;
			case OM_FORMAT_UNKNOWN:
				warningsecondarytext =
				    g_strdup("OM_FORMAT_UNKNOWN");
				break;
			case OM_BAD_DISK_NAME:
				warningcode =
				    g_strdup("OM_BAD_DISK_NAME");
				break;
			case OM_CONFIG_EXCEED_DISK_SIZE:
				warningcode =
				    g_strdup("OM_CONFIG_EXCEED_DISK_SIZE");
				break;
			default:
				warningcode =
				    g_strdup(
				    _("An unknown internal error (Orchestrator) occurred."));
				break;
		}

		g_warning("om_validate_and_resize_disk_partitions () failed.");
		g_warning("\tReason: %s", warningcode);
		if (error == OM_UNSUPPORTED_CONFIG) {
			/* Create a specific error message */
			errorprimarytext =
			    g_strdup(_("Unsupported partitioning configuration."));
			errorsecondarytext =
			    g_strdup(_("OpenIndiana does not support changing the "
			    "partition type when two or more of that "
			    "type exist on the disk. Please Quit the "
			    "installer, run fdisk in the terminal window "
			    "to create the Solaris partition, then restart "
			    "the installer."));
		} else {
			/* Create a generic error message */
			errorprimarytext =
			    g_strdup(_("Internal partitioning error."));
			errorsecondarytext =
			    g_strdup_printf(_("Error code: %s\nThis is an unexpected, "
			    "internal error. It is not safe to continue with "
			    "installation of this system and you should quit the "
			    "installation process now."),
			    warningcode);
		}
		g_free(warningcode);
	} else {
		if (partitions == modifiedpartitions[activedisk]) {
			print_orig_vs_modified(alldiskinfo[activedisk],
			    originalpartitions[activedisk],
			    modifiedpartitions[activedisk]);
			/*
			 * If the user didn't use the default partitioning
			 * layout, update the display if necessary to
			 * reflect actual partitioning.
			 */
			modifiedpartitions[activedisk] = newpartitions;
			proposedpartitions[activedisk] =
			    modifiedpartitions[activedisk];

			partitionsmatch =
			    disk_partitions_match(partitions, newpartitions);
			om_free_disk_partition_info(omhandle, partitions);

			g_debug("Proposed partitions, after adjustment by OM:");
			print_partinfos(activedisk, alldiskinfo, modifiedpartitions);

			if (!partitionsmatch) {
				warningprimarytext =
				    g_strdup(_("Adjustments were made to the size of "
				    "some new or resized partitions."));
				warningsecondarytext =
				    g_strdup(_("The requested partitioning would require "
				    "existing partitions to be moved. \n\n"
				    "Click cancel to review the adjustments. "));
			}
		} else if (partitions == defaultpartitions[activedisk]) {
			/*
			 * Even though the default layout shouldn't need
			 * any corrections from the validate_and_resize
			 * function, it can happen, probably because of
			 * rounding errors mapping megabytes to disk blocks
			 * etc.  So we need to overwrite the defaultlayout
			 * we created for the disk and replace it with what
			 * the orchestrator gave us back. But don't
			 * display this to the user or it will look stupid.
			 */
			defaultpartitions[activedisk] = newpartitions;
			proposedpartitions[activedisk] =
			    defaultpartitions[activedisk];
			om_free_disk_partition_info(omhandle, partitions);
		}
	}
#endif /* (__i386) */
	/* Nothing else right now */
	if (errorprimarytext != NULL) {
		gui_install_prompt_dialog(FALSE, FALSE, FALSE,
		    GTK_MESSAGE_ERROR,
		    errorprimarytext,
		    errorsecondarytext);
		g_free(errorprimarytext);
		g_free(errorsecondarytext);
		return (FALSE);
	}

	if (warningprimarytext != NULL) {
		prompt_retval = gui_install_prompt_dialog(TRUE, FALSE, FALSE,
		    GTK_MESSAGE_WARNING,
		    warningprimarytext,
		    warningsecondarytext);
		g_free(warningprimarytext);
		g_free(warningsecondarytext);
		if (prompt_retval == FALSE) {
			/*
			 * Need to reevaluate the paritions and gaps, so do
			 * the same as if the Back button is pressed from the
			 * Timezone screen.
			 */
			installationdisk_screen_set_default_focus(TRUE);

			g_debug("Cancel selected reviewing proposed layout :");
			print_partinfos(activedisk, alldiskinfo, modifiedpartitions);
			print_blkorder(alldiskinfo[activedisk],
			    modifiedprimaryblkorder[activedisk],
			    modifiedlogicalblkorder[activedisk]);
			print_gui(MainWindow.InstallationDiskWindow);
			return (FALSE);
		}
	}

	return (TRUE);
}

void
installation_disk_store_data()
{
	disk_parts_t *partitions = NULL;
	const gchar *diskname = (alldiskinfo[activedisk]->disk_name);
	partition_info_t *partition = NULL;
	gint i = 0;
	int status = 0;

	partitions = proposedpartitions[activedisk];
	InstallationProfile.diskname = diskname;
	InstallationProfile.disktype =
	    orchestrator_om_get_disk_type(alldiskinfo[activedisk]);
	InstallationProfile.disksize =
	    orchestrator_om_get_disk_sizegb(alldiskinfo[activedisk]);
#if defined(__i386)
	for (i = 0; i < OM_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (IS_SOLARIS_PAR(partition->partition_type,
		    partition->content_type)) {
			InstallationProfile.installpartsize =
			    orchestrator_om_get_partition_sizegb(partition);
			break;
		}
	}

	/*
	 * Tell orchestrator to use this partitioning layout for install
	 * Only gets applied after om_perform_install() is called so not
	 * too late yet.
	 */
	status = om_set_disk_partition_info(omhandle,
	    proposedpartitions[activedisk]);
	if (status != OM_SUCCESS) {
		status = om_get_error();
		/*
		 * If setting the partition info failed, things are
		 * screwed but this is unlikely since the partition
		 * data was already validated with any necessary
		 * adjustments already made by the validation call
		 */
		g_critical("Failed to set proposed partitioning layout");
		g_critical("Error code is: %d", status);
	}
#endif
}

static disk_parts_t *
installation_disk_create_default_layout(disk_info_t *diskinfo)
{
	disk_parts_t *partitions;
	partition_info_t *partinfo;
	gint i = 0;

	partitions = g_new0(disk_parts_t, 1);
	partitions->disk_name = g_strdup(diskinfo->disk_name);
	/*
	 * New suggested partition layout:
	 * partition   0:  type = Solaris, size = diskcapacity, active = TRUE
	 * partition 1-3:  type = Unused,  size = 0, active = FALSE
	 */
	for (i = 0; i < FD_NUMPART; i++) {
		partinfo = &partitions->pinfo[i];
		partinfo->partition_id = i+1;
		partinfo->partition_order = i+1;
		partinfo->partition_offset = 0;
		partinfo->content_type = OM_CTYPE_UNKNOWN;
		if (i == 0) {
			partinfo->partition_type = SUNIXOS2;
			partinfo->partition_size = (uint64_t)
			    orchestrator_om_get_disk_sizemb(diskinfo);
			partinfo->active = B_TRUE;
		} else {
			partinfo->partition_type = UNUSED;
			partinfo->partition_size = 0;
			partinfo->active = B_FALSE;
		}
	}

	/* Initialize all 32 possible logicals */
	for (i = FD_NUMPART; i < OM_NUMPART; i++) {
		partinfo = &partitions->pinfo[i];
		partinfo->partition_id = 0;
		partinfo->partition_size = 0;
		partinfo->partition_offset = 0;
		partinfo->partition_order = 0;
		partinfo->partition_type = UNUSED;
		partinfo->content_type = OM_CTYPE_UNKNOWN;
		partinfo->active = B_FALSE;
		partinfo->partition_size_sec = 0;
		partinfo->partition_offset_sec = 0;
	}
	return (partitions);
}

static LogicalPartition *
get_logical_partition_at_pos(gint index,
	LogicalPartition *startlogical)
{
	LogicalPartition *curlogical = NULL;
	gint logicalorder = 0;

	for (curlogical = startlogical;
	    curlogical != NULL;
	    curlogical = curlogical->next) {
		logicalorder++;
		if (logicalorder == index) {
			return (curlogical);
		}
	}
	return (NULL);
}
static void
update_logical_disk_partitions_from_ui(disk_parts_t *partitions)
{
	gint pidx = 0;
	gint lidx = 0;
	partition_info_t *primpartinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	gint primparttype;
	LogicalPartition *curlogical = NULL;
	LogicalPartition *startlogical = NULL;
	GtkSpinButton *logspinner = NULL;
	GtkComboBox *logcombo = NULL;
	GtkLabel *logavaillabel = NULL;
	gfloat size = 0;
	const gchar *availtext;
	gint comboindex = 0;

	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		primpartinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, pidx);
		g_assert(primpartinfo);

		primparttype = orchestrator_om_get_partition_type(primpartinfo);

		if (IS_EXT_PAR(primparttype) &&
		    MainWindow.InstallationDiskWindow.startlogical[pidx] != NULL) {

			/* If this primary is Extended, update it's logicals */
			startlogical =
			    MainWindow.InstallationDiskWindow.startlogical[pidx];
			for (lidx = FD_NUMPART;
			    lidx < OM_NUMPART;
			    lidx++) {
				logpartinfo =
				    orchestrator_om_get_part_by_blkorder(
				    partitions,
				    lidx);

				if (logpartinfo) {
					/* Get LogicalPartition struct from linked list */
					curlogical = get_logical_partition_at_pos(
					    (lidx+1)-FD_NUMPART,
					    startlogical);

					/* If no curlogical or hasn't changed try next one */
					if ((!curlogical) ||
					    (curlogical &&
					    (!curlogical->typechange &&
					    !curlogical->sizechange)))
						continue;

					logcombo = GTK_COMBO_BOX(curlogical->typecombo);
					logspinner = GTK_SPIN_BUTTON(curlogical->sizespinner);
					logavaillabel = GTK_LABEL(curlogical->availlabel);

					size = 0;
					comboindex =
					    gtk_combo_box_get_active(logcombo);

					/*
					 * For Unused partitions as spin button
					 * has been reset to 0 so use the
					 * partition size itself.
					 */
					if (comboindex != UNUSED_PARTITION) {
						size = (gfloat)
						    gtk_spin_button_get_value(logspinner);
					} else {
						size = orchestrator_om_round_mbtogb(
						    logpartinfo->partition_size);
					}

					if (curlogical->sizechange == TRUE) {
						orchestrator_om_set_partition_sizegb(
						    logpartinfo, size);
					}

					if (curlogical->typechange == TRUE) {
						switch (comboindex) {
							case UNUSED_PARTITION:
								logpartinfo->partition_type = UNUSED;
								break;
							case SOLARIS_PARTITION:
								logpartinfo->partition_type = SUNIXOS2;
								break;
							default:
								g_warning(
								    "Logical partition %d type is invalid",
								    lidx+1);
								break;
						}
					}
					update_blkorder_from_partinfo(
					    modifiedlogicalblkorder[activedisk],
					    logpartinfo);
				} else {
					/* No More logicals */
					break;
				}
			}
		}
	}
}

/*
 * Updates partition structure based on GUI's partitioning controls
 * Skips over partitions that haven't been modified so as not to
 * bork an unsupported partition type or the precise size of the
 * partition.
 */
static void
update_disk_partitions_from_ui(disk_parts_t *partitions)
{
	GtkSpinButton *primspinner = NULL;
	GtkComboBox *primcombo = NULL;
	GtkLabel *primavaillabel = NULL;
	partition_info_t *primpartinfo = NULL;
	gint pidx = 0;
	const gchar *availtext;
	gint comboindex = 0;
	gint primparttype;
	gfloat size = 0;

	g_return_if_fail(partitions);

	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		primpartinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, pidx);
		g_assert(primpartinfo);

		primparttype = orchestrator_om_get_partition_type(primpartinfo);
		primcombo = GTK_COMBO_BOX
		    (MainWindow.InstallationDiskWindow.partcombo[pidx]);
		primspinner = GTK_SPIN_BUTTON
		    (MainWindow.InstallationDiskWindow.partspin[pidx]);
		primavaillabel = GTK_LABEL
		    (MainWindow.InstallationDiskWindow.partavail[pidx]);

		if ((!activediskisreadable) ||
		    (MainWindow.InstallationDiskWindow.parttypechanges[pidx] == TRUE) ||
		    (MainWindow.InstallationDiskWindow.partsizechanges[pidx] == TRUE)) {
			size = 0;
			comboindex = gtk_combo_box_get_active(primcombo);

			/*
			 * For Unused partitions as spin button has been reset
			 * to 0 therefore we must use the actual partition size.
			 */
			if (comboindex != UNUSED_PARTITION) {
				size = (gfloat)
				    gtk_spin_button_get_value(primspinner);
			} else {
				size = orchestrator_om_round_mbtogb(
				    primpartinfo->partition_size);
			}

			if ((!activediskisreadable) ||
			    (MainWindow.InstallationDiskWindow.partsizechanges[pidx] ==
			    TRUE)) {
				orchestrator_om_set_partition_sizegb(
				    primpartinfo, size);
			}

			if ((!activediskisreadable) ||
			    (MainWindow.InstallationDiskWindow.parttypechanges[pidx] ==
			    TRUE)) {
				switch (comboindex) {
					case UNUSED_PARTITION:
						primpartinfo->partition_type = UNUSED;
						break;
					case SOLARIS_PARTITION:
						primpartinfo->partition_type = SUNIXOS2;
						break;
					case EXTENDED_PARTITION:
						primpartinfo->partition_type = EXTDOS;
						break;
					default:
						g_warning("Partition %d type is invalid", pidx+1);
						break;
				}
			}
			update_blkorder_from_partinfo(
			    modifiedprimaryblkorder[activedisk], primpartinfo);
			update_logical_disk_partitions_from_ui(partitions);
		} else {
			if (IS_EXT_PAR(primparttype)) {
				update_logical_disk_partitions_from_ui(
				    partitions);
			}
		}
	}
}

/*
 * Set the default widget with focus.
 * When activedisk is not set, the default
 * widget for installation disk screen is
 * the 1st bootable disk button, or the 1st
 * usable disk button, or the 1st available
 * disk button. Otherwise activedisk is the
 * default.
 */
void
installationdisk_screen_set_default_focus(gboolean back_button)
{
	if (activedisk < 0)
		activedisk = get_default_disk_index();
	if (activedisk >= 0) {
		gtk_widget_grab_focus(diskbuttons[activedisk]);
		if (back_button &&
		    proposedpartitions[activedisk] !=
		    defaultpartitions[activedisk]) {
			restore_unused_partitions(activedisk,
			    proposedpartitions[activedisk]);
		}
	}
	if (MainWindow.MileStoneComplete[OM_UPGRADE_TARGET_DISCOVERY] == TRUE &&
	    get_default_disk_index() < 0) {
		gtk_widget_set_sensitive(MainWindow.nextbutton, FALSE);
	}

	update_data_loss_warnings();
}
