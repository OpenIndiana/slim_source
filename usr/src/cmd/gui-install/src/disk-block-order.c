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
#include <ctype.h>
#include <math.h>

#include "orchestrator-wrappers.h"
#include "installation-disk-screen.h"
#include "disk-block-order.h"
#include "error-logging.h"

void
installationdisk_reorder_to_blkorder(disk_parts_t *partitions,
	DiskBlockOrder *primaryblkorder,
	DiskBlockOrder *logicalblkorder)
{
	partition_info_t tmppartinfo;
	partition_info_t *freepartinfo;
	partition_info_t *logpartinfo;
	gboolean sorted = FALSE;
	gint idx = 0;
	gint parttype;
	DiskBlockOrder *curblkorder;

	/* Sort primaries first */
	while (sorted == FALSE) {
		sorted = TRUE;
		for (idx = 1; idx < FD_NUMPART; idx++) {
			/* Both partitions should have size > 0 */
			if (partitions->pinfo[idx-1].partition_size <= 0 ||
			    partitions->pinfo[idx].partition_size <= 0)
				continue;

			if (partitions->pinfo[idx-1].partition_offset >
			    partitions->pinfo[idx].partition_offset) {
				/* Switch them around */
				tmppartinfo = partitions->pinfo[idx-1];
				partitions->pinfo[idx-1] = partitions->pinfo[idx];
				partitions->pinfo[idx] = tmppartinfo;
				sorted = FALSE;
			}
		}
	}

	/* After sorting set partition_order to correct ordering */
	for (idx = 0; idx < FD_NUMPART; idx++) {
		partitions->pinfo[idx].partition_order = idx+1;

		/*
		 * Need to set partition_order on equivalent blkorder item
		 * to ensure in sync.
		 */
		curblkorder = installationdisk_blkorder_get_by_partition_id(
		    primaryblkorder, partitions->pinfo[idx].partition_id);

		if (curblkorder != NULL) {
			curblkorder->partinfo.partition_order =
			    partitions->pinfo[idx].partition_order;
		}
	}

	/* Sort logicals now if they exist */
	sorted = FALSE;
	while (sorted == FALSE) {
		sorted = TRUE;
		for (idx = FD_NUMPART+1; idx < OM_NUMPART; idx++) {
			if (partitions->pinfo[idx-1].partition_size <= 0 ||
			    partitions->pinfo[idx].partition_size <= 0)
				continue;

			if (partitions->pinfo[idx-1].partition_id <= 0 ||
			    partitions->pinfo[idx].partition_id <= 0)
				continue;

			if (partitions->pinfo[idx-1].partition_offset >
			    partitions->pinfo[idx].partition_offset) {
				/* Offsets don't align, switch them around */
				tmppartinfo = partitions->pinfo[idx-1];
				partitions->pinfo[idx-1] = partitions->pinfo[idx];
				partitions->pinfo[idx] = tmppartinfo;
				sorted = FALSE;
			}
		}
	}

	/* After sorting set partition_order to correct ordering */
	for (idx = FD_NUMPART; idx < OM_NUMPART; idx++) {
		if (partitions->pinfo[idx].partition_id > 0) {
			partitions->pinfo[idx].partition_order = idx+1;

			/*
			 * Need to set partition_order on equivalent blkorder item
			 * to ensure in sync.
			 */
			curblkorder = installationdisk_blkorder_get_by_partition_id(
			    logicalblkorder, partitions->pinfo[idx].partition_id);

			if (curblkorder != NULL) {
				curblkorder->partinfo.partition_order =
				    partitions->pinfo[idx].partition_order;
			}
		} else {
			break;
		}
	}
}

