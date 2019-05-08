#
# CDDL HEADER START
#
# The contents of this file are subject to the terms of the
# Common Development and Distribution License (the "License").
# You may not use this file except in compliance with the License.
#
# You can obtain a copy of the license at usr/src/OPENSOLARIS.LICENSE
# or http://www.opensolaris.org/os/licensing.
# See the License for the specific language governing permissions
# and limitations under the License.
#
# When distributing Covered Code, include this CDDL HEADER in each
# file and include the License file at usr/src/OPENSOLARIS.LICENSE.
# If applicable, add the following below this CDDL HEADER, with the
# fields enclosed by brackets "[]" replaced with your own identifying
# information: Portions Copyright [yyyy] [name of copyright owner]
#
# CDDL HEADER END
#
# Copyright 2010 Sun Microsystems, Inc.  All rights reserved.
# Use is subject to license terms.
#

'''
Utility functions
'''

import logging
import os
import shutil
from tempfile import NamedTemporaryFile
from subprocess import Popen
from subprocess import PIPE
from osol_install.install_utils import exec_cmd_outputs_to_log
from osol_install.profile.disk_space import DiskSpace
import osol_install.tgt as tgt

class InstallationError(Exception):
    '''Some sort of error occurred during installation.  The exact
       cause of the error should have been logged.  So, this
       just indicates that something is wrong

    '''
    pass

class NotEnoughSpaceError(Exception):
    '''There's not enough space in the target disk for successful installation

    '''
    pass

# All sizes are in MB
MIN_SWAP_SIZE = 512
MAX_SWAP_SIZE = DiskSpace("32gb").size_as("mb")		#32G
MIN_DUMP_SIZE = 256
MAX_DUMP_SIZE = DiskSpace("16gb").size_as("mb")		#16G
OVERHEAD = 1024
FUTURE_UPGRADE_SPACE = DiskSpace("2gb").size_as("mb") #2G
ZVOL_REQ_MEM = 900      # Swap ZVOL is required if memory is below this

