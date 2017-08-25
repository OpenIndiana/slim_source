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
# Copyright (c) 2009, 2010, Oracle and/or its affiliates. All rights reserved.
#

'''
Contains the Welcome Screen for the Text Installer
'''
import curses

from osol_install.text_install import _, RELEASE
from osol_install.text_install.action import Action
from osol_install.text_install.base_screen import BaseScreen
from osol_install.profile.install_profile import InstallProfile


class WelcomeScreen(BaseScreen):
    '''First screen of the text installer.
    No special __init__ needed beyond that provided by BaseScreen
    
    '''
    
    HEADER_TEXT = _("Welcome to %(release)s") % RELEASE
    WELCOME_TEXT = _("Thanks for choosing to install %(release)s! This "
                     "installer enables you to install the %(release)s "
                     "Operating System (OS) on SPARC or x86 systems.\n\n"
                     "The installation log will be at %(log)s.\n\n"
                     "How to navigate through this installer:")
    BULLET_ITEMS = [_("Use the function keys listed at the bottom of each "
                 "screen to move from screen to screen and to perform "
                 "other operations."),
               _("Use the up/down arrow keys to change the selection "
                 "or to move between input fields."),
               _("If your keyboard does not have function keys, or "
                 "they do not respond, press ESC; the legend at the "
                 "bottom of the screen will change to show the ESC keys"
                 " for navigation and other functions.")]
    BULLET = "- "
    
    def set_actions(self):
        '''Remove the F3_Back Action from the first screen'''
        self.main_win.actions.pop(self.main_win.back_action.key, None)
        zpool_install_action = Action(curses.KEY_F5, _("InstallToExistingPool"),
                                      self.zpool_install)
        self.main_win.actions[zpool_install_action.key] = zpool_install_action

    def zpool_install(self, dummy):
        '''We write down install_profile.install_to_pool flag
        and show the next screen.
        '''
        self.install_profile.install_to_pool = True
        return self.main_win.screen_list.get_next(self)
    
    def _show(self):
        '''Display the static paragraph WELCOME_TEXT'''
        log_file = self.install_profile.log_location
        y_loc = 1
        fmt = {"log" : log_file}
        fmt.update(RELEASE)
        text = WelcomeScreen.WELCOME_TEXT % fmt
        y_loc += self.center_win.add_paragraph(text, start_y=y_loc)
        x_loc = len(WelcomeScreen.BULLET)
        for bullet in WelcomeScreen.BULLET_ITEMS:
            self.center_win.add_text(WelcomeScreen.BULLET, start_y=y_loc)
            y_loc += self.center_win.add_paragraph(bullet, start_y=y_loc,
                                                   start_x=x_loc)
        # If user returned back to this screen, forget about hitting F5
        if self.install_profile.install_to_pool:
            self.install_profile.install_to_pool = False
            self.install_profile.pool_name = None
            self.install_profile.be_name = InstallProfile.DEFAULT_BE_NAME