void
installationdisk_get_blkorder_layout(disk_info_t *diskinfo,
	disk_parts_t *partitions,
	DiskBlockOrder **_primaryblkorder,
	DiskBlockOrder **_logicalblkorder)
{
	DiskBlockOrder *primaryblkorder = NULL;
	DiskBlockOrder *logicalblkorder = NULL;
	gint logpartindex, primpartindex;
	gint primparttype, logparttype;
	partition_info_t *primpartinfo = NULL;
	partition_info_t *logpartinfo = NULL;
	DiskBlockOrder *gap = NULL;
	DiskBlockOrder *tmpprimary = NULL;
	DiskBlockOrder *tmplogical = NULL;
	DiskBlockOrder *curprimary = NULL;
	DiskBlockOrder *curlogical = NULL;
	uint8_t primary_order = 0;
	uint8_t logical_order = 4;
	uint64_t disk_size_sec = 0;
	gfloat part_size = 0;

	if (_primaryblkorder != NULL) {
		primaryblkorder = *_primaryblkorder;
	}
	if (_logicalblkorder != NULL) {
		logicalblkorder = *_logicalblkorder;
	}

	if (primaryblkorder != NULL) {
		/* Free the List of primary block order elements */
		installationdisk_blkorder_free_list(primaryblkorder);
		primaryblkorder = NULL;
	}

	if (logicalblkorder != NULL) {
		/* Free the List of logical block order elements */
		installationdisk_blkorder_free_list(logicalblkorder);
		logicalblkorder = NULL;
	}

	for (primpartindex = 0;
	    primpartindex < FD_NUMPART;
	    primpartindex++) {
		primpartinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, primpartindex);

		if (!primpartinfo) {
			/* Check if space at end of disk */
			if (primaryblkorder == NULL) {
				/*
				 * No partitions at all on disk !!
				 * Allocate DiskBlockOrder
				 */
				gap = g_new0(DiskBlockOrder, 1);

				orchestrator_om_set_partition_info(&gap->partinfo,
				    diskinfo->disk_size,
				    0,
				    diskinfo->disk_size_sec,
				    0);
				gap->displayed = FALSE;
				gap->next = NULL;
				gap->partinfo.partition_order = 0;
				primaryblkorder = gap;
				curprimary = primaryblkorder;
			} else if ((curprimary->partinfo.partition_offset +
			    curprimary->partinfo.partition_size) <
			    diskinfo->disk_size) {
				/* disk_size & disk_size_sec */
				gap = g_new0(DiskBlockOrder, 1);
				if (diskinfo->disk_size_sec == 0) {
					/*
					 * Attempt to calculate the number of cylinders based
					 * on curprimary data
					 */
					disk_size_sec =
					    (curprimary->partinfo.partition_size_sec /
					    curprimary->partinfo.partition_size) *
					    diskinfo->disk_size;
				} else {
					disk_size_sec = diskinfo->disk_size_sec;
				}

				orchestrator_om_set_partition_info(&gap->partinfo,
				    diskinfo->disk_size -
				    (curprimary->partinfo.partition_offset +
				    curprimary->partinfo.partition_size),
				    (curprimary->partinfo.partition_offset +
				    curprimary->partinfo.partition_size + 1),
				    disk_size_sec -
				    (curprimary->partinfo.partition_offset_sec +
				    curprimary->partinfo.partition_size_sec),
				    (curprimary->partinfo.partition_offset_sec +
				    curprimary->partinfo.partition_size_sec + 1));
				gap->partinfo.partition_order = 0;
				gap->displayed = FALSE;
				curprimary->next = gap;
				curprimary = curprimary->next;
			}
			break;
		} else {
			/* Allocate DiskBlockOrder */
			tmpprimary = g_new0(DiskBlockOrder, 1);
			(void) memcpy(&tmpprimary->partinfo, primpartinfo,
			    sizeof (partition_info_t));
			tmpprimary->displayed = TRUE;
			tmpprimary->next = NULL;

			if (primaryblkorder == NULL &&
			    primpartinfo->partition_offset > 1) {
				/* Gap at start of disk */
				gap = g_new0(DiskBlockOrder, 1);
				orchestrator_om_set_partition_info(&gap->partinfo,
				    tmpprimary->partinfo.partition_offset, 0,
				    tmpprimary->partinfo.partition_offset_sec, 0);
				gap->displayed = FALSE;
				gap->next = NULL;
				gap->partinfo.partition_order = 0;
				primaryblkorder = gap;
				curprimary = primaryblkorder;
			}

			if (primaryblkorder == NULL) {
				/* First item in the list */
				tmpprimary->partinfo.partition_order = ++primary_order;
				primaryblkorder = tmpprimary;
				curprimary = primaryblkorder;
			} else {
				/*
				 * Check if there is unused space between this partition
				 * And the previous partition
				 */
				if ((curprimary->partinfo.partition_offset +
				    curprimary->partinfo.partition_size +1) <
				    tmpprimary->partinfo.partition_offset) {
					/*
					 * curprimary points to previous list item
					 * Previous List item size + offset should be equal to
					 * current offset, if not then there is a gap
					 */

					/* Gap Between partitions */
					gap = g_new0(DiskBlockOrder, 1);
					orchestrator_om_set_partition_info(&gap->partinfo,
					    tmpprimary->partinfo.partition_offset -
					    (curprimary->partinfo.partition_offset +
					    curprimary->partinfo.partition_size + 1),
					    (curprimary->partinfo.partition_offset +
					    curprimary->partinfo.partition_size + 1),
					    tmpprimary->partinfo.partition_offset_sec -
					    (curprimary->partinfo.partition_offset_sec +
					    curprimary->partinfo.partition_size_sec + 1),
					    (curprimary->partinfo.partition_offset_sec +
					    curprimary->partinfo.partition_size_sec + 1));
					gap->displayed = FALSE;
					gap->next = NULL;
					gap->partinfo.partition_order = 0;

					curprimary->next = gap;
					curprimary = curprimary->next;
				}

				tmpprimary->partinfo.partition_order = ++primary_order;
				curprimary->next = tmpprimary;
				curprimary = curprimary->next;
			}

			/* Check if Extended Primary */
			primparttype = orchestrator_om_get_partition_type(primpartinfo);
			if (IS_EXT_PAR(primparttype)) {
				/*
				 * Cycle through logical partitions
				 * Logic here is the same as for primary partitions
				 */
				for (logpartindex = FD_NUMPART;
				    logpartindex < OM_NUMPART;
				    logpartindex++) {
					logpartinfo =
					    orchestrator_om_get_part_by_blkorder(partitions,
					    logpartindex);

					if (!logpartinfo) {
						/* Check if space at end of extended partition */
						if (logicalblkorder == NULL) {
							/*
							 * No logical disks found so entire extended
							 * primary is a gap and available for new logical
							 */
							gap = g_new0(DiskBlockOrder, 1);
							(void) memcpy(&gap->partinfo, primpartinfo,
							    sizeof (partition_info_t));
							gap->displayed = FALSE;
							gap->next = NULL;

							gap->partinfo.partition_order = ++logical_order;
							logicalblkorder = gap;
							curlogical = logicalblkorder;
						} else if ((curlogical->partinfo.partition_offset +
						    curlogical->partinfo.partition_size) <
						    (primpartinfo->partition_offset +
						    primpartinfo->partition_size)) {
							/* Gap at end */
							gap = g_new0(DiskBlockOrder, 1);
							orchestrator_om_set_partition_info(&gap->partinfo,
							    (primpartinfo->partition_offset +
							    primpartinfo->partition_size) -
							    (curlogical->partinfo.partition_offset +
							    curlogical->partinfo.partition_size),
							    (curlogical->partinfo.partition_offset +
							    curlogical->partinfo.partition_size + 1),
							    (primpartinfo->partition_offset_sec +
							    primpartinfo->partition_size_sec) -
							    (curlogical->partinfo.partition_offset_sec +
							    curlogical->partinfo.partition_size_sec),
							    (curlogical->partinfo.partition_offset_sec +
							    curlogical->partinfo.partition_size_sec + 1));
							gap->displayed = FALSE;
							gap->next = NULL;

							gap->partinfo.partition_order = ++logical_order;
							curlogical->next = gap;
							curlogical = curlogical->next;
						}
						break;
					} else {
						/* Allocate DiskBlockOrder */
						tmplogical = g_new0(DiskBlockOrder, 1);
						(void) memcpy(&tmplogical->partinfo, logpartinfo,
						    sizeof (partition_info_t));
						tmplogical->displayed = TRUE;
						tmplogical->next = NULL;

						if (logicalblkorder == NULL &&
						    (logpartinfo->partition_offset >
						    primpartinfo->partition_offset)) {
							/* Gap at start of logical disk */
							gap = g_new0(DiskBlockOrder, 1);
							orchestrator_om_set_partition_info(&gap->partinfo,
							    tmplogical->partinfo.partition_offset -
							    primpartinfo->partition_offset,
							    primpartinfo->partition_offset,
							    tmplogical->partinfo.partition_offset_sec -
							    primpartinfo->partition_offset_sec,
							    primpartinfo->partition_offset_sec);
							gap->displayed = FALSE;
							gap->next = NULL;
							gap->partinfo.partition_order = ++logical_order;
							logicalblkorder = gap;
							curlogical = logicalblkorder;
						}

						if (logicalblkorder == NULL) {
							/* First item in the list */
							tmplogical->partinfo.partition_order =
							    ++logical_order;
							logicalblkorder = tmplogical;
							curlogical = logicalblkorder;
						} else {
							/*
							 * Check if there is unused space between
							 * this partition and the previous partition
							 */
							if ((curlogical->partinfo.partition_offset +
							    curlogical->partinfo.partition_size +1) <
							    tmplogical->partinfo.partition_offset) {
								/*
								 * curlogical points to previous list item
								 * Previous List item size + offset should
								 * be equal to current offset, if not then
								 * there is a gap
								 */

								/* Gap Between partitions */
								gap = g_new0(DiskBlockOrder, 1);
								orchestrator_om_set_partition_info(
								    &gap->partinfo,
								    tmplogical->partinfo.partition_offset -
								    (curlogical->partinfo.partition_offset +
								    curlogical->partinfo.partition_size + 1),
								    (curlogical->partinfo.partition_offset +
								    curlogical->partinfo.partition_size + 1),
								    tmplogical->partinfo.partition_offset_sec -
								    (curlogical->partinfo.partition_offset_sec +
								    curlogical->partinfo.partition_size_sec+1),
								    (curlogical->partinfo.partition_offset_sec +
								    curlogical->partinfo.partition_size_sec+1));
								gap->displayed = FALSE;
								gap->next = NULL;

								gap->partinfo.partition_order = ++logical_order;
								curlogical->next = gap;
								curlogical = curlogical->next;
							}

							tmplogical->partinfo.partition_order =
							    ++logical_order;
							curlogical->next = tmplogical;
							curlogical = curlogical->next;
						}
					}
				}
			}
		}
	}

	/*
	 * Traverse logicals and remove any gaps that are less
	 * 0.1GB in size as these cannot be displayed.
	 */
	if (logicalblkorder) {
		for (curlogical = logicalblkorder; curlogical != NULL; ) {
			part_size = orchestrator_om_round_mbtogb(
			    curlogical->partinfo.partition_size);

			if (part_size <= 0) {
				curlogical = installationdisk_blkorder_remove(
				    FALSE,
				    &logicalblkorder,
				    curlogical,
				    TRUE);
				continue;
			}
			curlogical = curlogical->next;
		}
	}

	if (_primaryblkorder != NULL) {
		*_primaryblkorder = primaryblkorder;
	}
	if (_logicalblkorder != NULL) {
		*_logicalblkorder = logicalblkorder;
	}
}

