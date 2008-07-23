/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv (at@at) stud.ntnu.no>
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

#ifndef BATCHEXTRACT_H
#define BATCHEXTRACT_H

#include <KParts/MainWindow>
#include <KParts/ReadWritePart>
#include <KUrl>
#include <KDialog>

class Interface;

class BatchExtract: public KDialog
{
	Q_OBJECT
	public:
		BatchExtract( QWidget *parent = 0 );
		~BatchExtract();
		bool loadPart();

	public slots:
		//void openUrl( const KUrl& url );
		//void setShowExtractDialog(bool);
		void slotExtractUrl();

	private slots:

	private:
		KParts::ReadWritePart *m_part;
		KParts::OpenUrlArguments m_openArgs;
		Interface *arkInterface;
};

#endif // BATCHEXTRACT_H
