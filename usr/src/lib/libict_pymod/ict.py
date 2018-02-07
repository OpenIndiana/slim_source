#!/usr/bin/python2.7
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
# Copyright (c) 2008, 2010, Oracle and/or its affiliates. All rights reserved.
#
'''Install Completion Tasks (ICT)

Each ICT is implemented as a method of class ict.

Guide to calling ICTs

The most efficient way to invoke ICTs is to create an ict class instance,
then to invoke the ICT methods of the ict class:
    - create an ict class instance, providing at least root target directory
    - call ICT as a method of the class instance just created
    - returned status is a tuple of exit status code and any other return
      information
    - logging is done by ICT, but you can return exit status to caller
    - logging service options are inherited from the environment
    - logging level can be set (second parameter) and overridden in environment
        LS_DBG_LVL=(1-4)
    - do not abort if an ICT fails (unless it is an untenable situation)

ICTs can also be invoked singly through a command line using function
exec_ict():

    python ict.py <ICT method> <target directory>  [<ICT-specific parameter>]
    Example command line:
        python ict.py ict_test /a  #runs test ICT

Guide to writing ICTs

- Locate ict class (line starts with "def ict")
- Within the ict class, find "end Install Completion Tasks"
- Add new ICT as method just before that
- ICTs are methods of the ict class
- ICTs have 'self' as 1st parameter - add other parameters as desired
- Create error return code(s)
- ICTs should return either status code, or a tuple with status code first
- See __init__ method for initialization of class members
- self.basedir is the root directory of the target
- for first line of ICT, use _register_task(inspect.currentframe())
    as a trace aid
- do not allow unhandled exceptions
- if a critical condition is encountered, log error and sys.exit()
    with error status
- as a last exception handler, instead of raising the exception,
    log the traceback,

    e.g.:
        except StandardError:
                prerror('Unexpected error doing something.')
                prerror(traceback.format_exc())

- use the module utility routines:
    prerror(string) - log error message
    _dbg_msg(string) - log output debugging message
    info_msg(string) - log informational message
    _cmd_out(cmd) - execute shell command, returning only exit status
    _cmd_status(cmd) - execute shell command, returning exit status and stdout

- place ICT comments just before or after def statement for module
   pydoc to generate documentation

Skeleton ICT:

from osol_install.ict import *
icto = ict('/a')
status = icto.<some ICT (class method)>(<parameters depend on ICT>)

ICT initial project tasks from Transfer Module (TM):
        Setting default keyboard layout TM
        Creating initial SMF repository TM
        Creating /etc/mnttab TM
        Cleanup unnecessary symbolic links and files from the alternate root.
        (clobber files) TM
'''
import errno
import os
import os.path
import sys

from stat import S_IREAD, \
     S_IWRITE, \
     S_IEXEC, \
     S_IRUSR, \
     S_IWUSR, \
     S_IRGRP, \
     S_IXGRP, \
     S_IROTH, \
     S_IXOTH, \
     S_ISLNK

import fcntl
import array
import struct
import shutil
import tempfile

import inspect
import filecmp
import traceback
import re
import platform
import signal
import commands

from pkg.cfgfiles import PasswordFile, UserattrFile

from osol_install.liblogsvc import LS_DBGLVL_ERR, \
LS_DBGLVL_INFO, \
init_log, \
write_dbg, \
write_log

ICTID = 'ICT'
(
ICT_INVALID_PARAMETER,
ICT_INVALID_PLATFORM,
ICT_NOT_MULTIBOOT,
ICT_ADD_FAILSAFE_MENU_FAILED,
ICT_KIOCLAYOUT_FAILED,
ICT_OPEN_KEYBOARD_DEVICE_FAILED,
ICT_KBD_LAYOUT_NAME_NOT_FOUND,
ICT_UPDATE_BOOTPROP_FAILED,
ICT_MKMENU_FAILED,
ICT_SPLASH_IMAGE_FAILURE,
ICT_REMOVE_LIVECD_COREADM_CONF_FAILURE,
ICT_SET_BOOT_ACTIVE_TEMP_FILE_FAILURE,
ICT_FDISK_FAILED,
ICT_UPDATE_DUMPADM_NODENAME_FAILED,
ICT_CONFIGURE_NWAM_FAILED,
ICT_ENABLE_NWAM_FAILED,
ICT_FIX_FAILSAFE_MENU_FAILED,
ICT_CREATE_SMF_REPO_FAILED,
ICT_CREATE_MNTTAB_FAILED,
ICT_PACKAGE_REMOVAL_FAILED,
ICT_DELETE_BOOT_PROPERTY_FAILURE,
ICT_GET_ROOTDEV_LIST_FAILED,
ICT_SETUP_DEV_NAMESPACE_FAILED,
ICT_UPDATE_ARCHIVE_FAILED,
ICT_COPY_SPLASH_XPM_FAILED,
ICT_SMF_CORRECT_SYS_PROFILE_FAILED,
ICT_REMOVE_BOOTPATH_FAILED,
ICT_ADD_SPLASH_IMAGE_FAILED,
ICT_SET_FLUSH_CONTENT_CACHE_ON_SUCCESS_FAILED,
ICT_FIX_GRUB_ENTRY_FAILED,
ICT_CREATE_SPARC_BOOT_MENU_FAILED,
ICT_COPY_SPARC_BOOTLST_FAILED,
ICT_CLOBBER_FILE_FAILED,
ICT_CLEANUP_FAILED,
ICT_REBUILD_PKG_INDEX_FAILED,
ICT_PKG_RESET_UUID_FAILED,
ICT_PKG_SEND_UUID_FAILED,
ICT_SET_SWAP_AS_DUMP_FAILED,
ICT_EXPLICIT_BOOTFS_FAILED,
ICT_ENABLE_HAPPY_FACE_BOOT_FAILED,
ICT_POPEN_FAILED,
ICT_REMOVE_LIVECD_ENVIRONMENT_FAILED,
ICT_SET_ROOT_PW_FAILED,
ICT_CREATE_NU_FAILED,
ICT_OPEN_PROM_DEVICE_FAILED,
ICT_IOCTL_PROM_FAILED,
ICT_SET_PART_ACTIVE_FAILED,
ICT_SVCCFG_FAILURE,
ICT_SET_AUTOHOME_FAILED,
ICT_COPY_CAPABILITY_FAILED,
ICT_APPLY_SYSCONFIG_FAILED,
ICT_GENERATE_SC_PROFILE_FAILED,
ICT_SETUP_RBAC_FAILED,
ICT_SETUP_SUDO_FAILED,
ICT_COPY_GENERATED_FILES_FAILED,
) = range(200,255)

# Global variables
DEBUGLVL = LS_DBGLVL_ERR
CUR_ICT_FRAME = None  # frame info for debugging and tracing
MENU_LST_DEFAULT_TITLE = "OpenIndiana"


#Module functions - intended for local use, but usable by importers
def _register_task(fm):
    '''register current ICT for logging, debugging and tracing
    By convention, use as 1st executable line in ICT
    '''
    global CUR_ICT_FRAME

    CUR_ICT_FRAME = fm
    if CUR_ICT_FRAME != None:
        cf = inspect.getframeinfo(CUR_ICT_FRAME)
        write_log(ICTID, 'current task:' + cf[2] + '\n')


def prerror(msg):
    '''Log an error message to logging service and stderr
    '''
    msg1 = msg + "\n"
    write_dbg(ICTID, LS_DBGLVL_ERR, msg1)


def _move_in_updated_config_file(new, orig):
    '''move in new version of file to original file location,
    overwriting original
    side effect: deletes temporary file upon failure
    '''
    # if files are identical
    if os.path.exists(new) and os.path.exists(orig) and filecmp.cmp(new, orig):
        _delete_temporary_file(new)
        return True
    try:
        shutil.copyfile(new, orig)
        os.remove(new)
    except IOError:
        prerror('IO error - cannot move file ' + new + ' to ' + orig)
        prerror(traceback.format_exc())
        _delete_temporary_file(new)
        return False
    except StandardError:
        prerror('Unrecognized error - failure to move file ' + new +
                ' to ' + orig)
        prerror(traceback.format_exc())
        _delete_temporary_file(new)
        return False
    return True


def _cmd_out(cmd):
    '''execute a shell command and return output
    cmd - command to execute
    returns tuple:
            status = command exit status
            dfout = array of lines output to stdout, stderr by command
    '''
    _dbg_msg('_cmd_out: executing cmd=' + cmd)
    status = 0
    dfout = []
    '''Since Python ignores SIGPIPE, according to Python issue 1652,
    UNIX scripts in subprocesses will also ignore SIGPIPE.
    Workaround is to save original signal handler, restore default handler,
    launch script, restore original signal handler
    '''
    #save SIGPIPE signal handler
    orig_sigpipe = signal.getsignal(signal.SIGPIPE)
    #restore default signal handler for SIGPIPE
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    try:
        fp = os.popen(cmd)
        if fp == None or fp == -1:
            return ICT_POPEN_FAILED, []
        for rline in fp:
            if rline and rline.endswith('\n'):
                rline = rline[:-1]
            dfout.append(rline)
            _dbg_msg('_cmd_out: stdout/stderr line=' + rline)
        status = fp.close()
    except StandardError:
        prerror('system error in launching shell cmd (' + cmd + ')')
        status = 1
    #restore original signal handler for SIGPIPE
    signal.signal(signal.SIGPIPE, orig_sigpipe)
    if status == None:
        status = 0
    if status != 0:
        write_log(ICTID, 'shell cmd (' + cmd + ') returned status ' +
                  str(status) + "\n")
    if DEBUGLVL >= LS_DBGLVL_INFO:
        print ICTID + ': _cmd_out status =', status, 'stdout/stderr=', dfout

    return status, dfout


def _cmd_status(cmd):
    '''execute a shell command using popen and return its exit status
    '''
    _dbg_msg('_cmd_status: executing cmd=' + cmd)
    exitstatus = None

    '''Since Python ignores SIGPIPE, according to Python issue 1652,
    UNIX scripts in subprocesses will also ignore SIGPIPE.
    Workaround is to save original signal handler, restore default handler,
    launch script, restore original signal handler
    '''
    #save SIGPIPE signal handler
    orig_sigpipe = signal.getsignal(signal.SIGPIPE)
    #restore default signal handler for SIGPIPE
    signal.signal(signal.SIGPIPE, signal.SIG_DFL)
    try:
        fp = os.popen(cmd)
        if fp == None or fp == -1:
            return ICT_POPEN_FAILED
        exitstatus = fp.close()
    except StandardError:
        prerror('unknown error in launching shell cmd (' + cmd + ')')
        prerror('Traceback:')
        prerror(traceback.format_exc())
        exitstatus = 1
    if exitstatus == None:
        exitstatus = 0
    # restore original signal handler for SIGPIPE
    signal.signal(signal.SIGPIPE, orig_sigpipe)
    _dbg_msg('_cmd_status: return exitstatus=' + str(exitstatus))
    return exitstatus

def get_net_size(netmask):
     binary_str = ''
     na = netmask.split(".")
     for octet in na:
         binary_str += bin(int(octet))[2:].zfill(8)
     return str(len(binary_str.rstrip('0')))



def info_msg(msg):
    '''
    send an informational message to logging service
    '''
    write_log(ICTID, msg + '\n')


#send informational debugging message to logging service, according to level
def _dbg_msg(msg):
    '''send informational debugging message to logging service,
    according to level
    '''
    if (DEBUGLVL >= LS_DBGLVL_INFO):
        write_dbg(ICTID, LS_DBGLVL_INFO, msg + '\n')