DiskBlockOrder *
installationdisk_blkorder_dup(DiskBlockOrder *srcblkorder)
{
	DiskBlockOrder *startdest = NULL;
	DiskBlockOrder *curdest = NULL;
	DiskBlockOrder *cursrc = NULL;
	DiskBlockOrder *gap = NULL;

	if (srcblkorder == NULL) {
		return (NULL);
	}

	for (cursrc = srcblkorder; cursrc != NULL; cursrc = cursrc->next) {
		gap = g_new0(DiskBlockOrder, 1);
		(void) memcpy(gap, cursrc, sizeof (DiskBlockOrder));

		/* Zap next pointer, this will be set in curdest */
		gap->next = NULL;

		if (startdest == NULL) {
			startdest = gap;
			curdest = gap;
		} else {
			curdest->next = gap;
			curdest = curdest->next;
		}
	}

	return (startdest);
}

void
installationdisk_blkorder_free_list(DiskBlockOrder *startblkorder)
{
	DiskBlockOrder *curblkorder;
	DiskBlockOrder *tmpblkorder;

	g_return_if_fail(startblkorder != NULL);

	for (curblkorder = startblkorder; curblkorder != NULL; TRUE) {
		tmpblkorder = curblkorder->next;
		g_free(curblkorder);
		curblkorder = tmpblkorder;
	}
}