class SwapDump:
    ''' All information associated with swap and dump'''

    ''' The type of swap/dump.  Define them as strings so debugging output
        is easier to read.
    '''
    SLICE = "Slice"
    ZVOL = "ZVOL"
    NONE = "None"

    mem_size = 0
    swap_type = ""
    swap_size = 0

    dump_type = ""
    dump_size = 0

    swap_dump_computed = False

    def __init__(self):
        ''' Initialize swap/dump calculation.  This will get the size of
            running's system's memory
        '''
        self.mem_size = get_system_memory()
        self.swap_type = SwapDump.NONE
        self.swap_size = 0
        self.dump_type = SwapDump.NONE
        self.dump_size = 0
        self.swap_dump_computed = False

    def get_required_swap_size(self):
        ''' Determines whether swap is required.  If so, the amount of
            space used for swap is returned.  If swap is not required,
            0 will be returned.  Value returned is in MB.

            If system memory is less than 900mb, swap is required.
            Minimum required space for swap is 0.5G (MIN_SWAP_SIZE).
        '''
   
        if (self.mem_size < ZVOL_REQ_MEM):
            return MIN_SWAP_SIZE

        return 0
       
    
    def calc_swap_dump_size(self, installation_size, available_size,
                            swap_included=False):
        '''Calculate swap/dump, based on the amount of
           system memory, installation size and available size.

           The following rules are used for
           determining the type of swap to be created, whether swap zvol
           is required and the size of swap to be created.
 
            memory        type           required    size
            --------------------------------------------------
            <900mb        zvol           yes          0.5G (MIN_SWAP_SIZE)
            900mb-1G      zvol            no          0.5G (MIN_SWAP_SIZE)
            1G-64G        zvol            no          (0.5G-32G) 1/2 of memory
            >64G          zvol            no          32G (MAX_SWAP_SIZE)

            The following rules are used for calculating the amount
            of required space for dump

            memory        type            size
            --------------------------------------------------
            <0.5G         zvol            256MB (MIN_DUMP_SIZE)
            0.5G-32G      zvol            256M-16G (1/2 of memory)
            >32G          zvol            16G (MAX_DUMP_SIZE)

            If slice/zvol is required, and there's not enough space in the,
            target, an error will be raised.  If swap zvol is
            not required, and there's not enough space in the target, as much
            space as available will be utilized for swap/dump

            Size of all calculation is done in MB
      
            Input:
                installation_size: size required for installation (MB)
                available_size: available size in the target disk. (MB)
                swap_included: Indicate whether required swap space is already
                               included and validated in the installation size.
                               Default to false.

            Output:
                returns a tuple (swap_type, swap_size, dump_type, dump_size)

            Raise:
                NotEnoughSpaceError 
        '''
        if self.swap_dump_computed:
            return(self.swap_type, self.swap_size, self.dump_type,
                   self.dump_size)

        swap_required = False

        if (installation_size > available_size):
            logging.error("Space required for installation: %s",
                          installation_size)
            logging.error("Total available space: %s", available_size)
            raise NotEnoughSpaceError

        self.swap_size = self.get_required_swap_size()
        if self.swap_size != 0:
            swap_required = True

        logging.debug("Installation size: %sMB", installation_size)
        logging.debug("Available size: %sMB", available_size)
        logging.debug("Memory: %sMB. Swap Required: %s",
                      self.mem_size, swap_required)

        if swap_required:
            # Make sure target disk has enough space for both swap and software
            if swap_included:
                with_swap_size = installation_size
            else:
                with_swap_size = installation_size + self.swap_size
                if (available_size < with_swap_size):
                    logging.error("Space required for installation "
                                  "with required swap: %s", with_swap_size)
                    logging.error("Total available space: %s", available_size)
                    raise NotEnoughSpaceError
            
            # calculate the size for dump
            self.dump_size = self.__calc_size(available_size - with_swap_size,
                                              MIN_DUMP_SIZE, MAX_DUMP_SIZE)
        else:
            free_space = available_size - installation_size
            self.swap_size = self.__calc_size(((free_space * MIN_SWAP_SIZE) / 
                                       (MIN_SWAP_SIZE + MIN_DUMP_SIZE)),
                                       MIN_SWAP_SIZE, MAX_SWAP_SIZE, 1)
            self.dump_size = self.__calc_size(((free_space * MIN_DUMP_SIZE) /
                                       (MIN_SWAP_SIZE + MIN_DUMP_SIZE)),
                                       MIN_DUMP_SIZE, MAX_DUMP_SIZE)
        if (self.dump_size > 0):
            self.dump_type = SwapDump.ZVOL

        if (self.swap_size > 0):
            self.swap_type = SwapDump.ZVOL

        logging.debug("Swap Type: %s", self.swap_type)
        logging.debug("Swap Size: %s", self.swap_size)
        logging.debug("Dump Type: %s", self.dump_type)
        logging.debug("Dump Size: %s", self.dump_size)
        self.swap_dump_computed = True

        return(self.swap_type, self.swap_size, self.dump_type, self.dump_size)

    def __calc_size(self, available_space, min_size, max_size, is_swap=0):
        '''Calculates size of swap or dump in MB based on amount of
           physical memory available.

           If less than calculated space is available, swap/dump size will be
           trimmed down to the avaiable space.  If calculated space
           is more than the max size to be used, the swap/dump size will
           be trimmed down to the maximum size to be used for swap/dump

           Because Firefox requires a lot of memory, and thus a lot of reserved
           swap, swap size is counted differently to dump size.

           Args:
               available_swap_space: space that can be dedicated to swap in MB
               min_size: minimum size to use
               max_size: maximum size to use
               is_swap: swap size is being calculated

           Returns:
               size of swap in MB

        '''

        ratio = 2
        if is_swap:
            ratio = 0.5

        if available_space == 0:
            return (0)

        if (self.mem_size < min_size):
            size = min_size
        else:
            size = int(self.mem_size / ratio)
            if (size >  max_size):
                size = max_size

        if (available_space < size):
            size = available_space - 1024

        return (int)(size)	# Make sure size is an int

    def get_swap_device(self, pool_name):
        ''' Return the string representing the device used for swap '''
        if (self.swap_type == SwapDump.ZVOL):
            return ("/dev/zvol/dsk/" + pool_name + "/swap")

        return None

IMAGE_INFO = "/.cdrom/.image_info"
IMAGE_SIZE_KEYWORD = "IMAGE_SIZE"

