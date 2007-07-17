/*

 ark -- archiver for the KDE project

 Copyright (C) 2002 Helio Chissini de Castro <helio@conectiva.com.br>
 Copyright (C) 2001 Corel Corporation (author: Michael Jarrett <michaelj@corel.com>)
 Copyright (C) 1999-2000 Corel Corporation (author: Emily Ezust <emilye@corel.com>)
 Copyright (C) 1999 Francois-Xavier Duranceau <duranceau@kde.org>
 Copyright (C) 1997-1999 Rob Palmbos <palm9744@kettering.edu>

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
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

*/

// ark includes
#include "arch.h"
#include "archivebase.h"
#include "archivefactory.h"

// C includes
#include <cstdlib>

// QT includes
#include <QApplication>
#include <QFile>
#include <QByteArray>

// KDE includes
#include <KDebug>
#include <KMessageBox>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KLocale>
#include <KPasswordDialog>
#include <KStandardDirs>
#include <KLibLoader>

Arch::Arch( const QString &filename )
	: QObject( 0 ), m_filename( filename ), m_readOnly( false )
{
}

Arch::~Arch()
{
}

void Arch::extractFile( const QVariant & fileName, const QString & destinationDir )
{
	QList<QVariant> l;
	l << fileName;
	extractFiles( l, destinationDir );
}

Arch *Arch::archFactory( ArchType /*aType*/,
                         const QString &filename,
                         const QString &/*openAsMimeType*/ )
{
	QString mimeType = KMimeType::findByPath( filename )->name();
	KService::List offers = KMimeTypeTrader::self()->query( mimeType, "Kerfuffle/Plugin", "(exist Library)" );

	if ( !offers.isEmpty() )
	{
		QString libraryName = offers[ 0 ]->library();
		kDebug( 1601 ) << k_funcinfo << "Loading library " << libraryName << " for handling mimetype " << mimeType << endl;
		KLibrary *lib = KLibLoader::self()->library( QFile::encodeName( libraryName ), QLibrary::ExportExternalSymbolsHint );
		if ( lib )
		{
			ArchiveFactory *( *pluginFactory )() = ( ArchiveFactory *( * )() )lib->resolveFunction( "pluginFactory" );
			if ( pluginFactory )
			{
				ArchiveFactory *factory = pluginFactory(); // TODO: cache these
				Arch *arch = factory->createArchive( filename, 0 );
				delete factory;
				return arch;
			}
		}
	}
	kDebug( 1601 ) << k_funcinfo << "Couldn't find a library for mimetype " << mimeType << endl;
	return 0;
}

QStringList Arch::supportedMimeTypes()
{
	QStringList supported;
	KService::List offers = KServiceTypeTrader::self()->query( "Kerfuffle/Plugin", "(exist Library)" );

	kDebug( 1601 ) << k_funcinfo << "returned " << offers.count() << " offers" << endl;

	foreach( KService::Ptr service, offers )
	{
		kDebug( 1601 ) << k_funcinfo << "	Service " << service->name() << " has service types " << service->serviceTypes() << endl;
		foreach( const QString& mimeType, service->serviceTypes() )
		{
			if ( !mimeType.contains( "Kerfuffle" ) )
			{
				supported << mimeType;
			}
		}
	}
	return supported;
}

#include "arch.moc"