DiskBlockOrder *
installationdisk_blkorder_getlast(DiskBlockOrder *startblkorder)
{
	/* Given a DiskBlockorder list starting point return pointer */
	/* To last item on the list */
	DiskBlockOrder *curblkorder;

	g_return_val_if_fail(startblkorder != NULL, NULL);

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder->next == NULL) {
			break;
		}
	}
	return (curblkorder);
}

DiskBlockOrder *
installationdisk_blkorder_getprev(DiskBlockOrder *startblkorder,
	DiskBlockOrder *blkorder)
{
	DiskBlockOrder *curblkorder = NULL;
	DiskBlockOrder *retblkorder = NULL;

	/* Given a DiskBlockOrder list and a element, return the previous */
	/* DiskBlockOrder item in the list or NULL if reached the start */
	g_return_val_if_fail(startblkorder != NULL, NULL);

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder == blkorder) {
			break;
		}
		retblkorder = curblkorder;
	}
	return (retblkorder);
}

DiskBlockOrder *
installationdisk_blkorder_get_by_partition_id(DiskBlockOrder *startblkorder,
	gint id)
{
	DiskBlockOrder *curblkorder = NULL;

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder->partinfo.partition_id == id) {
			return (curblkorder);
		}
	}

	return (NULL);
}

DiskBlockOrder *
installationdisk_blkorder_get_by_partition_order(DiskBlockOrder *startblkorder,
	gint order)
{
	DiskBlockOrder *curblkorder = NULL;

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder->partinfo.partition_order == order) {
			return (curblkorder);
		}
	}

	return (NULL);
}

