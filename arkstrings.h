/*
    ark: A program for modifying archives via a GUI.
    (c)1997 Robert Palmbos

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

File added 1999

Modifications to this program were made by Corel Corporation, November, 1999.
All such modifications are copyright (C) 1999 Corel Corporation and are 
licensed under the terms of the GNU General Public License

*/

#ifndef __ARKSTRINGS_H__
#define __ARKSTRINGS_H__

// Strings to be used in arkwidget.cpp

#include <kapp.h>

#define IDS_NEW_ARCHIVE i18n("&New Archive...")
#define IDS_OPEN_ARCHIVE i18n("&Open Archive...")
#define IDS_CLOSE_ARCHIVE i18n("&Close Archive")
#define IDS_NEW_WINDOW i18n("New &Window")
#define IDS_EXIT_APP i18n("&Exit")
#define IDS_ADD_FILE i18n("&Add File...")
#define IDS_ADD_DIR i18n("Add Directory...")
#define IDS_DELETE_FILE i18n("&Delete")
#define IDS_RENAME_FILE i18n("&Rename")
#define IDS_EXTRACT_FILE i18n("&Extract...")
#define IDS_VIEW_FILE i18n("&View")
#define IDS_SELECT_ALL i18n("&Select All")
#define IDS_HELP_TOPICS i18n("Help Topics")
#define IDS_ABOUT_ARCHIVER i18n("About Archive Administrator")
#define IDS_MENU_FILE i18n("&File")
#define IDS_MENU_EDIT i18n("&Edit")
#define IDS_MENU_OPTIONS i18n("&Options")
#define IDS_OPTIONS i18n("&Options...")
#define IDS_SAVE_OPTIONS i18n("&Save Now")
#define IDS_MENU_HELP i18n("&Help")
#define IDS_POPUP_EXTRACT i18n("Extract to...")
#define IDS_POPUP_VIEW i18n("Open/Run")
#define IDS_POPUP_RENAME i18n("Rename")
#define IDS_POPUP_DELETE i18n("Delete")
#define IDS_POPUP_ADD i18n("Add File...")
#define IDS_POPUP_SELECT_ALL i18n("Select All")
#define IDS_POPUP_NEW i18n("New Archive...")
#define IDS_POPUP_OPEN i18n("Open Archive...")
#define IDS_POPUP_CLOSE i18n("Close Archive")
#define IDS_POPUP_HELP i18n("Help")
#define IDS_POPUP_OPTIONS i18n("Options...")
#define IDS_TOOLBAR_NEW i18n("New")
#define IDS_TOOLBAR_NEW_TIP i18n("Create a new archive")
#define IDS_TOOLBAR_OPEN i18n("Open")
#define IDS_TOOLBAR_OPEN_TIP i18n("Open an archive")
#define IDS_TOOLBAR_ADD_FILE i18n("Add File")
#define IDS_TOOLBAR_ADD_FILE_TIP i18n("Add file(s) to the current archive")
#define IDS_TOOLBAR_ADD_DIR i18n("Add Dir")
#define IDS_TOOLBAR_ADD_DIR_TIP i18n("Add a directory to the current archive")
#define IDS_TOOLBAR_DEL i18n("Delete")
#define IDS_TOOLBAR_DEL_TIP i18n("Delete file(s) from the current archive")
#define IDS_TOOLBAR_EXTRACT i18n("Extract")
#define IDS_TOOLBAR_EXTRACT_TIP i18n("Extract file(s) from the current archive")
#define IDS_TOOLBAR_SELECT_ALL i18n("Select All")
#define IDS_TOOLBAR_SELECT_ALL_TIP i18n("Mark all files selected")
#define IDS_TOOLBAR_VIEW i18n("View")
#define IDS_TOOLBAR_VIEW_TIP i18n("View or run a file")
#define IDS_TOOLBAR_OPTIONS i18n("Options")
#define IDS_TOOLBAR_OPTIONS_TIP i18n("Change your settings")
#define IDS_TOOLBAR_HELP i18n("Help")
#define IDS_TOOLBAR_HELP_TIP i18n("Help")
#define IDS_HEADER_NAME i18n("Name            ")
#define IDS_HEADER_PERMS i18n("Permissions     ")
#define IDS_HEADER_OWNERGRP i18n("Owner/Group     ")
#define IDS_HEADER_SIZE i18n("Size            ")
#define IDS_HEADER_DATE i18n("Date            ")
#define IDS_ERROR i18n("Error")
#define IDS_BAD_ARCH i18n("\nSorry - Archive Administrator cannot create an archive of that type.\n\n  [Hint:  The filename should have an extension such as `.tgz' to\n  indicate the type of the archive. Please see the help pages for\n  more information on supported archive formats.]")
#define IDS_NOT_NORMAL_EXIT i18n("Your archive has been corrupted somehow")
#define IDS_UNKNOWN_ARCH i18n("Unknown archive format")
#define IDS_DOESNT_EXIST i18n("Archive does not exist")
#define IDS_NO_ACCESS i18n("Can't access archive")
#define IDS_NO_PERMISSION  i18n("You don't have permission to access that archive")
#define IDS_NO_LISTING i18n("Couldn't get listing.\n")
#define IDS_NOT_SUPPORTED i18n("Adding a directory is not supported by this archiving utility.\n")
#define IDS_EXTRACTING i18n("Extracting...")
#define IDS_OPENING i18n("Opening archive...")
#define IDS_ADDING i18n("Adding to archive...")
#define IDS_SHOWING i18n("Displaying...")
#define IDS_INFO i18n("Information")
#define IDS_EXTRACT_DONE i18n("Extraction completed")
#define IDS_ARCH_ALREADY_EXISTS i18n("Archive already exists")
#define IDS_OVERWRITE_QUESTION i18n("Archive already exists. Do you wish to overwrite it?")
#define IDS_DELETE i18n("Delete")
#define IDS_DELETION_PROMPT i18n("Deletion is permanent. Do you wish to delete all the selected files?")
#define IDS_DELETING i18n("Deleting File(s) from Archive")
#define IDS_COREL_ARCHIVER_PREFIX i18n("Archive Administrator: ")
#define IDS_COREL_ARCHIVER i18n("Archive Administrator")
#define IDS_COREL_BLURB i18n("Archive Administrator: v0.5\n(c) 1997 Robert Palmbos\nWith modifications by Corel Corporation (1999)")

