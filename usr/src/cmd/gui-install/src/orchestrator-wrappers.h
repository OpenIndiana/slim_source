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
 * Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
 */

#ifndef __ORCHESTRATOR_WRAPPERS_H
#define	__ORCHESTRATOR_WRAPPERS_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
#include <glib.h>
#include <orchestrator_api.h>

#define	MBPERGB	1024
#define	GBPERTB	1024

/* Global target discovery handle */
extern om_handle_t	omhandle;

/* Function declarations */
om_handle_t
orchestrator_om_start_discovery(om_callback_t callback);

disk_info_t	**
orchestrator_om_get_disk_info(om_handle_t handle,
    gint *numdisks);

disk_info_t	*
orchestrator_om_duplicate_disk_info(disk_info_t *dinfo);

gboolean
orchestrator_om_are_parts_ordered(disk_parts_t *partitions);

gint
orchestrator_om_get_free_spacegb(om_handle_t handle,
    disk_info_t *diskinfo,
    gfloat *freespace);

gint
orchestrator_om_get_numparts_of_type(disk_parts_t *partitions,
    guint partitiontype);

disk_parts_t *
orchestrator_om_partitions_dup(disk_parts_t *partitions);

disk_parts_t *
orchestrator_om_get_disk_partitions(om_handle_t handle,
    gchar *diskname);

gint
orchestrator_om_get_max_partition_id(disk_parts_t *partitions);

partition_info_t *
orchestrator_om_get_part_by_blkorder(
    disk_parts_t *partitions,
    guint order);

partition_info_t *
orchestrator_om_get_part_by_partition_id(
    disk_parts_t *partitions,
    guint partid);

void
orchestrator_om_set_partition_info(
	partition_info_t *partinfo,
	uint32_t size,
	uint32_t offset,
	uint64_t size_sec,
	uint64_t offset_sec);

partition_info_t *
orchestrator_om_find_unused_primary_partition(
    disk_parts_t *partitions,
    guint partitiontype,
    gint order);

partition_info_t *
orchestrator_om_find_unused_logical_partition(
    disk_parts_t *partitions,
    guint partitiontype,
    gint order);

gint
orchestrator_om_get_last_logical_index(
	disk_parts_t *partitions);

guint
orchestrator_om_get_partition_type(partition_info_t *partition);

void
orchestrator_om_set_partition_type(
    partition_info_t *partition,
    guint type);

guint64
orchestrator_om_get_partition_sizemb(partition_info_t *partition);

void
orchestrator_om_set_partition_sizemb(
    partition_info_t *partition,
    guint64 size);

gfloat
orchestrator_om_get_partition_sizegb(partition_info_t *partition);

gfloat
orchestrator_om_round_mbtogb(uint32_t sizemb);

uint32_t
orchestrator_om_gbtomb(gfloat sizegb);

void
orchestrator_om_set_partition_sizegb(
    partition_info_t *partition,
    gfloat size);

gchar *
orchestrator_om_get_disk_type(disk_info_t *diskinfo);

void
orchestrator_om_set_disk_label(
    disk_info_t *diskinfo,
    om_disklabel_type_t type);

guint64
orchestrator_om_get_disk_sizemb(disk_info_t *dinfo);

gfloat
orchestrator_om_get_disk_sizegb(disk_info_t *dinfo);

const gchar *
orchestrator_om_get_disk_devicename(disk_info_t *diskinfo);

const gchar *
orchestrator_om_get_disk_vendor(disk_info_t *diskinfo);

gboolean
orchestrator_om_disk_is_bootdevice(disk_info_t *diskinfo);

guint64
orchestrator_om_get_mininstall_sizemb(void);

gfloat
orchestrator_om_get_mininstall_sizegb(gboolean roundup);

guint64
orchestrator_om_get_recommended_sizemb(void);

guint64
orchestrator_om_get_recommended_sizegb(void);

guint64
orchestrator_om_get_total_disk_sizemb(disk_info_t *dinfo);

gfloat
orchestrator_om_get_total_disk_sizegb(disk_info_t *dinfo);

/* upgrade instance */
gint
orchestrator_om_get_upgrade_targets_by_disk(
    disk_info_t *dinfo,
    upgrade_info_t **uinfo,
    guint16 *found);

upgrade_info_t *
orchestrator_om_duplicate_upgrade_targets(upgrade_info_t *uinfo);

gboolean
orchestrator_om_is_upgrade_target(upgrade_info_t *uinfo);

void
orchestrator_om_is_upgrade_target_valid(upgrade_info_t *uinfo,
    om_callback_t callback);

void
orchestrator_om_free_upgrade_targets(upgrade_info_t *uinfo);

upgrade_info_t *
orchestrator_om_upgrade_instance_get_next(upgrade_info_t *uinfo);

gchar *
orchestrator_om_upgrade_instance_get_release_name(upgrade_info_t *uinfo);

gchar *
orchestrator_om_upgrade_instance_construct_slicename(upgrade_info_t *uinfo);

gchar *
orchestrator_om_upgrade_instance_get_diskname(upgrade_info_t *uinfo);

gint
orchestrator_om_upgrade_instance_get_slicenum(upgrade_info_t *uinfo);

/* language support */
gint
orchestrator_om_get_install_languages(
    GList **locales,
	gint *total);

gint
orchestrator_om_get_available_languages(
    GList **locales,
    gint *total);

gchar *
orchestrator_om_language_get_name(lang_info_t *language);

gchar *
orchestrator_om_language_get_code(lang_info_t *language);

locale_info_t *
orchestrator_om_language_get_locales(lang_info_t *language);

gboolean
orchestrator_om_language_is_default(lang_info_t *language);

void
orchestrator_om_free_language(lang_info_t *language);

gint
orchestrator_om_language_get_locale_count(lang_info_t *language);

gchar *
orchestrator_om_locale_get_name(locale_info_t *locale);

gchar *
orchestrator_om_locale_get_desc(locale_info_t *locale);

int
orchestrator_om_perform_install(
    nvlist_t *uchoices,
    om_callback_t callback);

gboolean
orchestrator_om_locale_is_default(locale_info_t *locale);

locale_info_t *
orchestrator_om_locale_get_cposix(void);

gboolean
orchestrator_om_locale_is_cposix(locale_info_t *locale);

gboolean
orchestrator_om_locale_is_utf8(locale_info_t *locale);

void
orchestrator_om_free_locale(locale_info_t *locale);

void
orchestrator_om_set_install_lang_by_value(lang_info_t *locale_info);

void
orchestrator_om_set_install_lang_by_name(char *lang_name);

#ifdef __cplusplus
}
#endif

#endif /* __ORCHESTRATOR_WRAPPERS_H */