gint
installationdisk_blkorder_get_index(DiskBlockOrder *startblkorder,
	DiskBlockOrder *blkordertoget)
{
	gint retidx = -1;
	DiskBlockOrder *curblkorder;


	/* Returning indx count into linked list, index starting at 0 */
	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		retidx++;
		if (blkordertoget == curblkorder) {
			break;
		}
	}

	return (retidx);
}

gboolean
update_blkorder_from_partinfo(DiskBlockOrder *startblkorder,
	partition_info_t *partinfo)
{
	DiskBlockOrder *curblkorder = NULL;
	gboolean retval = FALSE;

	g_return_val_if_fail(startblkorder != NULL, retval);

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder->partinfo.partition_id ==
		    partinfo->partition_id) {
			curblkorder->partinfo.partition_size =
			    partinfo->partition_size;
			curblkorder->partinfo.partition_type =
			    partinfo->partition_type;
			retval = TRUE;
			break;
		}
	}

	return (retval);
}

gboolean
update_partinfo_from_blkorder(gboolean is_primary,
	DiskBlockOrder *blkorder,
	disk_parts_t *partitions)
{
	gint idx = 0;
	gint startidx = 0;
	gint endidx = 0;
	partition_info_t *partinfo;
	gboolean retval = FALSE;

	g_return_val_if_fail(blkorder != NULL, retval);

	if (is_primary == TRUE) {
		startidx = 0;
		endidx = FD_NUMPART;
	} else {
		startidx = FD_NUMPART;
		endidx = OM_NUMPART;
	}

	g_debug("update_partinfo_from_blkorder : %d : %d", startidx, endidx);

	for (idx = startidx; idx < endidx; idx++) {
		partinfo =
		    orchestrator_om_get_part_by_blkorder(partitions, idx);

		if (idx < FD_NUMPART) {
			g_assert(partinfo);
		}

		if (partinfo) {
			if (blkorder->partinfo.partition_id == partinfo->partition_id) {
				partinfo->partition_size =
				    blkorder->partinfo.partition_size;
				retval = TRUE;
				break;
			}
		}
	}

	if (idx == endidx) {
		g_warning("Failed to update partinfo from blkorder\n");
	}

	return (retval);
}

