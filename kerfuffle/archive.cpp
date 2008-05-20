/*
 * Copyright (c) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "archive.h"
#include "archivefactory.h"

#include <QFile>

#include <KDebug>
#include <KMimeType>
#include <KMimeTypeTrader>
#include <KServiceTypeTrader>
#include <KLibLoader>

static bool comparePlugins( const KService::Ptr &p1, const KService::Ptr &p2 )
{
	return ( p1->property( "X-KDE-Priority" ).toInt() ) > ( p2->property( "X-KDE-Priority" ).toInt() );
}

namespace Kerfuffle
{
	Archive *factory( const QString & filename, const QString & requestedMimeType )
	{
		kDebug( 1601 ) ;
		qRegisterMetaType<ArchiveEntry>( "ArchiveEntry" );
		QString mimeType = requestedMimeType.isEmpty()? KMimeType::findByPath( filename )->name() : requestedMimeType;
		KService::List offers = KMimeTypeTrader::self()->query( mimeType, "Kerfuffle/Plugin", "(exist Library)" );

		qSort( offers.begin(), offers.end(), comparePlugins );

		if ( !offers.isEmpty() )
		{
			QString libraryName = offers[ 0 ]->library();
			KLibrary *lib = KLibLoader::self()->library( QFile::encodeName( libraryName ), QLibrary::ExportExternalSymbolsHint );

			kDebug( 1601 ) << "Loading library " << libraryName ;
			if ( lib )
			{
				ArchiveFactory *( *pluginFactory )() = ( ArchiveFactory *( * )() )lib->resolveFunction( "pluginFactory" );
				if ( pluginFactory )
				{
					ArchiveFactory *factory = pluginFactory(); // TODO: cache these
					Archive *arch = factory->createArchive( filename, 0 );
					delete factory;
					return arch;
				}
			}
			kDebug( 1601 ) << "Couldn't load library " << libraryName ;
		}
		kDebug( 1601 ) << "Couldn't find a library capable of handling " << filename ;
		return 0;
	}

	QStringList supportedMimeTypes()
	{
		QStringList supported;
		KService::List offers = KServiceTypeTrader::self()->query( "Kerfuffle/Plugin", "(exist Library)" );

		foreach( const KService::Ptr& service, offers )
		{
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

	QStringList supportedWriteMimeTypes()
	{
		QStringList supported;
		KService::List offers = KServiceTypeTrader::self()->query( "Kerfuffle/Plugin", "(exist Library) and ([X-KDE-Kerfuffle-ReadWrite] == true)" );

		foreach( const KService::Ptr& service, offers )
		{
			foreach( const QString& mimeType, service->serviceTypes() )
			{
				if ( !mimeType.contains( "Kerfuffle" ) )
				{
					supported << mimeType;
				}
			}
		}
		kDebug( 1601 ) << "Returning" << supported;
		return supported;
	}
} // namespace Kerfuffle