def get_image_size():
    '''Total size of the software in the image is stored in the 
       /.cdrom/.image_info indicated by the keywoard IMAGE_SIZE.
       This function retrieves that value from the .image_file
       The size recorded in the .image_file is in KB, other functions
       in this file uses the value in MB, so, this function will
       return the size in MB

       Returns:
           size of retrieved from the .image_info file in MB

    '''
    img_size = 0
    try:
        with open(IMAGE_INFO, 'r') as ih:
            for line in ih:
                (opt, val) = line.split("=")
                if opt == IMAGE_SIZE_KEYWORD:
                    # Remove the '\n' character read from
                    # the file, and convert to integer
                    img_size = int(val.rstrip('\n'))
                    break
    except IOError, ioe:
        logging.error("Failed to access %s", IMAGE_INFO)
        logging.exception(ioe)
        raise InstallationError
    except ValueError, ive:
        logging.error("Invalid file format in %s", IMAGE_INFO)
        logging.exception(ive)
        raise InstallationError

    if (img_size == 0):
        # We should have read in a size by now
        logging.error("Unable to read the image size from %s", IMAGE_INFO)
        raise InstallationError

    return (DiskSpace(str(img_size) +"kb").size_as("mb"))


def get_system_memory():
    ''' Returns the amount of memory available in the system
        The value returned is in MB.

    '''
    memory_size = 0
    try:
        with os.popen("/usr/sbin/prtconf") as fp:
            for line in fp.readlines():
                # Looking for the line that says "Memory size: xxxxx Megabytes"
                val = line.split()
                if ((len(val) == 4) and ((val[0] + " " + val[1]) == \
	            "Memory size:")):
                    memory_size = int(val[2]) # convert the size to an integer
                    break
    except Exception:
        pass

    if (memory_size <= 0):
        # We should have a valid size now
        logging.error("Unable to determine amount of system memory")
        raise InstallationError

    return memory_size

def get_minimum_size(swap_dump_info):
    ''' Returns the minimum amount of space required to perform an installation
        This does take into account MIN_SWAP_SIZE required for
        low-memory system.
        
        Size is returned in MB.

    '''
    swap_size = swap_dump_info.get_required_swap_size()
    return(get_image_size() + OVERHEAD + swap_size)

def get_minimum_size_without_swap():
    ''' Returns the minimum amount of space required to perform an installation
        without using swap.
        
        Size is returned in MB.

    '''
    return(get_image_size() + OVERHEAD)
    
def get_recommended_size(swap_dump_info):
    '''Returns the recommended size to perform an installation.
    This takes into account estimated space to perform an upgrade.

    '''
    return (get_minimum_size(swap_dump_info) + FUTURE_UPGRADE_SPACE)

def get_recommended_size_without_swap():
    '''Returns the recommended size to perform an installation
    without swap.
    This takes into account estimated space to perform an upgrade.

    '''
    return (get_minimum_size_without_swap() + FUTURE_UPGRADE_SPACE)

INIT_FILE = "/etc/default/init"
TIMEZONE_KW = "TZ"
def save_timezone_in_init(basedir, timezone):
    '''Save the timezone in /etc/default/init.

    '''    
    saved_tz = False
    init_file = basedir + INIT_FILE
    try:
        with open(init_file, 'r') as ih:
            with NamedTemporaryFile(dir="/tmp", delete=False) as th:
                tmp_fname = th.name

                for line in ih:
                    eq = line.split("=")
                    if eq[0] == TIMEZONE_KW:
                        new_line = TIMEZONE_KW + "=" + timezone + "\n"
                        th.write(new_line)
                        saved_tz = True
                    else:
                        th.write(line)
                if not saved_tz:
                    new_line = TIMEZONE_KW + "=" + timezone + "\n"
                    th.write(new_line)

		th.close()

                # Set the owner, group and permission bits from original file
                # to temp file.  The copystat() call will cause the last
                # access and modification time as well, but it is not
                # important.  Capturing the correct file permission is.
                shutil.copymode(init_file, tmp_fname)
                shutil.copystat(init_file, tmp_fname)
                shutil.copy2(tmp_fname, init_file)
                os.remove(tmp_fname)
    except IOError, ioe:
        logging.error("Failed to save timezone into %s", init_file)
        logging.exception(ioe)
        raise InstallationError

