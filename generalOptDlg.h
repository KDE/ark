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

#ifndef GENERAL_DLG_H
#define GENERAL_DLG_H

class QWidget;
class QCheckBox;

#include <kdialogbase.h>

class ArkSettings;


class GeneralOptDlg : public KDialogBase
{
	Q_OBJECT
public:
	GeneralOptDlg(ArkSettings *_d, QWidget *_parent=0, const char *_name=0);
	
protected:
	void createAddTab( QFrame* );
	void createExtractTab( QFrame* );
	void createDirectoryTab( QFrame *);

protected slots:
	void readAddSettings();
	void writeAddSettings();

	void readExtractSettings();
	void writeExtractSettings();

private:
	ArkSettings *m_settings;

	// Extract options
	QCheckBox *m_cbOverwrite, *m_cbPreservePerms;
	QCheckBox *m_cbToLower, *m_cbToUpper, *m_cbDiscardPathnames;

	// Add options
	QCheckBox *m_cbReplaceOnlyWithNewer, *m_cbStoreSymlinks;
	QCheckBox *m_cbMakeGeneric, *m_cbForceMS, *m_cbConvertCRLF;
	QCheckBox *m_cbRecurseSubdirs, *m_cbJunkDirNames;
};

#endif /* GENERAL_DLG_H */

