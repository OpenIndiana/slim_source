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


#include <unistd.h>
#include <stdio.h>
#include <config.h>
#include "orchestrator-wrappers.h"
#include "installation-disk-screen.h"
#include "disk-block-order.h"
#include "error-logging.h"

#define	PARTINFO_NOT_EQ(x, y) \
	((x)->partition_id != (y)->partition_id || \
	(x)->partition_size != (y)->partition_size || \
	(x)->partition_offset != (y)->partition_offset || \
	(x)->partition_order != (y)->partition_order || \
	(x)->partition_type != (y)->partition_type || \
	(x)->content_type != (y)->content_type || \
	(x)->active != (y)->active || \
	(x)->partition_size_sec != (y)->partition_size_sec || \
	(x)->partition_offset_sec != (y)->partition_offset_sec)

#define	PRINT_PARTINFO(str, index, partinfo, diff) \
	g_debug("%4s : %3d %2u %6u %6u %5u %12s %7s %5d %10llu %10llu %s", \
	    str, \
	    index, \
	    (partinfo)->partition_id, \
	    (partinfo)->partition_offset, \
	    (partinfo)->partition_size, \
	    (partinfo)->partition_order, \
	    installationdisk_parttype_to_string((partinfo)), \
	    content_type_to_string((partinfo)->content_type), \
	    (partinfo)->active, \
	    (partinfo)->partition_offset_sec, \
	    (partinfo)->partition_size_sec, \
	    ((diff)?"*":" "))

void
gui_error_logging_handler(
	const gchar *log_domain,
	GLogLevelFlags log_level,
	const gchar *message,
	gpointer user_data)
{
	ls_dbglvl_t level;
	gchar * domain;

	if (log_domain)
		domain = g_strdup_printf("GUI:%s", log_domain);
	else
		domain = g_strdup("GUI");

	level = G_LOG_LEVEL_MASK & log_level;
	/*
	 * Map glib logging levels to comparable liblogsvc levels.
	 * G_LOG_LEVEL_ERROR is the highest error condition causing
	 * an abort() so it needs to be mapped to LS_DBGLVL_EMERG
	 * instead of LS_DBGLVL_ERR which is non fatal.
	 */
	switch (level) {
		case G_LOG_LEVEL_ERROR:
			level = LS_DBGLVL_EMERG;
			break;
		case G_LOG_LEVEL_CRITICAL:
			level = LS_DBGLVL_ERR;
			break;
		case G_LOG_LEVEL_WARNING:
			level = LS_DBGLVL_WARN;
			break;
		case G_LOG_LEVEL_MESSAGE:
			level = LS_DBGLVL_INFO;
			break;
		case G_LOG_LEVEL_INFO:
			level = LS_DBGLVL_INFO;
			break;
		case G_LOG_LEVEL_DEBUG:
			level = LS_DBGLVL_INFO;
			break;
		default:
			level = LS_DBGLVL_NONE;
			break;
	}

	ls_write_dbg_message(domain, level, "%s\n", message);
	g_free(domain);
}

void
gui_error_logging_init(gchar *name)
{
	ls_init(NULL);
	g_log_set_default_handler(
	    gui_error_logging_handler,
	    (gpointer)name);
}

/* Debug printing functions */
static gchar *
content_type_to_string(om_content_type_t content_type)
{
	gchar *type;

	switch (content_type) {
		case OM_CTYPE_UNKNOWN :
			type = "UNKNOWN";
			break;
		case OM_CTYPE_SOLARIS :
			type = "SOLARIS";
			break;
		case OM_CTYPE_LINUXSWAP :
			type = "LINUXSWAP";
			break;
		case OM_CTYPE_LINUX :
			type = "LINUX";
			break;
		deafult :
			type = "Undefined";
			break;
	}

	return (type);
}

