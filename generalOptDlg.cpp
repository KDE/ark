/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 2001: Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)

 This program is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License
 as published by the Free Software Foundation; either version 2
 of the License, or (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

// Qt includes
#include <qvbox.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

// KDE includes
#include <klocale.h>

// ark includes
#include "arksettings.h"
#include "dirDlg.h"
#include "generalOptDlg.h"

#define DLG_NAME i18n("Settings")
#define TAB_ADD_NAME i18n("&Adding")
#define TAB_EXTRACT_NAME i18n("&Extracting")
#define TAB_PATH_NAME i18n("&Directories")

#define GRP_ADDSET i18n("Add Settings")
#define GRP_EXTRACTSET i18n("Extract Settings")

#define OPT_REPLACE_NEWER i18n("Replace &old files only with newer files")
#define OPT_MAKEGENERIC i18n("Keep entries &generic (Lha)")
#define OPT_DOS_FILENAMES i18n("Force &MS-DOS short filenames (Zip)")
#define OPT_CONV_CRLF i18n("Tranlate LF to DOS &CRLF (Zip)")
#define OPT_RECURSE_SUBDIRS i18n("&Recursively add subdirectories (Zip, Rar)")
#define OPT_STORE_SYMLINKS i18n("&Store symlinks as links (Zip, Rar)")

#define OPT_OVERWRITE i18n("O&verwrite files (Zip, Tar, Zoo, Rar)")
#define OPT_PRESERVEPERMS i18n("&Preserve permissions (Tar)")
#define OPT_DISCARDPATHS i18n("&Ignore directory names (Zip)")
#define OPT_TOLOWER i18n("Convert filenames to &lowercase (Zip, Rar)")
#define OPT_TOUPPER i18n("Convert filenames to &uppercase (Rar)")

GeneralOptDlg::GeneralOptDlg(ArkSettings *_d, QWidget *_parent, const char *_name)
	: KDialogBase(KDialogBase::Tabbed, DLG_NAME, Ok | Apply | Cancel, Ok,
		      _parent, _name)
{
	m_settings = _d;

	createAddTab();
        createExtractTab();
	createDirectoryTab();	
}

void GeneralOptDlg::createAddTab()
{
	QVBox *addFrame = addVBoxPage(TAB_ADD_NAME);

	QGroupBox *addSet = new QGroupBox(1, Horizontal, GRP_ADDSET, addFrame);
	
	m_cbReplaceOnlyWithNewer = new QCheckBox(OPT_REPLACE_NEWER, addSet);
	m_cbMakeGeneric = new QCheckBox(OPT_MAKEGENERIC, addSet);
	m_cbForceMS = new QCheckBox(OPT_DOS_FILENAMES, addSet);
	m_cbConvertCRLF = new QCheckBox(OPT_CONV_CRLF, addSet);
	m_cbStoreSymlinks = new QCheckBox(OPT_STORE_SYMLINKS, addSet);
	m_cbRecurseSubdirs = new QCheckBox(OPT_RECURSE_SUBDIRS, addSet);
	
	readAddSettings();
	connect(this, SIGNAL(applyClicked()), SLOT(writeAddSettings()));
	connect(this, SIGNAL(okClicked()), SLOT(writeAddSettings()));
}

void GeneralOptDlg::createExtractTab()
{
	QFrame *exFrame = addVBoxPage(TAB_EXTRACT_NAME);

	QGroupBox *exSet = new QGroupBox(1, Horizontal, GRP_EXTRACTSET, exFrame);

	m_cbOverwrite = new QCheckBox(OPT_OVERWRITE, exSet);
	m_cbPreservePerms = new QCheckBox(OPT_PRESERVEPERMS, exSet);
	m_cbDiscardPathnames = new QCheckBox(OPT_DISCARDPATHS, exSet);
	m_cbToLower = new QCheckBox(OPT_TOLOWER, exSet);
	m_cbToUpper = new QCheckBox(OPT_TOUPPER, exSet);

	readExtractSettings();
	connect(this, SIGNAL(applyClicked()), SLOT(writeExtractSettings()));
	connect(this, SIGNAL(okClicked()), SLOT(writeExtractSettings()));
}

void GeneralOptDlg::createDirectoryTab()
{
	QFrame *dirFrame = addPage(TAB_PATH_NAME);

	// Modified the old dirdlg to inherit widget instead of QDialog
	// Now we can just add it to our dialog frame!
	DirDlg *dirPage = new DirDlg(m_settings, dirFrame);
	connect(this, SIGNAL(applyClicked()), dirPage, SLOT(saveConfig()));
	connect(this, SIGNAL(okClicked()), dirPage, SLOT(saveConfig()));

	dirFrame->setMinimumSize(dirPage->minimumSize());
}

void GeneralOptDlg::readAddSettings()
{
	m_cbReplaceOnlyWithNewer->setChecked(m_settings->getAddReplaceOnlyWithNewer());
	m_cbMakeGeneric->setChecked(m_settings->getLhaGeneric());
	m_cbForceMS->setChecked(m_settings->getZipAddMSDOS());
	m_cbConvertCRLF->setChecked(m_settings->getZipAddConvertLF());

	m_cbStoreSymlinks->setChecked(m_settings->getRarStoreSymlinks());
	m_settings->setZipStoreSymlinks(m_settings->getRarStoreSymlinks());
	m_cbRecurseSubdirs->setChecked(m_settings->getRarRecurseSubdirs());
	m_settings->setZipAddRecurseDirs(m_settings->getRarRecurseSubdirs());
}

void GeneralOptDlg::writeAddSettings()
{
	m_settings->setAddReplaceOnlyWithNewer(m_cbReplaceOnlyWithNewer->isChecked());
	m_settings->setLhaGeneric(m_cbMakeGeneric->isChecked());
	m_settings->setZipAddMSDOS(m_cbForceMS->isChecked());
	m_settings->setZipAddConvertLF(m_cbConvertCRLF->isChecked());

	// TODO: Fix dupe
	m_settings->setZipStoreSymlinks(m_cbStoreSymlinks->isChecked());
	m_settings->setRarStoreSymlinks(m_cbStoreSymlinks->isChecked());
	m_settings->setZipAddRecurseDirs(m_cbRecurseSubdirs->isChecked());
	m_settings->setRarRecurseSubdirs(m_cbRecurseSubdirs->isChecked());
}

void GeneralOptDlg::readExtractSettings()
{
	m_cbOverwrite->setChecked(m_settings->getExtractOverwrite());
	m_cbPreservePerms->setChecked(m_settings->getTarPreservePerms());
	m_cbDiscardPathnames->setChecked(m_settings->getZipExtractJunkPaths());
	m_cbToLower->setChecked(m_settings->getRarExtractLowerCase());
	m_settings->setZipExtractLowerCase(m_settings->getRarExtractLowerCase());
	m_cbToUpper->setChecked(m_settings->getRarExtractUpperCase());
}

void GeneralOptDlg::writeExtractSettings()
{
	m_settings->setExtractOverwrite(m_cbOverwrite->isChecked());
	m_settings->setTarPreservePerms(m_cbPreservePerms->isChecked());
	m_settings->setZipExtractJunkPaths(m_cbDiscardPathnames->isChecked());
	m_settings->setRarExtractUpperCase(m_cbToUpper->isChecked());

	// TOOD: Fix dupe
	m_settings->setZipExtractLowerCase(m_cbToLower->isChecked());
	m_settings->setRarExtractLowerCase(m_cbToLower->isChecked());
}

#include "generalOptDlg.moc"