DiskBlockOrder *
installationdisk_blkorder_remove(gboolean is_primary,
	DiskBlockOrder **_startblkorder,
	DiskBlockOrder *blkordertoremove,
	gboolean ret_next_item)
{
	DiskBlockOrder *curblkorder = NULL;
	DiskBlockOrder *retblkorder = NULL;
	DiskBlockOrder *prevblkorder = NULL;
	DiskBlockOrder *startblkorder = NULL;

	startblkorder = *_startblkorder;

	/* Remove blkordertoremove item from the startblkorder list */
	/* Return pointer to next item in the list */
	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder == blkordertoremove) {
			/* Free this item */
			if (prevblkorder == NULL) {
				/* At start of the list, reset starting point */
				*_startblkorder = curblkorder->next;
			} else {
				prevblkorder->next = curblkorder->next;
			}

			if (ret_next_item == TRUE) {
				retblkorder = curblkorder->next;
			} else {
				retblkorder = prevblkorder;
			}
			g_free(curblkorder);
			break;
		}
		prevblkorder = curblkorder;
	}

	if (is_primary == FALSE) {
		/* For logicals we need to reduce the partition_order field aswell */
		if (prevblkorder == NULL) {
			curblkorder = *_startblkorder;
		} else {
			curblkorder = prevblkorder->next;
		}

		for (; curblkorder != NULL; curblkorder = curblkorder->next) {
			curblkorder->partinfo.partition_order--;
		}
	}

	return (retblkorder);
}

void
installationdisk_blkorder_insert_displayed(DiskBlockOrder *startblkorder,
	DiskBlockOrder *newblkorder)
{
	DiskBlockOrder *curblkorder = NULL;
	gint pidx = 0;

	/*
	 * insert new blkorder item just after item based on new blkorders
	 * own partition_order.
	 */

	for (pidx = 0; pidx < FD_NUMPART; pidx++) {
		curblkorder = installationdisk_blkorder_get_by_partition_order(
		    startblkorder,
		    pidx+1);

		if (curblkorder == NULL) {
			/* Insert after previous item */
			curblkorder = installationdisk_blkorder_get_by_partition_order(
			    startblkorder,
			    pidx);
			installationdisk_blkorder_insert_after(
			    startblkorder, curblkorder, newblkorder, FALSE);
			break;
		}
	}
}

void
installationdisk_blkorder_insert_after(DiskBlockOrder *startblkorder,
	DiskBlockOrder *addafterblkorder,
	DiskBlockOrder *newblkorder,
	gboolean increment_partition_order)
{
	DiskBlockOrder *curblkorder = NULL;
	gboolean incrementing = FALSE;

	/* Insert a newblkorder item after an existing item */
	/* Can never be inserted before the first item */
	g_return_if_fail(startblkorder != NULL);
	g_return_if_fail(addafterblkorder != NULL);
	g_return_if_fail(newblkorder != NULL);

	for (curblkorder = startblkorder;
	    curblkorder != NULL;
	    curblkorder = curblkorder->next) {
		if (curblkorder == addafterblkorder) {
			newblkorder->next = curblkorder->next;
			curblkorder->next = newblkorder;
			if (increment_partition_order == FALSE) {
				break;
			}
		} else if (increment_partition_order == TRUE) {
			if (curblkorder == newblkorder) {
				incrementing = TRUE;
			} else if (incrementing == TRUE) {
				curblkorder->partinfo.partition_order++;
			}
		}
	}
}

