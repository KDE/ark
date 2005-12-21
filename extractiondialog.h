/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C)
 *
 * 1997-1999: Rob Palmbos palm9744@kettering.edu
 * 1999: Francois-Xavier Duranceau duranceau@kde.org
 * 1999-2000: Corel Corporation (author: Emily Ezust emilye@corel.com)
 * 2001: Corel Corporation (author: Michael Jarrett, michaelj@corel.com)
 * 2001: Roberto Selbach Teixeira (maragato@conectiva.com)
 * 2005: Henrique Pinto ( henrique.pinto@kdemail.net )
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#ifndef EXTRACTIONDIALOG_H
#define EXTRACTIONDIALOG_H

#include <qcheckbox.h>

#include <kurl.h>
#include <kdialogbase.h>

class QRadioButton;

class KURLRequester;

class ExtractionDialog : public KDialogBase
{
	Q_OBJECT
	public:
		/**
		 * Constructor.
		 */
		ExtractionDialog( QWidget *parent = 0, const char *name = 0,
		                  bool enableSelected = true,
		                  const KURL &defaultExtractionDir = KURL(),
		                  const QString &prefix = QString(),
		                  const QString &archiveName = QString::null );

		/**
		 * Destructor.
		 */
		~ExtractionDialog();



		/**
		 * Returns true if the user wants to extract only the selected files
		 */
		bool selectedOnly() const { return m_selectedOnly; }

		/**
		 * Returns the directory the files should be extracted to.
		 */
		KURL extractionDirectory() const { return m_extractionDirectory; }

		/**
		 * Returns true if the user wants the extraction folder to be opened after extraction
		 */
		bool viewFolderAfterExtraction() const { return m_viewFolderAfterExtraction->isChecked(); }



	public slots:
		void accept();
		void extractDirChanged( const QString & );



	private:
		QRadioButton  *m_selectedButton;
		QRadioButton  *m_allButton;
		QCheckBox     *m_viewFolderAfterExtraction;
		bool           m_selectedOnly;
		KURL           m_extractionDirectory;
		KURLRequester *m_urlRequester;
		QString        m_defaultExtractionDir;
		QString        m_prefix;
};

#endif //  EXTRACTIONDIALOG_H
// kate: space-indent off; tab-width 4;