void
print_from_parts(gboolean header_only,
	const gchar *parttype,
	gint partindex,
	partition_info_t *partinfo,
	gfloat partsize,
	GtkSpinButton *spinner,
	gfloat avail_space)
{
	gdouble spinvalue;

	if (header_only) {
		g_debug("");
		g_debug(" %8s : %3s : %7s : %7s : %7s : %7s",
		    "PartType", "Idx", "Size MB", "Size GB", "SpinVal", "Avail");
		g_debug(" %8s : %3s : %7s : %7s : %7s : %7s",
		    "========", "===", "=======", "=======", "=======", "========");
	} else {
		spinvalue = gtk_spin_button_get_value(spinner);
		g_debug(" %8s : %3d : %7lld : %7.2f : %7.2lf : %7.2f",
		    parttype != NULL ? parttype : "",
		    partindex,
		    orchestrator_om_get_partition_sizemb(partinfo),
		    partsize,
		    spinvalue,
		    avail_space);
	}
}

void
print_partinfo(gint index,
	partition_info_t *partinfo,
	gboolean header)
{
	if (header) {
		g_debug("%3s %2s %6s %6s %5s %12s %7s %6s %10s %10s",
		    "idx", "id", "size", "offset", "order", "type", "content",
		    "active", "size_sec", "offset_sec");
		g_debug("%3s %2s %6s %6s %5s %12s %7s %6s %10s %10s",
		    "===",
		    "==",
		    "======",
		    "======",
		    "=====",
		    "============",
		    "=======",
		    "======",
		    "==========",
		    "==========");
	}

	if (!partinfo)
		return;

	g_debug("%3d %2u %6u %6u %5u %12s %7s %6d %10llu %10llu",
	    index,
	    partinfo->partition_id,
	    partinfo->partition_size,
	    partinfo->partition_offset,
	    partinfo->partition_order,
	    installationdisk_parttype_to_string(partinfo) != NULL ?
	    installationdisk_parttype_to_string(partinfo) : "",
	    content_type_to_string(partinfo->content_type) != NULL ?
	    content_type_to_string(partinfo->content_type) : "",
	    partinfo->active,
	    partinfo->partition_size_sec,
	    partinfo->partition_offset_sec);
}

void
print_partinfos(gint activedisk,
	disk_info_t **alldiskinfo,
	disk_parts_t **modpartitions)
{
	gint numparts = 0;
	gint i = 0;
	partition_info_t *partition = NULL;
	gboolean header = TRUE;
	disk_parts_t *partitions = NULL;
	gint localactivedisk = 0;

	if (activedisk < 0) {
		localactivedisk = 0;
	} else {
		localactivedisk = activedisk;
	}
	partitions = modpartitions[localactivedisk];
	numparts = orchestrator_om_get_max_partition_id(
	    modpartitions[localactivedisk]);

	g_debug("");
	g_debug("Disk Name : %s",
	    alldiskinfo[localactivedisk]->disk_name != NULL ?
	    alldiskinfo[localactivedisk]->disk_name : "");
	g_debug("");
	g_debug("Primary Partitions :");
	for (i = 0; i < FD_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (partition->partition_id > 0 ||
		    partition->partition_order > 0) {
			print_partinfo(i, &partitions->pinfo[i], header);
			header = FALSE;
		}
	}

	header = TRUE;
	g_debug("");
	g_debug("Logical Partitions : ");
	for (i = FD_NUMPART; i < OM_NUMPART; i++) {
		partition = &partitions->pinfo[i];
		if (partition->partition_id > 0 ||
		    partition->partition_order > 0) {
			print_partinfo(i, &partitions->pinfo[i], header);
			header = FALSE;
		}
	}
	g_debug("\n");
}