#define IDS_NO_FILES_SELECTED i18n("0 Files Selected")
#define IDS_NO_FILES i18n("Total 0 Files, 0 KB")
#define IDS_TOTAL i18n("Total")
#define IDS_FILES i18n("Files")
#define IDS_FILE i18n("File")
#define IDS_SELECTED i18n("Selected")
#define IDS_YES i18n("Yes")
#define IDS_NO i18n("No")
#define IDS_JUST_CURRENT i18n("Just the current file")
#define IDS_CANCEL i18n("Cancel")

#define IDS_NO_ARCHIVE1 i18n("There is no archive currently open. Do you wish to open one now for these files?")
#define IDS_NO_ARCHIVE2 i18n("There is no archive currently open. Do you wish to open one now for this file?")

#define IDS_QUESTION i18n("Question")
#define IDS_ARCH_DRAG i18n("Do you wish to add this to the current archive or open it as a new archive?")

#define IDS_ADD i18n("Add")
#define IDS_OPEN i18n("Open")

#define IDS_ARCH_CREATE i18n("You are currently working with a simple compressed file.\nWould you like to make it into an archive so that it can contain multiple files?\nIf so, you must choose a name for your new archive.")

#define IDS_OK i18n("OK")

#define IDS_DELETE_CONFIRM i18n("Deletion is permanent. Do you wish to delete this file?")

#define IDS_NO_EXTENSION i18n("Your file is missing an extension to indicate the archive type.\nShall create a file of the default type (TGZ)?")

#define IDS_WARNING i18n("Warning")
#define IDS_ARCHIVE_LOCKED i18n("Archive Administrator has detected a lock file for this archive.\nThis may mean that you are already viewing this archive in another\nwindow. If you make changes here, it could cause inconsistencies there.\n\nAre you sure you wish to continue?")

#define IDS_BLURB i18n("Ark version: %1")
#define IDS_ARCHIVER "ark"

#endif //  __ARKSTRINGS_H__
