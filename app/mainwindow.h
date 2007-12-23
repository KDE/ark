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
#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <KParts/MainWindow>
#include <KParts/ReadWritePart>
#include <KUrl>

class KRecentFilesAction;

class MainWindow: public KParts::MainWindow
{
	Q_OBJECT
	public:
		MainWindow( QWidget *parent = 0 );
		~MainWindow();

	public slots:
		void openUrl( const KUrl& url );

	private slots:
		void updateActions();
		void newArchive();
		void openArchive();
		void quit();

		void editKeyBindings();
		void editToolbars();

	private:
		void setupActions();

		KParts::ReadWritePart *m_part;
		KRecentFilesAction    *m_recentFilesAction;
		QAction               *m_openAction;
		QAction               *m_newAction;
};

#endif // MAINWINDOW_H