void
print_orig_vs_modified(disk_info_t *diskinfo,
	disk_parts_t *origpartitions,
	disk_parts_t *modpartitions)
{
	gint num_diffs = 0;
	gint orig_numparts = orchestrator_om_get_max_partition_id(
	    origpartitions);
	gint mod_numparts = orchestrator_om_get_max_partition_id(
	    modpartitions);
	gint i = 0;
	partition_info_t *partition = NULL;
	gboolean diff = FALSE;
	disk_parts_t *orig_partitions = origpartitions;
	disk_parts_t *mod_partitions = modpartitions;

	g_debug("Comparing Orig to Modified Partitions : (only changes output)");
	g_debug("Disk Name : %s", diskinfo->disk_name);
	g_debug("Primary Partitions :");

	g_debug("%6s %3s %2s %6s %6s %5s %12s %7s %5s %10s %10s",
	    "src", "idx", "id", "offset", "size", "order", "type", "content",
	    "active", "offset_sec", "size_sec");
	g_debug("%6s %3s %2s %6s %6s %5s %12s %7s %5s %10s %10s",
	    "======",
	    "===",
	    "==",
	    "======",
	    "======",
	    "=====",
	    "============",
	    "=======",
	    "=====",
	    "==========",
	    "==========");

	for (i = 0; i < FD_NUMPART; i++) {
		if (PARTINFO_NOT_EQ(&orig_partitions->pinfo[i],
		    &mod_partitions->pinfo[i])) {
			num_diffs++;
			diff = TRUE;
		}
		PRINT_PARTINFO("ORIG", i, &orig_partitions->pinfo[i], diff);
		PRINT_PARTINFO("MOD ", i, &mod_partitions->pinfo[i], diff);
		g_debug("");
		diff = FALSE;
	}

	g_debug("Logical Partitions : ");
	for (i = FD_NUMPART; i < OM_NUMPART; i++) {
		if (PARTINFO_NOT_EQ(&orig_partitions->pinfo[i],
		    &mod_partitions->pinfo[i])) {
			num_diffs++;
			diff = TRUE;
		}
		PRINT_PARTINFO("ORIG", i, &orig_partitions->pinfo[i], diff);
		PRINT_PARTINFO("MOD ", i, &mod_partitions->pinfo[i], diff);
		g_debug("");
		diff = FALSE;
	}
	g_debug("Compare DONE (%d diffs found)\n", num_diffs);
}

void
print_combo_box_number_of_items(GtkComboBox *combo)
{
	GtkTreeModel *combotree = gtk_combo_box_get_model(combo);
	gint n_children = 0;

	n_children = gtk_tree_model_iter_n_children(combotree, NULL);
	g_debug("Number of children : %d\n", n_children);
}

void
print_blkorder(disk_info_t *diskinfo,
	DiskBlockOrder *primary,
	DiskBlockOrder *logical)
{
	DiskBlockOrder *cur;

	g_debug("");
	g_debug("Disk : %s", diskinfo->disk_name != NULL ?
	    diskinfo->disk_name : "");
	g_debug("  Size : %d", diskinfo->disk_size);
	g_debug("  SecSize : %d", diskinfo->disk_size_sec);

	if (primary != NULL) {
		g_debug("  Primary Partitions Block Order :");
		g_debug("    %2s %5s %6s %12s %10s %10s %10s %10s",
		    "Id", "Order", "Disply", "Type", "Size",
		    "Offset", "SecSize", "SecOffset");
		g_debug("    %2s %5s %6s %12s %10s %10s %10s %10s",
		    "==", "=====", "======", "============", "==========",
		    "==========", "==========", "==========");
		for (cur = primary; cur != NULL; cur = cur->next) {
			g_debug("   %2u %2u %6s %12s %10u %10u %10llu %10llu",
			    cur->partinfo.partition_id,
			    cur->partinfo.partition_order,
			    cur->displayed == TRUE ? "TRUE" : "FALSE",
			    installationdisk_parttype_to_string(&cur->partinfo) != NULL ?
			    installationdisk_parttype_to_string(&cur->partinfo) : "",
			    cur->partinfo.partition_size,
			    cur->partinfo.partition_offset,
			    cur->partinfo.partition_size_sec,
			    cur->partinfo.partition_offset_sec);
		}
	}

	if (logical != NULL) {
		g_debug("  Logical Partitions Block Order :");
		g_debug("    %2s %5s %6s %12s %10s %10s %10s %10s",
		    "Id", "Order", "Unused", "Type", "Size",
		    "Offset", "SecSize", "SecOffset");
		g_debug("    %2s %5s %6s %12s %10s %10s %10s %10s",
		    "==", "=====", "======", "============", "==========",
		    "==========", "==========", "==========");
		for (cur = logical; cur != NULL; cur = cur->next) {
			g_debug("   %2u %2u %6s %12s %10u %10u %10llu %10llu",
			    cur->partinfo.partition_id,
			    cur->partinfo.partition_order,
			    cur->displayed == TRUE ? "TRUE" : "FALSE",
			    installationdisk_parttype_to_string(&cur->partinfo) != NULL ?
			    installationdisk_parttype_to_string(&cur->partinfo) : "",
			    cur->partinfo.partition_size,
			    cur->partinfo.partition_offset,
			    cur->partinfo.partition_size_sec,
			    cur->partinfo.partition_offset_sec);
		}
	}
	g_debug("\n");
}

