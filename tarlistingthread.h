/*
    This file is part of KDE.

    Copyright (c) 2006 Henrique Pinto <henrique.pinto@kdemail.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
    USA.
*/

#ifndef TARLISTINGTHREAD_H
#define TARLISTINGTHREAD_H

#include <qthread.h>
#include <qstringlist.h>
#include <qevent.h>

class QString;
class KArchive;

class ListingEvent: public QCustomEvent
{
	public:
		enum Status { Normal, Error, ListingFinished };
		ListingEvent( const QStringList& data, Status st = Normal )
			: QCustomEvent( 65442 ), m_data( data ), m_status( st ) {}
		
		const QStringList& columns() const { return m_data; }
		Status status() const { return m_status; }
		
	private:
		QStringList m_data;
		Status      m_status;
};

class TarListingThread: public QThread
{
	public:
		TarListingThread( QObject *parent, const QString& filename );
		~TarListingThread();
	
	protected:
		void run();
	
	private:
		void processDir( const KTarDirectory *tardir, const QString & root );
		
		KArchive *m_archive;
		QObject  *m_parent;
};

#endif // TARLISTINGTHREAD_H
