/*

 $Id$

 ark -- archiver for the KDE project

 Copyright (C)

 1997-1999: Rob Palmbos palm9744@kettering.edu
 1999: Francois-Xavier Duranceau duranceau@kde.org
 2001: Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
 2001-2002: Roberto Teixeira <maragato@kde.org>

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
 Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/

// Qt includes
#include <qlayout.h>
#include <qgroupbox.h>
#include <qcheckbox.h>

// KDE includes
#include <klocale.h>
#include <kiconloader.h>

// ark includes
#include "arksettings.h"
#include "dirDlg.h"
#include "generalOptDlg.h"

#define DLG_NAME I18N_NOOP("Configure")
#define TAB_ADD_NAME I18N_NOOP("Addi&ng")
#define TAB_EXTRACT_NAME I18N_NOOP("&Extracting")
#define TAB_PATH_NAME I18N_NOOP("&Folders")

#define GRP_ADDSET I18N_NOOP("Add Settings")
#define GRP_EXTRACTSET I18N_NOOP("Extract Settings")

#define OPT_REPLACE_NEWER I18N_NOOP("Replace old files only &with newer files")
#define OPT_MAKEGENERIC I18N_NOOP("Keep entries &generic (Lha)")
#define OPT_DOS_FILENAMES I18N_NOOP("Force &MS-DOS short filenames (Zip)")
#define OPT_CONV_CRLF I18N_NOOP("Translate &LF to DOS CRLF (Zip)")
#define OPT_RECURSE_SUBDIRS I18N_NOOP("&Recursively add subfolders (Zip, Rar)")
#define OPT_STORE_SYMLINKS I18N_NOOP("&Store symlinks as links (Zip, Rar)")

#define OPT_OVERWRITE I18N_NOOP("O&verwrite files (Zip, Tar, Zoo, Rar)")
#define OPT_PRESERVEPERMS I18N_NOOP("&Preserve permissions (Tar)")
#define OPT_DISCARDPATHS I18N_NOOP("&Ignore folder names (Zip)")
#define OPT_TOLOWER I18N_NOOP("Convert filenames to &lowercase (Zip, Rar)")
#define OPT_TOUPPER I18N_NOOP("Convert filenames to &uppercase (Rar)")

// little helper:
static inline QPixmap loadIcon( const char * name ) {
  return KGlobal::instance()->iconLoader()
    ->loadIcon( QString::fromLatin1(name),
                KIcon::NoGroup, KIcon::SizeMedium );
}

GeneralOptDlg::GeneralOptDlg(ArkSettings *_d, QWidget *_parent, const char *_name)
	: KDialogBase(IconList, i18n(DLG_NAME), Ok | Apply | Cancel, Ok,
		      _parent, _name, true, true)
{
    m_settings = _d;
    QFrame *frame;

    frame = addPage( i18n( "Addition" ), i18n( "File Addition Settings" ),
                     loadIcon( "ark_addfile" ) );
    createAddTab( frame );

    frame = addPage( i18n( "Extraction" ), i18n( "Extraction Settings" ),
                     loadIcon( "ark_extract" ) );
    createExtractTab( frame );

    frame = addPage( i18n( "Folders" ), i18n( "Folder Settings" ),
                     loadIcon( "folder" ) );
    createDirectoryTab( frame );
}

void GeneralOptDlg::createAddTab( QFrame *parent ) {
    //QVBox *addFrame = addVBoxPage(i18n(TAB_ADD_NAME));
    QFrame *addFrame( parent );

    QVBoxLayout *layout = new QVBoxLayout(parent);

    m_cbReplaceOnlyWithNewer = new QCheckBox(i18n(OPT_REPLACE_NEWER), addFrame);
    m_cbMakeGeneric = new QCheckBox(i18n(OPT_MAKEGENERIC), addFrame);
    m_cbForceMS = new QCheckBox(i18n(OPT_DOS_FILENAMES), addFrame);
    m_cbConvertCRLF = new QCheckBox(i18n(OPT_CONV_CRLF), addFrame);
    m_cbStoreSymlinks = new QCheckBox(i18n(OPT_STORE_SYMLINKS), addFrame);
    m_cbRecurseSubdirs = new QCheckBox(i18n(OPT_RECURSE_SUBDIRS), addFrame);

    layout->addWidget(m_cbReplaceOnlyWithNewer);
    layout->addWidget(m_cbMakeGeneric);
    layout->addWidget(m_cbForceMS);
    layout->addWidget(m_cbConvertCRLF);
    layout->addWidget(m_cbStoreSymlinks);
    layout->addWidget(m_cbRecurseSubdirs);

    layout->addStretch();

    readAddSettings();
    connect(this, SIGNAL(applyClicked()), SLOT(writeAddSettings()));
    connect(this, SIGNAL(okClicked()), SLOT(writeAddSettings()));
}

void GeneralOptDlg::createExtractTab( QFrame *parent ) {
    QFrame *exFrame( parent );// = addVBoxPage(i18n(TAB_EXTRACT_NAME));
    QVBoxLayout *layout = new QVBoxLayout(exFrame);

    m_cbOverwrite = new QCheckBox(i18n(OPT_OVERWRITE), exFrame);
    m_cbPreservePerms = new QCheckBox(i18n(OPT_PRESERVEPERMS), exFrame);
    m_cbDiscardPathnames = new QCheckBox(i18n(OPT_DISCARDPATHS), exFrame);
    m_cbToLower = new QCheckBox(i18n(OPT_TOLOWER), exFrame);
    m_cbToUpper = new QCheckBox(i18n(OPT_TOUPPER), exFrame);

    layout->addWidget(m_cbOverwrite);
    layout->addWidget(m_cbPreservePerms);
    layout->addWidget(m_cbDiscardPathnames);
    layout->addWidget(m_cbToLower);
    layout->addWidget(m_cbToUpper);

    layout->addStretch();

    readExtractSettings();
    connect(this, SIGNAL(applyClicked()), SLOT(writeExtractSettings()));
    connect(this, SIGNAL(okClicked()), SLOT(writeExtractSettings()));
}

void GeneralOptDlg::createDirectoryTab( QFrame *parent ) {
    QFrame *dirFrame( parent );// = addPage(i18n(TAB_PATH_NAME));
    QVBoxLayout *layout = new QVBoxLayout(dirFrame);

    // Modified the old dirdlg to inherit widget instead of QDialog
    // Now we can just add it to our dialog frame!
    DirDlg *dirPage = new DirDlg(m_settings, dirFrame);
    connect(this, SIGNAL(applyClicked()), dirPage, SLOT(saveConfig()));
    connect(this, SIGNAL(okClicked()), dirPage, SLOT(saveConfig()));

    layout->add( dirPage );

    layout->addStretch();
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
