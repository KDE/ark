/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
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


#include <KDirSelectDialog>

#include <KDialog>
#include "ui_extractiondialog.h"

#include "archivemodel.h"

class ExtractionDialogUI: public QFrame, public Ui::ExtractionDialog
{
	Q_OBJECT
	public:
		ExtractionDialogUI( QWidget *parent = 0 );
};

class ExtractionDialog: public KDirSelectDialog
{
	Q_OBJECT
	public:
		ExtractionDialog( QVariantMap& arguments, QWidget *parent = 0 );
		~ExtractionDialog();

	private Q_SLOTS:
		void updateArguments();

	private:
		ExtractionDialogUI *m_ui;
		QVariantMap& arguments;

};

#endif // EXTRACTIONDIALOG_H
