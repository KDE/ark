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
#ifndef INFOPANEL_H
#define INFOPANEL_H

#include <QFrame>
#include "kerfuffle/archive.h"
#include "archivemodel.h"
#include "ui_infopanel.h"


class InfoPanel: public QFrame, Ui::InformationPanel
{
	Q_OBJECT
	public:
		explicit InfoPanel( ArchiveModel *model, QWidget *parent = 0 );
		~InfoPanel();

		void setIndex( const QModelIndex & );
		void setIndexes( const QModelIndexList &list );

	private:
		void setDefaultValues();

		void showMetaData();
		void hideMetaData();

		void showActions();
		void hideActions();

		QString metadataTextFor( const QModelIndex & );

		ArchiveModel *m_model;
};

#endif // INFOPANEL_H