void
installationdisk_blkorder_empty_partinfo_sync(
	disk_parts_t *partitions,
	DiskBlockOrder *startblkorder,
	DiskBlockOrder *curblkorder,
	DiskBlockOrder *newblkorder)
{
	DiskBlockOrder *tmpblkorder = NULL;
	partition_info_t *partinfo;

	/* partition_order already at max primary of 4 return */
	g_return_if_fail(curblkorder->partinfo.partition_order < 4);

	/*
	 * Scan through startblkorder list and check if there are any
	 * partition_orders greater than this one
	 */
	for (tmpblkorder = curblkorder->next;
	    tmpblkorder != NULL;
	    tmpblkorder = tmpblkorder->next) {
		if (tmpblkorder->partinfo.partition_order >
		    curblkorder->partinfo.partition_order) {
			return;
		}
	}

	/*
	 * If we made it here then there's an empty primary slot displayed which
	 * Should be synced up with newblkorder that has been added
	 */
	partinfo = orchestrator_om_get_part_by_blkorder(partitions,
	    curblkorder->partinfo.partition_order);

	if (partinfo != NULL) {
		partinfo->partition_size = newblkorder->partinfo.partition_size;
		newblkorder->partinfo.partition_id = partinfo->partition_id;
		newblkorder->partinfo.partition_order = partinfo->partition_order;
		newblkorder->displayed = TRUE;
	}
}

void
update_partinfo_from_blkorder_and_display(disk_parts_t *partitions,
	partition_info_t *modpartinfo,
	DiskBlockOrder *curblkorder)
{
	partition_info_t *partinfo = NULL;
	gint pidx = 0;
	gint parttype = 0;

	/* Get next item after current primary being amended */
	if (modpartinfo->partition_order < 4) {
		/* Try next available partition below primary being modified */
		pidx = modpartinfo->partition_order;
	} else {
		/* -1 will get current modpartinfo, -2 will get one before that */
		pidx = modpartinfo->partition_order-2;
	}

	partinfo =
	    orchestrator_om_get_part_by_blkorder(partitions, pidx);

	/* partinfo should always contain a value at this juncture */
	g_assert(partinfo != NULL);

	/*
	 * If this is an unused slot, we need to just sync this item
	 * with the current curblkorder item
	 */
	parttype =
	    orchestrator_om_get_partition_type(partinfo);
	if (parttype == UNUSED) {
		partinfo->partition_size = curblkorder->partinfo.partition_size;
		curblkorder->partinfo.partition_id = partinfo->partition_id;
		curblkorder->partinfo.partition_order = partinfo->partition_order;
		curblkorder->displayed = TRUE;
	}
}

partition_info_t *
installationdisk_get_largest_free_block(gint disknum,
	gboolean setunused,
	DiskBlockOrder *startblkorder,
	partition_info_t *partinfo)
{
	/*
	 * Traverse DiskBlockOrder *primary or *logical, returning
	 * Largest free partinfo struct available, and set used flag
	 */
	DiskBlockOrder *cur = NULL;
	uint32_t size = 0;
	partition_info_t *retpartinfo = NULL;
	DiskBlockOrder *largestfree = NULL;
	gchar *partsizestr = NULL;

	/* Traverse the list */
	for (cur = startblkorder; cur != NULL; cur = cur->next) {
		if (cur->displayed == FALSE) {
			partsizestr = g_strdup_printf("%.1f",
			    orchestrator_om_get_partition_sizegb(&cur->partinfo));
			if (cur->partinfo.partition_size > 0 &&
			    strtod(partsizestr, NULL) > 0 &&
			    cur->partinfo.partition_size > size) {
				largestfree = cur;
				size = cur->partinfo.partition_size;
			}
			g_free(partsizestr);
		}
	}

	/* We have a free chunk, set unused to false and return */
	if (largestfree != NULL) {
		g_debug("Largest Free Chunk :");
		print_partinfo(-1, &largestfree->partinfo, TRUE);
		if (setunused == TRUE) {
			largestfree->displayed = TRUE;
			largestfree->partinfo.partition_id = partinfo->partition_id;
			largestfree->partinfo.partition_order = partinfo->partition_order;
		}
		retpartinfo = &largestfree->partinfo;
	}
	return (retpartinfo);
}