VFSTAB_FILE = "/etc/vfstab"
def setup_etc_vfstab_for_swap(swap_device, basedir):
    '''Add the swap device to /etc/vfstab.

    '''
    if swap_device is None:
        return    #nothing to do

    fname = basedir + VFSTAB_FILE
    try:
        with open (fname, 'a+') as vf:
            vf.write("%s\t%s\t\t%s\t\t%s\t%s\t%s\t%s\n" % 
                        (swap_device, "-", "-", "swap", "-", "no", "-"))
    except IOError, ioe:
        logging.error("Failed to write to %s", fname)
        logging.exception(ioe)
        raise InstallationError

def pool_list(arg):
    '''Return a list of zpools on the system

    '''
    argslist = ['/usr/sbin/zpool', arg]
    pool_names = []

    try:
        (zpoolout, zpoolerr) = Popen(argslist, stdout=PIPE,
                  stderr=PIPE).communicate()

    except OSError, err:
        logging.error("OSError occured during zpool call: %s", err)
        return pool_names

    if zpoolerr:
        logging.error("Error occured during zpool call: %s", zpoolerr)
        return pool_names

    line = zpoolout.splitlines(False)
    for entry in line:
        if 'pool:' in entry:
            val = entry.split()
            if ((len(val) == 2) and (val[0] == "pool:")):
                pool_names.append(val[1])

    return pool_names

def get_zpool_list():
    ''' Get the list of exported (inactive) as well as active
    zpools on the system.
    '''
    zpool_list = pool_list("import")
    zpool_list.extend(pool_list("status"))
    
    return zpool_list

def get_zpool_free_size(name):
    '''Return free size, available for pool's top-level filesystem.
    Pool is imported if necessary
    '''
    
    status = exec_cmd_outputs_to_log(["/usr/sbin/zpool", "list", name], logging)
    if status != 0:
        status = exec_cmd_outputs_to_log(["/usr/sbin/zpool", "import",
                                          "-N", name], logging)
        if status != 0:
            logging.warning(("Couldn't import pool %s, "
                             "you can try to run "
                             "\"zpool import -Nf %s\" "
                             "before proceeding"), name, name)

    if status == 0:
        try:
            argslist = ["/usr/sbin/zfs", "get", "-Hp", "-o", "value",
                        "available", name]
            (zfsout, zfserr) = Popen(argslist, stdout=PIPE,
       	          stderr=PIPE).communicate()
        except OSError, err:
            logging.error("OSError occured during zfs call: %s", err)
            return -1

        if zfserr:
            logging.error("Error occured during zpool call: %s", zfserr)
            return -1

        return int(zfsout)
        
    return 0

def get_zpool_be_names(name):
    ''' Return list of names which seem to be names of zpool boot environments.
    Pool is imported if necessary
    '''
    be_names = list()
    prefix = "%s/ROOT" % (name)
    status = exec_cmd_outputs_to_log(["/usr/sbin/zpool", "list", name], logging)
    if status != 0:
        status = exec_cmd_outputs_to_log(["/usr/sbin/zpool", "import",
                                          "-N", name], logging)
        if status != 0:
            logging.warning(("Couldn't import pool %s, "
                             "you can try to run "
                             "\"zpool import -Nf %s\" "
                             "before proceeding"), name, name)

    if status == 0:
        try:
            argslist = ["/usr/sbin/zfs", "list", "-t", "filesystem", "-d", "1",
                        "-H", "-o","name", "-r", prefix ]
            (zfsout, zfserr) = Popen(argslist, stdout=PIPE,
                  stderr=PIPE).communicate()
        except OSError, err:
            logging.error("OSError occured during zfs call: %s", err)
            return be_names

        if zfserr:
            logging.error("Error occured during zfs call: %s", zfserr)
            return be_names


        line = zfsout.splitlines(False)
        for entry in line:
            prefix_slash = "%s/" % (prefix)
            if entry.startswith(prefix_slash):
                be_names.append(entry.split(prefix_slash)[1])

    return be_names

def export_zpools():
    ''' Export all imported pools

    '''
    zpool_list = pool_list("status")
    for name in zpool_list:
        exec_cmd_outputs_to_log(["/usr/sbin/zpool", "export", name], logging)
