/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal
 * <haraldhv atatatat stud.ntnu.no>
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

#include "adddialog.h"
#include "ui_adddialog.h"
#include "kerfuffle/archive.h"

namespace Kerfuffle
{
	class AddDialogUI: public QWidget, public Ui::AddDialog
	{
		public:
			AddDialogUI( QWidget *parent = 0 )
				: QWidget( parent )
			{
				setupUi( this );
			}
	};

	AddDialog::AddDialog(const QStringList& itemsToAdd,
			const KUrl & 	startDir, 
					const QString & 	filter, 
					QWidget * 	parent, 
					QWidget * 	widget
					)
		: KFileDialog(startDir, filter, parent, widget)
	{
		setOperationMode(KFileDialog::Saving);
		setMode(KFile::File |
				KFile::LocalOnly );
		setCaption("Compress to archive");

		m_ui = new AddDialogUI( this );
		mainWidget()->layout()->addWidget(m_ui);

		foreach(const QString& item, itemsToAdd) {
			m_ui->groupCompressFiles->layout()->addWidget(
					new QLabel(item));
		}


		//These extra options will be implemented in a 4.2+ version of
		//ark
		m_ui->groupExtraOptions->hide();

		setMimeFilter(Kerfuffle::supportedWriteMimeTypes());

	}

}