void
print_gui(InstallationDiskWindowXML instdisk)
{
	gint i = 0;
	int active_idx = 0;
	gchar *active_str = NULL;
	LogicalPartition *curlogical = NULL;
	int logicalpartrow = 0;
	GtkComboBox *combo = NULL;
	GtkSpinButton *spinner = NULL;
	GtkLabel *avail = NULL;
	gdouble spinvalue;
	gdouble spinlr;
	gdouble spinur;
	const gchar *availtext;

	g_debug("");
	g_debug("%7s %3s %3s %14s %7s %6s %6s %5s %6s %6s",
	    "Type", "Idx", "Row", "PartDesc", "SpinVal", "SpinLR", "SpinUR",
	    "Avail", "SizeCh", "TypeCh");
	g_debug("%7s %3s %3s %14s %7s %6s %6s %5s %6s %6s",
	    "=======", "===", "===", "==============", "=======",
	    "======", "======", "=====", "======", "======");

	/* Primary and logical partrows and titles */
	for (i = 0; i < FD_NUMPART; i++) {
		/* print primary details */
		combo = GTK_COMBO_BOX(instdisk.partcombo[i]);
		spinner = GTK_SPIN_BUTTON(instdisk.partspin[i]);
		avail = GTK_LABEL(instdisk.partavail[i]);
		active_idx = gtk_combo_box_get_active(combo);
		active_str = gtk_combo_box_get_active_text(combo);
		spinvalue = gtk_spin_button_get_value(spinner);
		gtk_spin_button_get_range(spinner, &spinlr, &spinur);
		availtext = gtk_label_get_text(avail);

		g_debug("%7s %3d %3d %14s %07.2lf %06.2lf %06.2lf %5s %6s %6s",
		    "Primary",
		    i,
		    instdisk.partrow[i],
		    active_str != NULL ? active_str : "",
		    spinvalue, spinlr, spinur,
		    availtext != NULL ? availtext : "",
		    instdisk.partsizechanges[i] == TRUE ? "TRUE" : "FALSE",
		    instdisk.parttypechanges[i] == TRUE ? "TRUE" : "FALSE");

		if (instdisk.startlogical[i] != NULL) {
			/* print the logical details */
			logicalpartrow = 0;
			for (curlogical = instdisk.startlogical[i];
			    curlogical != NULL;
			    curlogical = curlogical->next) {

				logicalpartrow++;

				combo = GTK_COMBO_BOX(curlogical->typecombo);
				spinner =
				    GTK_SPIN_BUTTON(curlogical->sizespinner);
				avail = GTK_LABEL(curlogical->availlabel);

				active_idx = gtk_combo_box_get_active(combo);
				active_str =
				    gtk_combo_box_get_active_text(combo);
				spinvalue = gtk_spin_button_get_value(spinner);
				gtk_spin_button_get_range(
				    spinner, &spinlr, &spinur);
				availtext = gtk_label_get_text(avail);

				g_debug(
				    "%7s %3d %3d %14s %07.2lf %06.2lf %06.2lf %5s %6s %6s %d",
				    "Logical",
				    logicalpartrow,
				    instdisk.partrow[i]+logicalpartrow,
				    active_str != NULL ? active_str : "",
				    spinvalue, spinlr, spinur,
				    availtext != NULL ? availtext : "",
				    curlogical->sizechange == TRUE ?
				    "TRUE" : "FALSE",
				    curlogical->typechange == TRUE ?
				    "TRUE" : "FALSE",
				    curlogical->logpartindex);
			}
		}
	}

	g_debug("%7s %3d %14s\n", "Total",
	    instdisk.fdisktablerows, "Reset Button");
}