def _delete_temporary_file(filename):
    '''
    delete temporary file - suppress traceback on error
    '''
    try:
        os.unlink(filename)
    except StandardError:
        pass # ignore failure to delete temp file

def correct_rdsk(devname):
    '''
    appends s0 to devname if it ends with dX
    '''
    n = len(devname)-1
    while (n>0 and devname[n].isdigit()):
        n -= 1
    if n>=0 and devname[n] == "d":
        result = devname + "s0"
    else:
        result = devname
    return result

class ICT(object):
    '''main class to support ICT
    ICT object must first be created and initialized
        basedir - root directory (only required parameter)
        debuglvl - debugging message level for liblogsvc
        bootenvrc - normal location of bootenv.rc
        loc_grubmenu - normal location of GRUB menu
        ai_sc_profile - SC profile generated by Automated Installer
        target_sc_profile = target SC profile
        sudoers - normal location of sudo configuration file


    class initializer will exit with error status if:
        - basedir is missing or empty
        - basedir is '/', in order to protect against accidental usage.
          For a live system this should be permitted.

    '''
    def __init__(self, basedir,
        debuglvl=-1,
        bootenvrc='/boot/solaris/bootenv.rc',
        loc_grubmenu='/boot/grub/menu.lst',
        ai_sc_profile='/tmp/sc_manifest.xml',
        target_sc_profile='sc_profile.xml',
        sudoers='/etc/sudoers'):

        # determine whether we are doing AI install or slim install
        self.livecd_install = False
        self.text_install = False
        if os.access("/.livecd", os.R_OK):
            _dbg_msg('Determined to be doing Live CD install')
            self.livecd_install = True
        elif os.access("/.textinstall", os.R_OK):
            _dbg_msg("Determined to be doing Text Install")
            self.text_install = True

        if basedir == '':
            err_str = 'Base directory must be passed'
            prerror(err_str)
            raise ValueError(err_str)
        if basedir == '/':
            '''
            The code can be run on a live system but if we're not
            on a live system we should not support / for BASEDIR.
            '''
            if self.livecd_install or self.text_install:
                err_str = 'Base directory cannot be root ' + \
                    '("/") during install'
                prerror(err_str)
                raise ValueError(err_str)

        self.basedir = basedir
        '''
        If we're running outside of an install we should not use
        the basedir here since that could be the mountpoint of a
        pool that we're creating the menu.lst file on.
        '''
        if self.livecd_install or self.text_install:
            self.prependdir = basedir
        else:
            self.prependdir = ""

        self.bootenvrc = self.prependdir + bootenvrc

        #Is the current platform a SPARC system?
        self.is_sparc = (platform.platform().find('sparc') >= 0)

        global DEBUGLVL
        try:
            DEBUGLVL = int(os.getenv('LS_DBG_LVL', -1))
        except StandardError:
            prerror('Could not parse enviroment variable LS_DBG_LVL to ' +
                    'integer')
            DEBUGLVL = -1

        if DEBUGLVL == -1:
            DEBUGLVL = debuglvl
        if DEBUGLVL == -1:
            DEBUGLVL = LS_DBGLVL_ERR #default logging
        else:
            if DEBUGLVL != LS_DBGLVL_ERR and \
                init_log(DEBUGLVL) != 1: #set in logging service
                prerror('Setting logging service debug level to ' +
                    str(DEBUGLVL) + ' failed.')

        self.kbd_device = '/dev/kbd'
        self.kbd_layout_file = '/usr/share/lib/keytables/type_6/kbd_layouts'
        self.keyboard_layout = ''

        # determine whether we are installing to an iSCSI boot target
        self.iscsi_boot_install = False
        if os.access("/.iscsi_boot", os.R_OK):
            _dbg_msg('Determined to be doing iSCSI boot install')
            self.iscsi_boot_install = True

        #take root poolname from mnttab
        # Note there are TABs in the blow expression.
        # cmd = 'grep "^[^<TAB>]*<TAB>' + basedir +<TAB>' " /etc/mnttab | ' + \
        cmd = 'grep "^[^	]*	' + basedir + '	" /etc/mnttab | ' + \
              ' nawk \'{print $1}\' | sed \'s,/.*,,\''
        sts, rpa = _cmd_out(cmd)

        if len(rpa) == 0:
            prerror('Cannot determine root pool name. exit status=' +
                    str(sts) + ' command=' + cmd)
            sys.exit(ICT_GET_ROOTDEV_LIST_FAILED)
        self.rootpool = rpa[0]
        _dbg_msg('Root pool name discovered: ' + self.rootpool)

        if self.livecd_install or self.text_install:
            #With the root pool pre-pended to /boot/grub/menu.lst
            self.grubmenu = '/' + self.rootpool + loc_grubmenu
            self.bootmenu_path_sparc = '/' + self.rootpool + '/boot'
        else:
            #With the basedir pre-pended to /boot/grub/menu.lst
            self.grubmenu = basedir + loc_grubmenu
            #With the basedir pre-pended for the SPARC boot menu
            self.bootmenu_path_sparc = basedir + '/boot'

        #/boot/menu.lst
        self.bootmenu_sparc = self.bootmenu_path_sparc + '/menu.lst'

        self.sudoers = basedir + sudoers

        # System Configuration template used to assemble System Configuration
        # profile
        self.sc_template = '/usr/share/install/sc_template.xml'
        # path to System Configuration profile generated by Automated Installer
        self.ai_sc_profile = ai_sc_profile
        # name of target System Configuration profile
        self.sc_profile = target_sc_profile
        # files to copy from home directory
        self.home_generated_files = [ '.dmrc' ]

    #support methods
    def _get_bootprop(self, property_id):
        '''support method - get property from bootenv.rc
        Parameter: property_id - bootenv.rc property ID
        The format of a line in bootenvrc is:
        # followed by a comment
            or
        setprop <prop name> <prop value>

        returns property value or '' if not found
        '''
        fp = open(self.bootenvrc)
        for rline in fp:
            # Ignore comment lines
            if rline.startswith('#'):
                continue
            try:
                # Store the property name in field
                # and the property value in value.
                (field, value) = rline.split()[1:3]
            except StandardError:
                continue
            if field == property_id:
                fp.close()
                return value
        fp.close()
        return ''

    def _delete_bootprop(self, property_id):
        '''support method  - from bootenv.rc, delete property
        Parameter: property_id - bootenv.rc property ID
        return 0 for success, otherwise ICT failure status code
        '''
        new_rc = self.bootenvrc + '.new'
        try:
            fp = open(self.bootenvrc)
            op = open(new_rc, 'w')
            for rline in op:
                if rline.startswith('#'):
                    op.write(rline)
                    continue
                try:
                    # Assign to field the token between seperator 1 and 2,
                    # this being the second token.
                    field = rline.split()[1:2]
                except ValueError:
                    op.write(rline)
                    continue
                if field == property_id:
                    continue
                op.write(rline)
            fp.close()
            op.close()
            os.rename(new_rc, self.bootenvrc)
        except OSError, (errno, strerror):
            prerror('Error in deleting property in ' + self.bootenvrc +
                    ': ' + strerror)
            prerror('Failure. Returning: ICT_DELETE_BOOT_PROPERTY_FAILURE')
            return ICT_DELETE_BOOT_PROPERTY_FAILURE
        except StandardError:
            prerror('Unexpected error when deleting property in ' +
                    self.bootenvrc)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_DELETE_BOOT_PROPERTY_FAILURE')
            return ICT_DELETE_BOOT_PROPERTY_FAILURE
        return 0

    def _update_bootprop(self, property_id, newvalue):
        '''support method - set bootenv.rc property_id to value
        Parameters:
                property_id - bootenv.rc property ID
                newvalue - value to assign to property_id
        return 0 for success or ICT status code
        '''
        new_rc = self.bootenvrc + '.new'
        return_status = 0
        fp = op = None
        try:
            fp = open(self.bootenvrc)
            op = open(new_rc, 'w')
            #copy all lines that do not contain the property_id
            for rline in fp:
                if rline.startswith('#'):
                    op.write(rline)
                    continue
                try:
                    # Assign to field the token between seperator 1 and 2,
                    # this being the second token.
                    field = rline.split()[1:2]
                except ValueError:
                    op.write(rline) #just copy
                    continue
                if field != property_id:
                    op.write(rline)

            #add the line with the updated property_id
            op.write('setprop ' + property_id + ' ' + newvalue + '\n')
            os.rename(new_rc, self.bootenvrc)
        except OSError, (errno, strerror):
            prerror('Update boot property failed. ' + strerror + ' file=' +
                    self.bootenvrc + ' property=' + property_id +
                    ' value=' + newvalue)
            prerror('Failure. Returning: ICT_UPDATE_BOOTPROP_FAILED')
            return_status = ICT_UPDATE_BOOTPROP_FAILED
        except StandardError:
            prerror('Unexpected error when updating boot property. file=' +
                    self.bootenvrc +
                    ' property=' + property_id + ' value=' + newvalue)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_UPDATE_BOOTPROP_FAILED')
            return_status = ICT_UPDATE_BOOTPROP_FAILED
        if fp != None:
            fp.close()
        if op != None:
            op.close()
        return return_status

    @staticmethod
    def set_boot_active(raw_slice):
        '''support method - set boot device as active in fdisk disk formatter
        Parameter: raw_slice is a /dev path
        launches fdisk -F <format file> /dev/rdsk/cXtXdXpX
         on partitions if changes required

        return 0 upon success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        mtch = re.findall('p(\d+):boot$', raw_slice)
        if mtch and mtch[0]:
            p0 = raw_slice.replace('p' + mtch[0] + ':boot', 'p0')
        else:
            mtch = re.findall('s(\d+)$', raw_slice)
            if mtch and mtch[0]:
                p0 = raw_slice.replace('s' + mtch[0], 'p0')
            else:
                p0 = raw_slice

        # Note there are TABs in the blow expression.
        # cmd = 'fdisk -W - %s | grep -v \* | grep -v \'^[<TAB> ]*$\'' % (p0)
        cmd = 'fdisk -W - %s | grep -v \* | grep -v \'^[	 ]*$\'' % (p0)
        status, fdisk = _cmd_out(cmd)
        if status != 0:
            prerror('fdisk command fails to set ' + raw_slice + 
                    ' active. exit status=' + str(status))
            prerror('command was ' + cmd)
            prerror('Failure. Returning: ICT_FDISK_FAILED')
            return ICT_FDISK_FAILED
        # make sure there is a Solaris partition before doing anything
        has_solaris_systid = has_solaris_2_systid = False
        for ln in fdisk:
            if ln[0] == '*':
                continue
            cols = ln.split()
            if len(cols) < 2:
                continue
            if not has_solaris_systid:
                has_solaris_systid = (cols[0] == '130')
            if not has_solaris_2_systid:
                has_solaris_2_systid = (cols[0] == '191')
        if not has_solaris_systid and not has_solaris_2_systid:
            return 0 # no changes
        fdiskout = []
        made_fdisk_changes = False
        dont_change_active = False
        partition_number = 1
        for ln in fdisk:
            if ln[0] == '*':
                continue
            cols = ln.split()
            if len(cols) < 2:
                continue
            if has_solaris_2_systid:
                if cols[0] == '191':
                    #don't change active partiton if installing to logical
                    if partition_number > 4:
                        dont_change_active = True
                    if cols[1] != '128':
                        cols[1] = '128' #active partition
                        made_fdisk_changes = True
                else:
                    if cols[1] != '0':
                        cols[1] = '0'
                        made_fdisk_changes = True
            else: #systid Linux swap
                if cols[0] == '130':
                    if cols[1] != '128':
                        cols[1] = '128' #active partition
                        made_fdisk_changes = True
                else:
                    if cols[1] != '0':
                        cols[1] = '0'
                        made_fdisk_changes = True
            lnout = '  '
            for lno in cols:
                lnout += lno.ljust(6) + ' '
            lnout += '\n'
            fdiskout.append(lnout)
            partition_number = partition_number + 1
        if dont_change_active:
            _dbg_msg('Install partition is logical partition.'
                     ' Active partition not changed.')
            return 0
        if not made_fdisk_changes:
            _dbg_msg('No disk format changes - fdisk not run')
            return 0
        _dbg_msg('Disk format changes needed - fdisk will be run.')
        #write fdisk format to temporary file
        try:
            (fop, fdisk_tempfile) = tempfile.mkstemp('.txt', 'fdisk', '/tmp')
            for ln in fdiskout:
                os.write(fop, ln)
            os.close(fop)
        except OSError, (errno, strerror):
            prerror('Error in writing to temporary file. ' + strerror)
            prerror('Failure. Returning: ' +
                    'ICT_SET_BOOT_ACTIVE_TEMP_FILE_FAILURE')
            return ICT_SET_BOOT_ACTIVE_TEMP_FILE_FAILURE
        except StandardError:
            prerror('Unexpected error in writing to temporary file. ' +
                    strerror)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ' +
                    'ICT_SET_BOOT_ACTIVE_TEMP_FILE_FAILURE')
            return ICT_SET_BOOT_ACTIVE_TEMP_FILE_FAILURE
        cmd = 'fdisk -F %s %s 2>&1' % (fdisk_tempfile, p0)
        status = _cmd_status(cmd)
        #delete temporary file
        try:
            os.unlink(fdisk_tempfile)
        except OSError:
            pass # ignore failure to delete temporary file
        if status != 0:
            prerror('Error executing ' + cmd + '. exit status=' + str(status))
            prerror('Failure. Returning: ICT_FDISK_FAILED')
            return ICT_FDISK_FAILED
        return 0

    def _get_osconsole(self):
        '''support method - determine console device
        returns console device found in bootenv.rc, from prtconf command,
        or default 'text'
        
        If osconsole is not set (initial/flash install), we set it here based
        on what the current console device is.
        '''
        osconsole = self._get_bootprop('output-device')
        if osconsole != '':
            return osconsole

        # get console setting from prtconf command - console
        cmd = 'prtconf -v /devices | ' + \
            'sed -n \'/console/{n;p;}\' | cut -f 2 -d \\\''
        sts, co = _cmd_out(cmd)
        if sts != 0:
            prerror('Error from command to get console. exit status=' +
                    str(sts))
            prerror('Command in error=' + cmd)
        if len(co) > 0:
            osconsole = co[0]
        if osconsole == '':
            # get console setting from prtconf command - output-device
            cmd = 'prtconf -v /devices | ' + \
                'sed -n \'/output-device/{n;p;}\' | cut -f 2 -d \\\''
            sts, co = _cmd_out(cmd)
            if sts != 0:
                prerror('Error from command to get console. exit status=' +
                        str(sts))
                prerror('Command in error=' + cmd)
            if len(co) > 0:
                osconsole = co[0]
            if osconsole == 'screen':
                osconsole = 'text'
        # default console to text
        if osconsole == '':
            osconsole = 'text'
        return osconsole

    def get_rootdev_list(self, dev_path):
        '''ICT and support method - get list of disks with zpools
        associated with root pool launch zpool iostat -v + rootpool

        dev_path is a Solaris /dev disk directory path
        (e.g. /dev/dsk or /dev/rdsk)
        return tuple:
                status - 0 for success, error code otherwise
                device list - list of device names: <dev_path>/cXtXdXsX,
                empty list if failure
        '''
        _register_task(inspect.currentframe())
        cmd = 'zpool iostat -v ' + self.rootpool
        sts, zpool_iostat = _cmd_out(cmd)
        if sts != 0:
            prerror('Error from command to get rootdev list. exit status=' +
                    str(sts))
            prerror('Command in error=' + cmd)
            prerror('Failure. Returning: ICT_GET_ROOTDEV_LIST_FAILED')
            return ICT_GET_ROOTDEV_LIST_FAILED, []
        i = 0
        rootdevlist = []
        while i < len(zpool_iostat):
            p1 = zpool_iostat[i].split()
            if len(p1) > 1 and p1[0] == self.rootpool:
                i += 1
                while i < len(zpool_iostat):
                    if len(zpool_iostat[i]) > 1 and zpool_iostat[i][0] == ' ':
                        la = zpool_iostat[i].split()
                        if len(la) > 1 and la[0] != 'mirror' and \
                            not la[0].startswith('raidz') and la[0][0] != '-':
                            rootdevlist.append(dev_path + la[0])
                    i += 1
            i += 1
        return 0, rootdevlist

    def _get_kbd_layout_name(self, layout_number):
        '''support method - given a keyboard layout number return the
        layout string.

        parameter layout_number - keyboard layout number from:
                /usr/share/lib/keytables/type_6/kbd_layouts
        We should not be doing this here, but unfortunately there
        is no interface in the keyboard API to perform
        this mapping for us - RFE.'''
        try:
            fh = open(self.kbd_layout_file, "r")
        except StandardError:
            prerror('keyboard layout file open failure: filename=' +
                self.kbd_layout_file)
            return ''
        kbd_layout_name = ''
        for line in fh: #read file until number matches
            if line.startswith('#'):
                continue
            if '=' not in line:
                continue
            (kbd_layout_name, num) = line.split('=')
            if int(num) == layout_number:
                fh.close()
                return kbd_layout_name
        fh.close()
        return ''

    def bootadm_update_menu(self, rdsk):
        '''ICT and support method - add failsafe menu entry for disk
        parameter rdsk - raw disk device name in ctds format:
        /dev/rdsk/cXtXdXsX
        Does bootadm update-menu -R basedir -Z -o <raw disk>
        returns 0 if command succeeded, error code otherwise
        '''
        _register_task(inspect.currentframe())
        cmd = 'bootadm update-menu -R %s -Z -o %s 2>&1' % (self.basedir, correct_rdsk(rdsk))
        info_msg('update GRUB boot menu on device ' + rdsk)
        _dbg_msg('editing GRUB menu: ' + cmd)
        status, cmdout = _cmd_out(cmd)
        if status != 0:
            prerror('Adding failsafe menu with command: %s failed. ' +
                    ' exit status=%d' % (cmd, status))
            for ln in cmdout:
                prerror('bootadm_update_menu output: ' + ln)
            prerror('Failure. Returning: ICT_ADD_FAILSAFE_MENU_FAILED')
            return ICT_ADD_FAILSAFE_MENU_FAILED
        for ln in cmdout:
            info_msg('bootadm_update_menu output: ' + ln)
        return 0

    @staticmethod
    def _get_root_dataset():
        '''support routine - using beadm list, get the root dataset of
        the root pool
        return root dataset active on reboot or '' if not found
        log error if not found
        '''
        _register_task(inspect.currentframe())
        cmd = 'beadm list -aH'
        status, belist = _cmd_out(cmd)
        if status != 0:
            prerror('BE list command %s failed. Exit status=%d' %
                    (cmd, status))
            for msg in belist:
                prerror(msg)
            return ''
        for ln in belist:
            arg = ln.split(';') #parse datasets
            if not arg[2]:
                continue
            if arg[2].find('R') != -1: #check if active on reboot
                _dbg_msg('found root dataset %s ' % arg[1])
                return arg[1]
        return ''

    #end support routines

    #Install Completion Tasks start here

    def remove_bootpath(self):
        '''ICT - no bootpath needed for zfs boot - remove property
        from bootenv.rc
        blatant hack:  _setup_bootblock should be fixed
        in the spmisvc library to not put bootpath in bootenv.rc
        in the first place for zfs boot
        returns 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        newbootenvrc = self.bootenvrc + '.tmp'
        bootpath = self._get_bootprop('bootpath')
        if bootpath != '':
            # Note there are TABs in the blow expression.
            #...'sed \'/^setprop[ <TAB>][ <TAB>]*' +
            #... 'bootpath[ <TAB>]/d\' ' +
            status = _cmd_status('sed \'/^setprop[ 	][ 	]*' +
                                 'bootpath[ 	]/d\' ' +
                                 self.bootenvrc + ' > ' +
                                 self.bootenvrc + '.tmp')
            if status != 0:
                prerror('bootpath not removed from bootenv.rc - ' +
                        ' exit status=' + str(status))
                prerror('Failure. Returning: ICT_REMOVE_BOOTPATH_FAILED')
                return ICT_REMOVE_BOOTPATH_FAILED
            if not _move_in_updated_config_file(newbootenvrc, self.bootenvrc):
                prerror('bootpath not removed from bootenv.rc')
                prerror('Failure. Returning: ICT_REMOVE_BOOTPATH_FAILED')
                return ICT_REMOVE_BOOTPATH_FAILED
        _dbg_msg('bootpath property removed from ' + self.bootenvrc)
        return 0

    def get_keyboard_layout(self):
        '''Get keyboard layout using ioctl KIOCLAYOUT on /dev/kbd
        return 0 for success, otherwise error code
        '''
        #ioctl codes taken from /usr/include/sys/kbio.h
        kioc = ord('k') << 8
        kioclayout = kioc | 20
        _dbg_msg("Opening keyboard device: " + self.kbd_device)
        try:
            kbd = open(self.kbd_device, "r+")
        except StandardError:
            prerror('Failure to open keyboard device ' + self.kbd_device)
            prerror('Failure. Returning: ICT_OPEN_KEYBOARD_DEVICE_FAILED')
            return ICT_OPEN_KEYBOARD_DEVICE_FAILED
        if kbd == None:
            prerror('Failure to open keyboard device ' + self.kbd_device)
            prerror('Failure. Returning: ICT_OPEN_KEYBOARD_DEVICE_FAILED')
            return ICT_OPEN_KEYBOARD_DEVICE_FAILED

        k = array.array('i', [0])
        
        try:
            status = fcntl.ioctl(kbd, kioclayout, k, 1)
        except IOError as err:
            status = err.errno
            if status == errno.EINVAL:
                kbd.close()
                info_msg("Failed to read keyboard device ioctl; Ignoring")
                return 0
        except StandardError:
            status = 1
        
        if status != 0:
            kbd.close()
            prerror('fcntl ioctl KIOCLAYOUT_FAILED: status=' + str(status))
            prerror('Failure. Returning: ICT_KIOCLAYOUT_FAILED')
            return ICT_KIOCLAYOUT_FAILED
        kbd_layout = k.tolist()[0]
        kbd.close()

        self.keyboard_layout = self._get_kbd_layout_name(kbd_layout)

        return 0

    def generate_sc_profile(self):
        ''' ICT - Assemble System Configuration (SC) profile

        Configured parameters:
          * keyboard layout - profile will configure keymap/layout SMF property
            of svc:/system/keymap:default SMF service.
          * enabled display manager - either lightdm or gdm (if lightdm doesn't exist)
        
        Following approach is taken:
         * Take template profile
         * Set value of keymap/layout SMF property to desired value (it is
           configured as 'US-English' in template
         * Store SMF profile into profile directory
           (/etc/svc/profile/)

        return 0 for success, ICT_GENERATE_SC_PROFILE_FAILED in case of failure
        '''
        display_manager='gdm'

        _register_task(inspect.currentframe())

        sc_profile_src = self.basedir + self.sc_template
        sc_profile_dst = self.basedir + '/etc/svc/profile/' + \
                         self.sc_profile

        # Obtain desired keyboard layout.
        if self.get_keyboard_layout() != 0:
            prerror('get_keyboard_layout() failed, Returning: '
                    'ICT_GENERATE_SC_PROFILE_FAILED')
            return ICT_GENERATE_SC_PROFILE_FAILED

        #
        # If keyboard layout has not been identified,
        # go with default setting (US-English)
        #
        if self.keyboard_layout == '':
            info_msg('Keyboard layout has not been identified')
            info_msg('It will be configured to US-English.')
            self.keyboard_layout = 'US-English'

        info_msg('Detected ' + self.keyboard_layout + ' keyboard layout')

        if os.path.isfile(self.basedir + '/usr/sbin/lightdm'):
            display_manager='lightdm' 
            
        status = _cmd_status('/usr/bin/sed -e s/US-English/' + \
                             self.keyboard_layout + '/ ' + \
	                     ' -e s/gdm/' + display_manager + '/ ' + \
                             sc_profile_src + ' > ' + sc_profile_dst)
        if status != 0:
            try:
                os.unlink(sc_profile_dst)
            except OSError:
                pass
            prerror('Failure. Returning: ICT_GENERATE_SC_PROFILE_FAILED')
            return ICT_GENERATE_SC_PROFILE_FAILED

        info_msg('Created System Configuration profile ' + sc_profile_dst)

        return 0

    def delete_misc_trees(self):
        '''ICT - delete miscellanous directory trees used as work areas
        during installation
        always return success
        '''
        _register_task(inspect.currentframe())
        _cmd_status('rm -rf ' + self.basedir + '/var/tmp/*')
        _cmd_status('rm -rf ' + self.basedir + '/mnt/*')
        return 0

    def create_smf_repository(self):
        '''ICT - copies /lib/svc/seed/global.db to /etc/svc/repository.db
        returns 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        src = self.basedir + '/lib/svc/seed/global.db'
        dst = self.basedir + '/etc/svc/repository.db'
        try:
            shutil.copyfile(src, dst)
            os.chmod(dst, S_IRUSR | S_IWUSR)
            os.chown(dst, 0, 3) # chown root:sys
        except OSError, (errno, strerror):
            prerror('Cannot create smf repository due to error in copying ' +
                    src + ' to ' + dst + ': ' + strerror)
            prerror('Failure. Returning: ICT_CREATE_SMF_REPO_FAILED')
            return ICT_CREATE_SMF_REPO_FAILED
        except StandardError:
            prerror('Unrecognized error - cannot create smf repository. ' +
                    'source=' + src + ' destination=' + dst)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_CREATE_SMF_REPO_FAILED')
            return ICT_CREATE_SMF_REPO_FAILED
        return 0

    def create_mnttab(self):
        '''ICT - create /etc/mnttab if it doesn't already exist and chmod
        returns 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        mnttab = self.basedir + '/etc/mnttab'
        try:
            open(mnttab, 'w').close() # equivalent to touch(1)
            os.chmod(mnttab, S_IREAD | S_IRGRP | S_IROTH)
        except OSError, (errno, strerror):
            prerror('Cannot create ' + mnttab + ': ' + strerror)
            prerror('Failure. Returning: ICT_CREATE_MNTTAB_FAILED')
            return ICT_CREATE_MNTTAB_FAILED
        except StandardError:
            prerror('Unrecognized error - Cannot create ' + mnttab)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_CREATE_MNTTAB_FAILED')
            return ICT_CREATE_MNTTAB_FAILED
        return 0

    def add_splash_image_to_grub_menu(self):
        '''ICT - append splashimage and timeout commands to GRUB menu
        If console is redirected to serial line, don't enable GRUB
        splash screen.
        return 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        grubmenu = self.grubmenu
        try:
            fp = open(self.grubmenu, 'a+')
            if ((self._get_osconsole() == 'text') or
                (self._get_osconsole() == 'graphics')):

                fp.write('splashimage /boot/grub/splash.xpm.gz\n')
                fp.write('foreground 343434\n')
                fp.write('background F7FbFF\n')
                fp.write('default 0\n')
            else:
                info_msg('Console on serial line, GRUB splash image will ' +
                         'be disabled')

            fp.write('timeout 30\n')
            fp.close()
        except OSError, (errno, strerror):
            prerror('Error in appending splash image grub commands to ' +
                    grubmenu + ': ' + strerror)
            prerror('Failure. Returning: ICT_ADD_SPLASH_IMAGE_FAILED')
            return ICT_ADD_SPLASH_IMAGE_FAILED
        except StandardError:
            prerror('Unrecognized error in appending splash image grub ' +
                    'commands to ' + grubmenu)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_ADD_SPLASH_IMAGE_FAILED')
            return ICT_ADD_SPLASH_IMAGE_FAILED
        return 0

    def update_dumpadm_nodename(self):
        '''ICT - Update nodename in dumpadm.conf
        Note: This is just temporary solution, as dumpadm(1M) -r option\
        does not work.
        This issue is tracked by Bugster CR 6835106. Once this bug is fixed,
        dumpadm(1M) -r should be used for manipulating /etc/dumpadm.conf
        instead.

        returns 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        nodename = self.basedir + '/etc/nodename'
        dumpadmfile = '/etc/dumpadm.conf'
        dumpadmfile_dest = self.basedir + dumpadmfile
        try:
            fnode = open(nodename, 'r')
            na = fnode.readlines()
            fnode.close()
        except OSError, (errno, strerror):
            prerror('Error in accessing ' + nodename + ': ' + strerror)
            prerror('Failure. Returning: ICT_UPDATE_DUMPADM_NODENAME_FAILED')
            return ICT_UPDATE_DUMPADM_NODENAME_FAILED
        except StandardError:
            prerror('Unrecognized error in accessing ' + nodename)
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_UPDATE_DUMPADM_NODENAME_FAILED')
            return ICT_UPDATE_DUMPADM_NODENAME_FAILED
        nodename = na[0][:-1]

        status = _cmd_status('cat ' + dumpadmfile + ' | ' +
            'sed s/openindiana/' + nodename + '/ > ' + dumpadmfile_dest)
        if status != 0:
            try:
                os.unlink(dumpadmfile_dest)
            except OSError:
                pass
            prerror('Failure. Returning: ICT_UPDATE_DUMPADM_NODENAME_FAILED')
            return ICT_UPDATE_DUMPADM_NODENAME_FAILED

        return 0

    def explicit_bootfs(self):
        '''ICT - For libbe to be able to support the initial boot environment,
        we need an explicit bootfs value in our menu entry.  Add it
        to the entry before the ZFS-BOOTFS line.  This, along with the
        rest of the grub menu entry manipulation code in this file, will
        eventually need to get ripped out when we have support in libbe
        to create and activate the grub entry for the initial boot
        environment.
        returns 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        rootdataset = self._get_root_dataset()
        if rootdataset == '':
            prerror('Could not determine root dataset')
            prerror('Failure. Returning: ICT_EXPLICIT_BOOTFS_FAILED')
            return ICT_EXPLICIT_BOOTFS_FAILED
        newgrubmenu = self.grubmenu + '.new'

        # Note there are TABs in the blow expression.
        # sedcmd = 'sed \'/\-B[ <TAB>]*\\$ZFS-BOOTFS/ i\\\nbootfs ' +\
        sedcmd = 'sed \'/\-B[ 	]*\\$ZFS-BOOTFS/ i\\\nbootfs ' +\
            rootdataset + '\\\n\' ' + self.grubmenu + ' > ' + newgrubmenu
        status = _cmd_status(sedcmd)
        if status != 0:
            prerror('Adding bootfs command to grub menu fails. ' +
                    'exit status=' + str(status))
            prerror('Failure. Returning: ICT_EXPLICIT_BOOTFS_FAILED')
            return ICT_EXPLICIT_BOOTFS_FAILED
        try:
            shutil.copyfile(newgrubmenu, self.grubmenu)
            os.remove(newgrubmenu)
        except OSError, (errno, strerror):
            prerror('Moving GRUB menu ' + newgrubmenu + ' to ' +
                    self.grubmenu + ' failed. ' + strerror)
            prerror('Failure. Returning: ICT_EXPLICIT_BOOTFS_FAILED')
            return ICT_EXPLICIT_BOOTFS_FAILED
        except StandardError:
            prerror('Unrecognized error - cannot move GRUB menu ' +
                    newgrubmenu + ' to ' + self.grubmenu)
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_EXPLICIT_BOOTFS_FAILED')
            return ICT_EXPLICIT_BOOTFS_FAILED
        return 0

    def enable_happy_face_boot(self):
        '''ICT - Enable happy face boot
        Enable graphical Happy Face boot for the entries in the menu.lst file.
        If console is redirected to serial line, don't enable happy face boot.

        To enable Happy Face boot:
        above the ZFS-BOOTFS line add:
                splashimage /boot/solaris.xpm
                foreground FF0000
                background A8A8A8
        and to the end of the kernel line add: console=graphics

        returns 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        if self._get_osconsole() != 'text':
            info_msg('Console on serial line, happy face boot will' +
                     ' be disabled')
            return 0

        happy_face_splash = 'splashimage /boot/solaris.xpm'
        happy_face_foreground = 'foreground FF0000'
        happy_face_background = 'background A8A8A8'

        newgrubmenu = self.grubmenu + '.new'

        sedcmd = 'sed -e \'/^kernel.*\\$ZFS-BOOTFS/ i\\\n' +\
            happy_face_splash + '\\\n' +\
            happy_face_foreground + '\\\n' + happy_face_background +\
            '\' -e \'s/\\$ZFS-BOOTFS/\\$ZFS-BOOTFS,console=graphics/\' ' +\
            self.grubmenu + ' > ' + newgrubmenu
        status = _cmd_status(sedcmd)
        if status != 0:
            prerror('Adding happy face support to grub menu fails. ' +
                    'exit status=' + int(status))
            prerror('Failure. Returning: ICT_ENABLE_HAPPY_FACE_BOOT_FAILED')
            return ICT_ENABLE_HAPPY_FACE_BOOT_FAILED
        try:
            shutil.copyfile(newgrubmenu, self.grubmenu)
            os.remove(newgrubmenu)
        except OSError, (errno, strerror):
            prerror('Moving GRUB menu ' + newgrubmenu + ' to ' +
                self.grubmenu + ' failed. ' + strerror)
            prerror('Failure. Returning: ICT_ENABLE_HAPPY_FACE_BOOT_FAILED')
            return ICT_ENABLE_HAPPY_FACE_BOOT_FAILED
        except StandardError:
            prerror('Unrecognized error - cannot move GRUB menu ' +
                newgrubmenu + ' to ' + self.grubmenu)
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_ENABLE_HAPPY_FACE_BOOT_FAILED')
            return ICT_ENABLE_HAPPY_FACE_BOOT_FAILED
        return 0

    def setup_dev_namespace(self):
        '''ICT - Setup the dev namespace on the target using devfsadm(1M)
        if installing from IPS.

        Test if installing from IPS.
        launch devfsadm -R basedir
        return 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())

        # launch devfsadm -R basedir
        cmd = '/usr/sbin/devfsadm -R ' + self.basedir + ' 2>&1'
        status, cmdout = _cmd_out(cmd)
        if status != 0:
            prerror('Setting up dev namespace fails. exit status=' +
                    str(status) + ' command=' + cmd)
            prerror('Failure. Returning: ICT_SETUP_DEV_NAMESPACE_FAILED')
            return ICT_SETUP_DEV_NAMESPACE_FAILED
        for ln in cmdout:
            info_msg('devfsadm command output: ' + ln)
        return 0

    def update_boot_archive(self):
        '''ICT - update archive using bootadm(1M)
        launch bootadm update-archive -R basedir
        return 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        cmd = 'bootadm update-archive -R ' + self.basedir + ' 2>&1'
        status, cmdout = _cmd_out(cmd)
        info_msg('bootadm update-archive output: %s' % cmdout)
        if status != 0:
            prerror('Updating boot archive fails. exit status=' +
                    str(status) + ' command=' + cmd)
            prerror('Failure. Returning: ICT_UPDATE_ARCHIVE_FAILED')
            return ICT_UPDATE_ARCHIVE_FAILED
        else:
            return 0

    def copy_splash_xpm(self):
        '''ICT - copy splash file to grub directory in new root pool
        returns 0 for success or ICT status code
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        src = self.basedir + '/boot/grub/splash.xpm.gz'
        dst = '/' + self.rootpool + '/boot/grub/splash.xpm.gz'
        try:
            shutil.copy(src, dst)
        except OSError, (errno, strerror):
            prerror('Copy splash file ' + src + ' to ' + dst +
                    ' failed. ' + strerror)
            prerror('Failure. Returning: ICT_COPY_SPLASH_XPM_FAILED')
            return ICT_COPY_SPLASH_XPM_FAILED
        except StandardError:
            prerror('Unrecognized error - Could not copy splash file ' +
                    src + ' to ' + dst)
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_COPY_SPLASH_XPM_FAILED')
            return ICT_COPY_SPLASH_XPM_FAILED
        return 0

    def smf_correct_sys_profile(self):
        '''ICT - Point SMF at correct system profile
        return 0 if all files deleted and symlinks created,
        error status otherwise
        '''
        _register_task(inspect.currentframe())
        return_status = 0 #assume success until proven otherwise
        #delete and recreate links
        for src, dst in (
            ('generic_limited_net.xml',
             self.basedir + '/etc/svc/profile/generic.xml'),
            ('ns_dns.xml',
             self.basedir + '/etc/svc/profile/name_service.xml'),
            ('inetd_generic.xml',
             self.basedir + '/etc/svc/profile/inetd_services.xml'),
            (self.sc_profile,
             self.basedir + '/etc/svc/profile/site.xml')):

            try:
                os.unlink(dst)
            except OSError, (errno, strerror):
                if errno != 2: #file not found
                    prerror('Error deleting file ' + dst +
                        ' for smf profile. ' + strerror)
                    prerror('Failure. Returning: ' +
                            'ICT_SMF_CORRECT_SYS_PROFILE_FAILED')
                    return_status = ICT_SMF_CORRECT_SYS_PROFILE_FAILED
            except StandardError:
                prerror('Unrecognized error - could not delete file ' +
                    dst + ' for smf profile. ')
                prerror(traceback.format_exc())
                prerror('Failure. Returning: ' +
                        'ICT_SMF_CORRECT_SYS_PROFILE_FAILED')
                return_status = ICT_SMF_CORRECT_SYS_PROFILE_FAILED
            try:
                os.symlink(src, dst)
            except OSError, (errno, strerror):
                prerror('Error making symlinks for system profile. ' +
                        strerror)
                prerror('source=' + src + ' destination=' + dst)
                prerror('Failure. Returning: ' +
                        'ICT_SMF_CORRECT_SYS_PROFILE_FAILED')
                return_status = ICT_SMF_CORRECT_SYS_PROFILE_FAILED
            except StandardError:
                prerror('Unrecognized error making symlinks for ' +
                        'system profile.')
                prerror('source=' + src + ' destination=' + dst)
                prerror(traceback.format_exc())
                prerror('Failure. Returning: ' +
                        'ICT_SMF_CORRECT_SYS_PROFILE_FAILED')
                return_status = ICT_SMF_CORRECT_SYS_PROFILE_FAILED
        return return_status

    def configure_nwam(self):
        '''ICT - configure nwam by creating /etc/nwam/llp with
                the preferred interface followed by dhcp in it.
                The perferred interface should be the interface
                used for the network installation.

        return 0, otherwise error status
        '''
        _register_task(inspect.currentframe())

        cmd = "/usr/sbin/ifconfig -au | /usr/bin/grep '[0-9]:' " \
            "| /usr/bin/grep -v 'LOOPBACK'"

        (status, output) = commands.getstatusoutput(cmd)
        if status != 0:
            prerror('ifconfig command to determine preferred network ' +
                'interface failed. command=' + cmd)
            prerror('Failure. Returning: ICT_CONFIGURE_NWAM_FAILED')
            return ICT_CONFIGURE_NWAM_FAILED

        interface = output.split(':')[0]

        llp_dir = self.basedir + '/etc/nwam'
        llp_file = llp_dir + '/llp'

        if not os.access(llp_dir, os.F_OK):
            os.makedirs(llp_dir)
            # chown root:root
            os.chown(llp_dir, 0, 0)
            # chmod 755
            os.chmod(llp_dir,
                    S_IREAD | S_IWRITE | S_IEXEC |
                    S_IRGRP | S_IXGRP |
                    S_IROTH | S_IXOTH)
        try:
            config_file = open(llp_file, 'w')
            # add the line with <interface> dhcp to it.
            config_file.write(interface + "\tdhcp\n")
            config_file.close()
            # chown root:root
            os.chown(llp_file, 0, 0)
            # chmod 644
            os.chmod(llp_file, S_IREAD | S_IWRITE | S_IRGRP | S_IROTH)
        except IOError, errno:
            # Unable to open the file
            prerror('Unexpected error writing to <target>/etc/nwam/llp' +
                ' to configure nwam. ' +
                ' file=' + llp_file + ' failed to add the lines:\n' +
                interface + '\tdhcp' + ' due to ' + str(errno))
            prerror(traceback.format_exc()) #traceback to stdout and log
            prerror('Failure. Returning: ICT_CONFIGURE_NWAM_FAILED')
            return ICT_CONFIGURE_NWAM_FAILED

        return 0

    def enable_nwam(self):
        '''ICT - Enable nwam service
        SVCCFG_DTD=basedir + '/usr/share/lib/xml/dtd/service_bundle.dtd.1'
        SVCCFG_REPOSITORY=basedir + '/etc/svc/repository.db'
        svccfg apply basedir + '/etc/svc/profile/network_nwam.xml'

        return 0, otherwise error status
        '''
        _register_task(inspect.currentframe())

        return_status = 0

        nwam_profile = self.basedir + '/etc/svc/profile/network_nwam.xml'
        os.putenv('SVCCFG_DTD', self.basedir +
                  '/usr/share/lib/xml/dtd/service_bundle.dtd.1')
        os.putenv('SVCCFG_REPOSITORY', self.basedir + '/etc/svc/repository.db')
        cmd = '/usr/sbin/svccfg apply ' + nwam_profile + ' 2>&1'
        status, oa = _cmd_out(cmd)
        if status != 0:
            prerror('Command to enable nwam failed. exit status=' +
                    str(status))
            prerror('Command to enable nwam was: ' + cmd)
            for ln in oa:
                prerror(ln)

            prerror('Failure. Returning: ICT_ENABLE_NWAM_FAILED')
            return_status = ICT_ENABLE_NWAM_FAILED

        return return_status

    def do_not_configure_network(self):
        '''ICT - Do not configure any network.  NWAM will be disabled.
        Net physical default will be enabled.
        SVCCFG_DTD=basedir + '/usr/share/lib/xml/dtd/service_bundle.dtd.1'
        SVCCFG_REPOSITORY=basedir + '/etc/svc/repository.db'
        svccfg -s network/physical:default setprop general/enabled = true
        svccfg -s network/physical:nwam setprop general/enabled = false

        return 0, otherwise error status
        '''
        _register_task(inspect.currentframe())

        return_status = 0

        os.putenv('SVCCFG_DTD', self.basedir +
                  '/usr/share/lib/xml/dtd/service_bundle.dtd.1')
        os.putenv('SVCCFG_REPOSITORY', self.basedir + '/etc/svc/repository.db')
        cmd = '/usr/sbin/svccfg -s network/physical:default setprop ' + \
              'general/enabled = true 2>&1'
        status, oa = _cmd_out(cmd)
        if status != 0:
            prerror('Command to disable network/physical:default failed. ' + \
                    'exit status=' + str(status))
            prerror('Command to disable network/physical:default was: ' + cmd)
            for ln in oa:
                prerror(ln)
            prerror('Failure. Returning: ICT_SVCCFG_FAILURE')
            return(ICT_SVCCFG_FAILURE)

        cmd = '/usr/sbin/svccfg -s network/physical:nwam setprop ' + \
              'general/enabled = false 2>&1'
        status, oa = _cmd_out(cmd)
        if status != 0:
            prerror('Command to disable nwam failed. exit status=' + \
                    str(status))
            prerror('Command to disable nwam was: ' + cmd)
            for ln in oa:
                prerror(ln)

            prerror('Failure. Returning: ICT_SVCCFG_FAILURE')
            return (ICT_SVCCFG_FAILURE)

        return return_status

    def configure_network(self, ifname, ip, netmask, gw, dns, domain):
        '''ICT - Configure network manually.  NWAM will be disabled.
        Net physical default will be enabled.
        SVCCFG_DTD=basedir + '/usr/share/lib/xml/dtd/service_bundle.dtd.1'
        SVCCFG_REPOSITORY=basedir + '/etc/svc/repository.db'
        svccfg -s network/physical:default setprop general/enabled = true
        svccfg -s network/physical:nwam setprop general/enabled = false

        return 0, otherwise error status
        '''
        _register_task(inspect.currentframe())

        return_status = 0

        os.putenv('SVCCFG_DTD', self.basedir +
                  '/usr/share/lib/xml/dtd/service_bundle.dtd.1')
        os.putenv('SVCCFG_REPOSITORY', self.basedir + '/etc/svc/repository.db')
        cmd = '/usr/sbin/svccfg -s network/physical:default setprop ' + \
              'general/enabled = true 2>&1'
        status, oa = _cmd_out(cmd)
        if status != 0:
            prerror('Command to disable network/physical:default failed. ' + \
                    'exit status=' + str(status))
            prerror('Command to disable network/physical:default was: ' + cmd)
            for ln in oa:
                prerror(ln)
            prerror('Failure. Returning: ICT_SVCCFG_FAILURE')
            return(ICT_SVCCFG_FAILURE)

        cmd = '/usr/sbin/svccfg -s network/physical:nwam setprop ' + \
              'general/enabled = false 2>&1'
        status, oa = _cmd_out(cmd)
        if status != 0:
            prerror('Command to disable nwam failed. exit status=' + \
                    str(status))
            prerror('Command to disable nwam was: ' + cmd)
            for ln in oa:
                prerror(ln)

            prerror('Failure. Returning: ICT_SVCCFG_FAILURE')
            return (ICT_SVCCFG_FAILURE)

        sysding_cf="" + self.basedir + "/etc/sysding.conf"
	try:
            fp = open(sysding_cf,"w")
            if(ifname != ""):
                if(netmask !="" and ip !=""):
                    if (ip != "dhcp"):
                        fp.write("setup_interface %s v4 %s/%s\n" % (ifname,ip,get_net_size(netmask)))
                    else:
                        fp.write("setup_interface %s v4 dhcp\n" % (ifname))
                    if(gw != ""):
                        fp.write("setup_route default %s\n" % (gw))
            if(dns != ""):
                fp.write("setup_ns_dns \"%s\" \"%s\" \"%s\"\n" % (domain,"",dns))
            fp.close()
        except IOError:
            prerror('Failure. Returning: ICT_SVCCFG_FAILURE')
            return (ICT_SVCCFG_FAILURE)

        return return_status

    def remove_livecd_environment(self):
        '''ICT - Copy saved configuration files to remove vestiges of
        live CD environment. Also remove unneeded loader files.
        return 0 for success, error code otherwise
        '''
	loaderfile = self.basedir + '/boot/loader.rc.local'
	if os.path.exists(loaderfile):
	    os.unlink(loaderfile)
	loaderfile = self.basedir + '/boot/menu.rc.local'
	if os.path.exists(loaderfile):
	    os.unlink(loaderfile)
        savedir = self.basedir + '/save'
        if not os.path.exists(savedir):
            info_msg('saved configuration files directory is missing')
            return 0 # empty - assume no config files to back up
        cmd = '(cd %s && find . -type f -print | \
            /bin/cpio -pmu %s > /dev/null 2>& 1) && rm -rf %s' \
            % (savedir, self.basedir, savedir)
        status = _cmd_status(cmd)
        if status == 0:
            return 0
        prerror('remove liveCD environment failed: exit status ' + str(status))
        prerror('command was ' + cmd)
        prerror('Failure. Returning: ICT_REMOVE_LIVECD_ENVIRONMENT_FAILED')
        return ICT_REMOVE_LIVECD_ENVIRONMENT_FAILED

    def remove_specific_packages(self, pkg_list):
        '''ICT - Remove install-specific packages
        launch pkg -R basedir uninstall PACKAGE
        Parameter: pkg_list - list of pkg names
        return 0 for success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        return_status = 0
        for pkg in pkg_list:
            cmd = 'pkg -R %s uninstall -q --no-index %s 2>&1' % \
                (self.basedir, pkg)
            status, cmdout = _cmd_out(cmd)
            if status != 0:
                prerror('Removal of package %s failed.  pkg exit status =%d'
                        % (pkg, status))
                prerror('Failed package removal command=' + cmd)
                for ln in cmdout:
                    prerror(ln)
                prerror('Failure. Returning: ICT_PACKAGE_REMOVAL_FAILED')
                return_status = ICT_PACKAGE_REMOVAL_FAILED
        return return_status

    def set_flush_content_cache_false(self):
        '''ICT - The LiveCD can be configured to purge the IPS download cache.
        Restore the original IPS default to not purge the IPS download cache.
        Use the command line interface to IPS : pkg set-property
        return 0 upon success, error code otherwise
        '''
        _register_task(inspect.currentframe())

        cmd = 'pkg -R ' + self.basedir + \
            ' set-property flush-content-cache-on-success False'
        status = _cmd_status(cmd)
        if status != 0:
            prerror('Set property flush-content-cache-on-success ' +
                    'exit status = ' + str(status) + ', command was ' + cmd)
            prerror('Failure. Returning: ' +
                    'ICT_SET_FLUSH_CONTENT_CACHE_ON_SUCCESS_FAILED')
            return ICT_SET_FLUSH_CONTENT_CACHE_ON_SUCCESS_FAILED
        return 0

    def set_boot_device_property(self):
        '''ICT - update bootenv.rc 'console' property determines console
        boot device from bootenv.rc properties 'output-device' and 'console'
        updates 'console' bootenv.rc property
        return status = 0 if success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        # put it in bootenv.rc
        curosconsole = self._get_bootprop('output-device')
        osconsole = self._get_osconsole()
        if curosconsole != osconsole and osconsole != '':
            info_msg('Setting console boot device property to ' + osconsole)
            status = self._update_bootprop('console', osconsole)
            if status != 0:
                return status
        return 0

    def remove_livecd_coreadm_conf(self):
        '''ICT - Remove LiveCD-specific /etc/coreadm.conf config file.
        Coreadm will create its initial configuration on first boot
        see also coreadm(1m)
        returns 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        filename = self.basedir + '/etc/coreadm.conf'
        try:
            os.unlink(filename)
        except OSError, (errno, strerror):
            if errno != 2: #file does not exist
                prerror('I/O error - cannot delete file ' + filename +
                        ': ' + strerror)
                prerror('Failure. Returning: ' +
                        ' ICT_REMOVE_LIVECD_COREADM_CONF_FAILURE')
                return ICT_REMOVE_LIVECD_COREADM_CONF_FAILURE
            else:
                _dbg_msg('coreadm config file already removed ' + filename)
        except StandardError:
            prerror('Unrecognized error - cannot delete file ' + filename)
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ' +
                    'ICT_REMOVE_LIVECD_COREADM_CONF_FAILURE')
            return ICT_REMOVE_LIVECD_COREADM_CONF_FAILURE

        return 0

    def set_partition_active_sparc(self):
        '''support routine - set the Solaris partition active using eeprom
        return 0 if no errors for any drive, error code otherwise
        '''
        _register_task(inspect.currentframe())

        # Just in case this supporting routine was called directly.
        if not self.is_sparc:
            prerror('This supporting routine is not supported on this' +
                    ' hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM
        
        prom_device = '/dev/openprom'
        #ioctl codes and OPROMMAXPARAM taken from /usr/include/sys/openpromio.h
        oioc = ord('O') << 8
        opromdev2promname = oioc | 15   # Convert devfs path to prom path
        oprommaxparam = 32768

        # since the root device might be a metadevice, all the components
        # need to be located so each can be operated upon individually
        status = 0
        status, rdlist = self.get_rootdev_list('/dev/dsk/')
        if status != 0:
            prerror('get_rootdev_list() status=' + str(status))
            prerror('Failure. Returning: ICT_SET_PART_ACTIVE_FAILED')
            return ICT_SET_PART_ACTIVE_FAILED

        for rootdev in rdlist:
            _dbg_msg('root device: ' + rootdev)
            _dbg_msg('Opening prom device: ' + prom_device)
            try:
                prom = open(prom_device, "r")
            except StandardError:
                prom = None

            if prom == None:
                prerror('Failure to open prom device ' + prom_device)
                prerror('Failure. Returning: ICT_OPEN_PROM_DEVICE_FAILED')
                return ICT_OPEN_PROM_DEVICE_FAILED

            # Set up a mutable array for ioctl to read from and write to.
            # Standard Python objects are not usable here.  fcntl.ioctl
            # requires a mutable buffer pre-packed with the correct values
            # (as determined by the device-driver).  In this case,
            # openprom(7D) describes the following C stucture as defined in
            # <sys.openpromio.h>
            # struct openpromio {
            #     uint_t  oprom_size;       /* real size of following data */
            #     union {
            #         char  b[1];          /* NB: Adjacent, Null terminated */
            #         int   i;
            #     } opio_u;
            # };
            dev = (rootdev + "\0").ljust(oprommaxparam)
            buf = array.array('c', struct.pack('I%ds' % oprommaxparam,
                                               oprommaxparam, dev))

            # use ioctl to query the prom device.
            try:
                status = fcntl.ioctl(prom, opromdev2promname, buf, True)
            except StandardError:
                status = 1 # Force bad status for check below

            if status != 0:
                prom.close()
                prerror('ioctl OPROMDEV2PROMNAME ' + rootdev +
                        ' failed: status=' + str(status))
                prerror('Failure. Returning: ICT_IOCTL_PROM_FAILED')
                return ICT_IOCTL_PROM_FAILED

            prom.close()

            # Unpack the mutable array, buf, which ioctl just wrote into.
            new_oprom_size, new_dev = struct.unpack('I%ds' % oprommaxparam,
                                                    buf)

            # Device names are a list of null-terminated tokens, with a
            # double null on the final token.  We use only the first token.
            prom_name = new_dev.split('\0')[0]
            _dbg_msg('prom name:: ' + prom_name)

            # Set the boot device using eeprom
            status = _cmd_status('/usr/sbin/eeprom boot-device=' + prom_name)
            if status != 0:
                prerror('fcntl ioctl OPROMDEV2PROMNAME failed: status=' +
                        str(status))
                prerror('Failure. Returning: ICT_IOCTL_PROM_FAILED')
                return ICT_IOCTL_PROM_FAILED

        #if no errors encountered. Return 0 for success
        return 0

    def set_partition_active_x86(self):

        '''support routine - set the Solaris partition on the just
        installed drive to active rewrites disk format tables
        see set_boot_active()
        return 0 if no errors for any drive, error code otherwise
        '''
        _register_task(inspect.currentframe())

        # Just in case this supporting routine was called directly.
        if self.is_sparc:
            prerror('This supporting routine is not supported on this ' +
                    'hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        # since the root device might be a metadevice, all the components
        # need to be located so each can be operated upon individually
        return_status, rdlist = self.get_rootdev_list('/dev/rdsk/')
        if return_status == 0:
            for rootdev in rdlist:
                _dbg_msg('root device: ' + rootdev)
                status = self.set_boot_active(rootdev)
                if status != 0:
                    return_status = status
                status = self.bootadm_update_menu(rootdev)
                if status != 0:
                    return_status = status
        #if any operation fails, return error status
        return return_status

    def set_partition_active(self):
        '''ICT - set the Solaris partition active
        This ICT is implemented differently on SPARC and x86.
        Invoke the correct supporting routine based on platform.
        Bubble up status from supporting routine.
        return 0 if no errors for any drive, error code otherwise
        '''
        _register_task(inspect.currentframe())

        if self.is_sparc:
            return_status = self.set_partition_active_sparc()
        else:
            return_status = self.set_partition_active_x86()

        return return_status

    def get_special_grub_entry(self):
        '''Support function for the fix_grub_entry() function.
        Determines whether a special string is needed for the grub
        entry.  The special string, if specified, should be
        in the .image_info file.

        - return the special grub entry if one is found in .image_info.
        - return None if none is found
        '''

        if (not self.livecd_install and not self.text_install):
            # Not going to have .image_info file
            return None

        #
        # Check whether a specific title should be used for the
        # grub menu instead of the default one.  If a specific
        # title should be used, the Distribution Constructor
        # will put the special title in the /.cdrom/.image_info file
        #
        grub_title = None
        img_info_fd = None
        if (self.livecd_install or self.text_install):
            img_info_file = "/.cdrom/.image_info"
        else:
            img_info_file = "/tmp/.image_info"

        try:
            try:
                img_info_fd = open(img_info_file, "r")
                for line in img_info_fd:
                    if line.startswith("GRUB_TITLE="):
                        grub_title_line = line.rstrip('\n')
                        title_string = grub_title_line.split("=")
                        grub_title = title_string[1]
                        break
            except StandardError:
                # Should not get into this situation, but
                # it is harmless to continue, so, just
                # log it.
                _dbg_msg("No image file found. Use default grub title")
        finally:
            if (img_info_fd != None):
                img_info_fd.close()
        
        return grub_title
    
    def match_boot_entry(self, title):
        '''Returns True if the given line is the 'title' line that needs
        to be updated with the "special grub entry."
        
        For now, assume that boot_entry is the right entry, as there
        should be only one entry in the menu.lst file at this point
        in the install
        
        '''
        return title.startswith("title")
    
    def fix_grub_entry(self):
        '''ICT - Fix up the grub entry. If a special grub title entry
        is defined when the image is built by the Distribution
        Constructor, that special title will be used.
        
        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        #This ICT is not supported on SPARC platforms.
        #If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM
        
        new_title = self.get_special_grub_entry()
        if new_title is None:
            # No need to update the grub entry
            return 0
        
        newgrubmenu = self.grubmenu + '.new'
        try:
            with open(self.grubmenu, "r") as old_grub_fd:
                old_lines = old_grub_fd.readlines()
            
            with open(newgrubmenu, "w") as new_grub_fd:
                for line in old_lines:
                    if self.match_boot_entry(line):
                        # replace part of existing title
                        # with the specified new title
                        new_grub_fd.write("title %s\n" % new_title)
                    else:
                        new_grub_fd.write(line)
        except (OSError, IOError) as err:
            prerror('Error updating grub menu: ' + str(err))
            prerror('Failure. Returning: ICT_FIX_GRUB_ENTRY_FAILED')
            return ICT_FIX_GRUB_ENTRY_FAILED
        
        if not _move_in_updated_config_file(newgrubmenu,
                                            self.grubmenu):
            prerror('Failure. Returning: ICT_FIX_GRUB_ENTRY_FAILED')
            return ICT_FIX_GRUB_ENTRY_FAILED

        return 0

    def create_sparc_boot_menu(self, title=None):
        '''ICT - Create a boot menu.lst file on a SPARC system.
        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        #This ICT is only supported on SPARC platforms.
        #If invoked on a non SPARC platform return ICT_INVALID_PLATFORM
        if not self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        # Attempt to create the path to where the menu.lst file will reside
        # Catch OSError and pass if the directory already exists.
        try:
            os.makedirs(self.bootmenu_path_sparc)
        except OSError:
            pass

        # Write to the menu.lst file
        rootdataset = self._get_root_dataset()
        if rootdataset == '':
            prerror('Could not determine root dataset')
            prerror('Failure. Returning: ICT_CREATE_SPARC_BOOT_MENU_FAILED')
            return ICT_CREATE_SPARC_BOOT_MENU_FAILED
        
        if title is None:
            title = MENU_LST_DEFAULT_TITLE
        sparc_title_line = 'title %s\n' % title
        bootfs_line = 'bootfs ' + rootdataset + '\n'

        try:
            op = open(self.bootmenu_sparc, 'w')
            op.write(sparc_title_line)
            op.write(bootfs_line)
            op.close()
            os.chmod(self.bootmenu_sparc,
                     S_IREAD | S_IWRITE | S_IRGRP | S_IROTH)
            os.chown(self.bootmenu_sparc, 0, 3)  # chown root:sys
        except OSError, (errno, strerror):
            prerror('Error when creating sparc boot menu.lst file ' +
                self.bootmenu_sparc + ': ' + strerror)
            prerror('Failure. Returning: ICT_CREATE_SPARC_BOOT_MENU_FAILED')
            return ICT_CREATE_SPARC_BOOT_MENU_FAILED
        except StandardError:
            prerror('Unexpected error when creating sparc boot ' +
                    ' menu.lst file ' + self.bootmenu_sparc)
            prerror(traceback.format_exc())  # traceback to stdout and log
            prerror('Failure. Returning: ICT_CREATE_SPARC_BOOT_MENU_FAILED')
            return ICT_CREATE_SPARC_BOOT_MENU_FAILED

        return 0

    def copy_sparc_bootlst(self):
        '''ICT - Copy the bootlst file on a SPARC system.
        On SPARC systems a bootlst file is maintained at:
        /platform/`uname -m`/bootlst

        It needs to be copied to:
        <rootpool>/platform/`uname -m`/bootlst

        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())

        # This ICT is only supported on SPARC platforms.
        # If invoked on a non SPARC platform return ICT_INVALID_PLATFORM
        if not self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        # Copy file bootlst from basedir to the rootpool
        bootlst_dir = '/' + self.rootpool + '/platform/' + platform.machine()
        bootlst_dst = bootlst_dir + '/bootlst'
        bootlst_src = self.basedir + '/platform/' + platform.machine() + \
            '/bootlst'

        # Create the destination directory if it does not already exist.
        # Catch OSError and pass if the directory already exists.
        try:
            os.makedirs(bootlst_dir)
        except OSError:
            pass

        # Copy the bootlst
        try:
            shutil.copyfile(bootlst_src, bootlst_dst)
            os.chmod(bootlst_dst, S_IREAD | S_IWRITE | S_IRGRP | S_IROTH)
            os.chown(bootlst_dst, 0, 3)  # chown root:sys

        except OSError, (errno, strerror):
            prerror('Error when copying the sparc bootlst file ' +
                    bootlst_src + ': ' + strerror)
            prerror('Failure. Returning: ICT_COPY_SPARC_BOOTLST_FAILED')
            return ICT_COPY_SPARC_BOOTLST_FAILED
        except StandardError:
            prerror('Unexpected error when copying the sparc bootlst file ' +
                    self.bootmenu_sparc)
            prerror(traceback.format_exc())  # traceback to stdout and log
            prerror('Failure. Returning: ICT_COPY_SPARC_BOOTLST_FAILED')
            return ICT_COPY_SPARC_BOOTLST_FAILED

        return 0

    def add_operating_system_grub_entry(self):
        '''ICT - add entries for other installed OS's to the grub menu
        Launch /sbin/mkmenu <target GRUB menu>
        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        # This ICT is not supported on SPARC platforms.
        # If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM

        cmd = '/sbin/mkmenu ' + self.grubmenu
        cwd_start = os.getcwd()
        os.chdir('/')
        status = _cmd_status(cmd)
        os.chdir(cwd_start)
        if status != 0:
            prerror('Add other OS to grub menu failed. command=' + cmd +
                ' exit status=' + str(status))
            prerror('Failure. Returning: ICT_MKMENU_FAILED')
            return ICT_MKMENU_FAILED
        return 0

    def do_clobber_files(self, flist_file):
        '''ICT - Given a file containing a list of pathnames,
        search for those entries in the alternate root and
        delete all matching pathnames from the alternate root that
        are symbolic links.
        This process is required because of the way the LiveCD env
        is constructed. Some of the entries in the boot_archive are
        symbolic links to files mounted off a compressed lofi file.
        This is done to drastically reduce space usage by the boot_archive.
        side effect: current directory changed to basedir
        returns 0 if all processing completed successfully,
        error code if any problems
        '''
        _register_task(inspect.currentframe())
        _dbg_msg('File with list of pathnames with symbolic links' +
                 ' to clobber: ' + flist_file)
        try:
            fh = open(flist_file, 'r')
            os.chdir(self.basedir)
        except OSError, (errno, strerror):
            prerror('I/O error - cannot access clobber list file ' +
                    flist_file + ': ' + strerror)
            prerror('Failure. Returning: ICT_CLOBBER_FILE_FAILED')
            return ICT_CLOBBER_FILE_FAILED
        except StandardError:
            prerror('Unrecognized error processing clobber list file ' +
                    flist_file)
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_CLOBBER_FILE_FAILED')
            return ICT_CLOBBER_FILE_FAILED
        return_status = 0
        for line in fh:
            line = line[:-1]
            try:
                mst = os.lstat(line)
                if S_ISLNK(mst.st_mode):
                    _dbg_msg("Unlink: " + line)
                    os.unlink(line)
            except OSError, (errno, strerror):
                if errno == 2:  # file does not exist
                    _dbg_msg('Pathname ' + line +
                             ' not found - nothing deleted')
                else:
                    prerror('I/O error - cannot delete soft link ' +
                            line + ': ' + strerror)
                    prerror('Failure. Returning: ICT_CLOBBER_FILE_FAILED')

                    # one or more items fail processing
                    return_status = ICT_CLOBBER_FILE_FAILED
            except StandardError:
                prerror('Unrecognized error during file ' + line + ' clobber')
                prerror(traceback.format_exc())
        fh.close()
        return return_status

    def copy_capability_file(self):
        '''ICT - copies grub capability file from microroot to grub
        directory in root pool.

        Parameters:
            none
        returns 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        # This ICT is not supported on SPARC platforms.
        # If invoked on a SPARC platform return ICT_INVALID_PLATFORM
        if self.is_sparc:
            prerror('This ICT is not supported on this hardware platform.')
            prerror('Failure. Returning: ICT_INVALID_PLATFORM')
            return ICT_INVALID_PLATFORM
        try:
            shutil.copy2("/boot/grub/capability", "/" + self.rootpool +
                         "/boot/grub/capability")
        except (OSError, IOError) as err:
            prerror('Error copying /boot/grub/capability to ' + '/' +
                    self.rootpool + '/boot/grub/capability:' + str(err))
            return ICT_COPY_CAPABILITY_FAILED
        return 0

    def cleanup_unneeded_files_and_dirs(self,
                                        more_cleanup_files=None,
                                        more_cleanup_dirs=None):
        '''ICT - removes list of files and directories that should
        not have been copied If these are of appreciable size, they
        should be withheld from cpio file list instead

        Parameters:
                more_cleanup_files - list of additional files to delete
                more_cleanup_dirs - list of additional directories to delete
        returns 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        # Cleanup the files and directories that were copied into
        # the basedir directory that are not needed by the installed OS.
        file_cleanup_list = ["/.livecd",
                             "/.volsetid",
                             "/.textinstall",
                             "/etc/sysconfig/language",
                             "/.liveusb"]
        dir_cleanup_list = ["/a", "/bootcd_microroot"]
        if more_cleanup_files:
            file_cleanup_list.extend(more_cleanup_files)
        if more_cleanup_dirs:
            dir_cleanup_list.extend(more_cleanup_dirs)
        return_status = 0
        for basefname in file_cleanup_list:
            fname = self.basedir + "/" + basefname
            _dbg_msg('Removing file ' + fname)
            try:
                os.remove(fname)
            except OSError, (errno, strerror):
                if errno == 2:  # file not found
                    _dbg_msg('File to delete was not found: ' + fname)
                else:
                    prerror('Error deleting file ' + fname + ': ' + strerror)
                    prerror('Failure. Returning: ICT_CLEANUP_FAILED')
                    return_status = ICT_CLEANUP_FAILED
            except StandardError:
                prerror('Unexpected error deleting directory.')
                prerror(traceback.format_exc())

        # Since pkg:/system/boot/grub delivers the reference grub menu file
        # (/boot/grub/menu.lst) we'll have to copy the menu.lst
        # file from the microroot into the installed system.
        # Since this file is for reference only if the copy
        # fails we don't want to stop the install for this but
        # we should log it.
        if not self.is_sparc:
            try:
                shutil.copy2("/boot/grub/menu.lst", self.basedir +
                             "/boot/grub/menu.lst")
            except (OSError, IOError) as err:
                prerror('Error copying /boot/grub/menu.lst to ' +
                        self.basedir + '/boot/grub/menu.lst :' + str(err))

        # The bootcd_microroot directory should be cleaned up in the
        # Distribution Constructor once they have finished the redesign.
        for basedname in dir_cleanup_list:
            dname = self.basedir + "/" + basedname
            _dbg_msg('removing directory' + dname)
            try:
                os.rmdir(dname)
            except OSError, (errno, strerror):
                if errno == 2:  # file not found
                    _dbg_msg('Path to delete was not found: ' + dname)
                else:
                    prerror('Error deleting directory ' + dname +
                            ': ' + strerror)
                    prerror('Failure. Returning: ICT_CLEANUP_FAILED')
                    return_status = ICT_CLEANUP_FAILED
            except StandardError:
                prerror('Unexpected error deleting file.')
                prerror(traceback.format_exc())
        return return_status

    def reset_image_uuid(self):
        '''ICT - reset pkg(1) image UUID for preferred publisher

        Obtain name of all publishers by parsing output of the following
        command: pkg -R basedir property -H publisher-search-order

        launch pkg -R basedir set-publisher --reset-uuid --no-refresh \
            <preferred_publisher>

        launch pkg -R basedir pkg set-property send-uuid True

        return 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        cmd = '/usr/bin/pkg -R ' + self.basedir + ' property -H ' + \
            'publisher-search-order'
        status, co = _cmd_out(cmd)
        if status != 0:
            prerror('pkg(1) failed to obtain list of publishers - '
                    'exit status = ' + str(status) + ', command was ' + cmd)
            prerror('Failure. Returning: ICT_PKG_RESET_UUID_FAILED')
            return ICT_PKG_RESET_UUID_FAILED

        publisher_search_order = co[0].strip().split(None, 1)
        # strip off the leading '[' and trailing ']' characters
        for publisher in publisher_search_order[1:-1]:
            # strip off any ' or , characters
            publisher = publisher.strip("',")

            cmd = 'pkg -R ' + self.basedir + \
                ' set-publisher --reset-uuid --no-refresh ' + publisher
            status = _cmd_status(cmd)
            if status != 0:
                prerror('Reset uuid failed for publisher:  ' + publisher +
                        '- exit status = ' + str(status) +
                        ', command was ' + cmd)
                prerror('Failure. Returning: ICT_PKG_RESET_UUID_FAILED')
                return ICT_PKG_RESET_UUID_FAILED

        cmd = 'pkg -R ' + self.basedir + ' set-property send-uuid True'
        status = _cmd_status(cmd)
        if status != 0:
            prerror('Set property send uuid - exit status = ' + str(status) +
                ', command was ' + cmd)
            prerror('Failure. Returning: ICT_PKG_SEND_UUID_FAILED')
            return ICT_PKG_SEND_UUID_FAILED

        return 0

    def rebuild_pkg_index(self):
        '''ICT - rebuild pkg(1) index
        launch pkg -R basedir rebuild-index
        return 0 for success, otherwise error code
        '''
        _register_task(inspect.currentframe())
        cmd = 'pkg -R ' + self.basedir + ' rebuild-index'
        status = _cmd_status(cmd)
        if status == 0:
            return 0
        prerror('Rebuild package index failed - exit status = ' +
                str(status) + ', command was ' + cmd)
        prerror('Failure. Returning: ICT_REBUILD_PKG_INDEX_FAILED')
        return ICT_REBUILD_PKG_INDEX_FAILED

    def create_new_user(self, gcos, login, pw, gid, uid):
        '''ICT - create the new user.
        using IPS class PasswordFile from pkg.cfgfiles
        It is possible no new user was requested. If none was
        specified do nothing and return 0.
        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())

        return_status = 0
        _dbg_msg('creating new user on target: ' + self.basedir)

        if not login:
            _dbg_msg('No login specified')
            return return_status

        try:
            pf = PasswordFile(self.basedir)
            nu = pf.getuser('nobody')
            nu['username'] = login
            nu['gid'] = gid
            nu['uid'] = uid
            nu['gcos-field'] = gcos
            nu['home-dir'] = '/export/home/' + login
            nu['login-shell'] = '/bin/bash'
            nu['password'] = pw
            pf.setvalue(nu)
            pf.writefile()

        except StandardError:
            prerror('Failure to modify the root password')
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_CREATE_NU_FAILED')
            return_status = ICT_CREATE_NU_FAILED

        return return_status

    def copy_generated_files(self, login, uid, gid):
        '''ICT - copy generated files from /jack to user home
        '''
        _register_task(inspect.currentframe())
        
        return_status = 0
        _dbg_msg('copying generated user files to home directory under: ' + self.basedir)
        
        if not login:
            _dbg_msg('No login specified')
            return return_status
        
        dest_dir = self.basedir + '/export/home/' + login
        for fl in self.home_generated_files:
            dstfile = dest_dir + '/' + fl
            srcfile = '/jack/' + fl
            if os.path.exists(srcfile):
                try:
                    if os.path.exists(dstfile):
                        os.unlink(dstfile)
                    shutil.copy2(srcfile, dstfile)
                    os.chown(dstfile, int(uid), int(gid))
                except (OSError, IOError) as err:
                    prerror('Error copying ' +
                            srcfile + ' to ' + dstfile +
                            ' :' + str(err))
                    return_status = ICT_COPY_GENERATED_FILES_FAILED

        return return_status
        
    def set_root_password(self, newpw, expire=False):
        '''ICT - set the root password on the specified install target.
        using IPS class PasswordFile from pkg.cfgfiles.  Pre-expire password
        if expire is True
        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())

        return_status = 0
        _dbg_msg('setting root password on target: ' + self.basedir)

        try:
            pf = PasswordFile(self.basedir)
            ru = pf.getuser('root')
            ru['password'] = newpw
            if expire:
                ru['lastchg'] = 0
            pf.setvalue(ru)
            pf.writefile()
        except StandardError:
            prerror('Failure to modify the root password')
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_SET_ROOT_PW_FAILED')
            return_status = ICT_SET_ROOT_PW_FAILED

        return return_status

    def apply_sysconfig_profile(self):
        '''ICT - apply system configuration SMF profile to the target.
        The SMF profile will be applied during first boot as part of
        Early Manifest Import process.

        Carry out only syntactic validation of SMF profile.

        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())

        sc_profile_src = self.ai_sc_profile
        sc_profile_dst = self.basedir + '/etc/svc/profile/' + \
                         self.sc_profile
        os.environ["SVCCFG_DTD"] = self.basedir + \
                   '/usr/share/lib/xml/dtd/service_bundle.dtd.1'
        os.environ["SVCCFG_REPOSITORY"] = self.basedir + \
                   '/etc/svc/repository.db'
        cmd = '/usr/sbin/svccfg apply -n ' + sc_profile_src + ' 2>&1'
        status, oa = _cmd_out(cmd)

        if status == 0:
            write_log(ICTID, 'Syntactic validation of System configuration '
                      'profile succeeded\n')
        else:
            prerror('Syntactic validation of System configuration profile '
                    'failed. exit status=' + str(status))
            prerror('Command to validate configuration profile was: ' + cmd)
            for ln in oa:
                prerror(ln)

            prerror('Failure. Returning: ICT_APPLY_SYSCONFIG_FAILED')
            return ICT_APPLY_SYSCONFIG_FAILED

        # copy SMF profile to the target and make sure it can be read only
        # by root in order to protect encrypted password for configured
        # root and user accounts
        try:
            shutil.copyfile(sc_profile_src, sc_profile_dst)
            os.chmod(sc_profile_dst, S_IRUSR)  # read-only by user (root)
            os.chown(sc_profile_dst, 0, 3)  # chown root:sys

        except OSError, (errno, strerror):
            prerror('Error when copying System Configuration profile ' +
                    sc_profile_src + ' to ' + sc_profile_dst + ' : ' +
                    strerror)
            prerror('Failure. Returning: ICT_APPLY_SYSCONFIG_FAILED')
            return ICT_APPLY_SYSCONFIG_FAILED
        except StandardError:
            prerror('Unexpected error when copying System Configuration ' +
                    ' profile ' + sc_profile_src + ' to ' + sc_profile_dst)
            prerror(traceback.format_exc())  # traceback to stdout and log
            prerror('Failure. Returning: ICT_APPLY_SYSCONFIG_FAILED')
            return ICT_APPLY_SYSCONFIG_FAILED
        
        return 0

    def setup_rbac(self, login):
        '''ICT - configure user for root role, without any extra profiles and
        remove the jack user from user_attr
        return 0 on success, error code otherwise
        '''
        _register_task(inspect.currentframe())
        return_status = 0
        _dbg_msg('configuring RBAC in: ' + self.basedir)

        try:
            f = UserattrFile(self.basedir)
            # Remove jack if present
            if f.getvalue({'username' : 'jack'}):
                f.removevalue({'username' : 'jack'})
            
            rootentry = f.getvalue({'username' : 'root'})
            rootattrs = rootentry['attributes']
            
            # If we're creating a user, then ensure root is a role and
            # add the user.  Otherwise ensure that root is not a role.
            if login:
                rootattrs['type'] = ['role']
                rootentry['attributes'] = rootattrs
                f.setvalue(rootentry)
                
                # Attributes of a userattr entry are a dictionary of list values
                userattrs = dict({'roles' : ['root']})
                # An entry is a dictionary with username and attributes
                userentry = dict({'username' : login, 'attributes' : userattrs})
                f.setvalue(userentry)
            else:
                if 'type' in rootattrs:
                    del rootattrs['type']
                    rootentry['attributes'] = rootattrs
                    f.setvalue(rootentry)
            
            # Write the resulting file
            f.writefile()

        except StandardError:
            prerror('Failure to edit user_attr file')
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_SETUP_RBAC_FAILED')
            return_status = ICT_SETUP_RBAC_FAILED
        
        return return_status

    def setup_sudo(self, login):
        '''ICT - configure user for sudo access, removing jack user from sudoers
        return 0 on success, error code otherwise
        '''
        
        _register_task(inspect.currentframe())
        return_status = 0
        _dbg_msg('configuring RBAC in: ' + self.basedir)

        temp_file = '/var/run/sudoers'

        try:
            if os.path.isfile(self.sudoers):
                with open(self.sudoers, 'r') as fp:
                    sudoers_lines = fp.readlines()
                with open(temp_file, 'w') as fp_tmp:
                    for l in sudoers_lines:
                        if not l.startswith("jack"):
                            fp_tmp.write(l)
                    if login:
                        fp_tmp.write(login + ' ALL=(ALL) ALL\n')
    
                os.remove(self.sudoers)
                shutil.move(temp_file, self.sudoers)
                os.chmod(self.sudoers, S_IREAD | S_IRGRP)
                os.chown(self.sudoers, 0, 0) # chown root:root
        except IOError, (errno, strerror):
            prerror('Failure to edit sudoers file')
            prerror(traceback.format_exc())
            prerror('Failure. Returning: ICT_SETUP_SUDO_FAILED')
            return_status = ICT_SETUP_SUDO_FAILED

        return return_status

    def ict_test(self, optparm=None):
        '''ICT - ict test
        This ict can be used to test the ICT object from the command line.
        It always returns 0
        '''
        _register_task(inspect.currentframe())

        info_msg('ict_test invoked')

        info_msg('optparm: ' + str(optparm))
        info_msg('basedir: ' + str(self.basedir))
        info_msg('bootenvrc: ' + str(self.bootenvrc))
        info_msg('grubmenu: ' + str(self.grubmenu))
        info_msg('is_sparc: ' + str(self.is_sparc))
        info_msg('livecd_install: ' + str(self.livecd_install))
        info_msg('text_install: ' + str(self.text_install))
        info_msg('kbd_device: ' + str(self.kbd_device))
        info_msg('kbd_layout_file: ' + str(self.kbd_layout_file))
        info_msg('rootpool: ' + str(self.rootpool))
        
        return 0

    #end Install Completion Tasks


def exec_ict(ict_name, basedir, debuglvl=None, optparm=None):
    '''run one ICT with a single command line using 'eval()'
    This will be called automatically if 2 or more command line arguments
    are provided
    ict_name - name of ICT in text string
    basedir - root directory of target
    debuglvl - logging service debugging level to override default
    optparm - parameter passed to ict_name if required by ict_name
    returns status of ict_name
    '''
    info_msg('Executing ICT=' + str(ict_name) + ' basedir=' + str(basedir))
    info_msg('    debuglvl=' + str(debuglvl) + ' optparm=' + str(optparm))

    if debuglvl != None:
        info_msg('setting debug level to' + debuglvl)
        myict = ICT(basedir, debuglvl)
    else:
        myict = ICT(basedir)
    if optparm != None:
        status = eval('myict.' + ict_name + '(optparm)')
    else:
        status = eval('myict.' + ict_name + '()')
    sys.exit(status)

if __name__ == '__main__':
    ''' The script is launched in order to run an individual ICT.

    Example command line:  # python ict.py ict_test /a  #runs test ICT
    '''
    # Invoke exec_ict() with the command line parameters, ignoring
    # the first argument, which is always the script name (ict.py).
    exec_ict(*sys.argv[1:])
