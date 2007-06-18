#include "archiveinterface.h"

ReadOnlyArchiveInterface::ReadOnlyArchiveInterface( const QString & filename, QObject *parent )
	: QObject( parent ), m_filename( filename )
{
}

ReadOnlyArchiveInterface::~ReadOnlyArchiveInterface()
{
}

void ReadOnlyArchiveInterface::error( const QString & message, const QString & details )
{
	foreach( ArchiveObserver *observer, m_observers )
	{
		observer->onError( message, details );
	}
}

void ReadOnlyArchiveInterface::entry( const ArchiveEntry & archiveEntry )
{
	foreach( ArchiveObserver *observer, m_observers )
	{
		observer->onEntry( archiveEntry );
	}
}

void ReadOnlyArchiveInterface::progress( double p )
{
	foreach( ArchiveObserver *observer, m_observers )
	{
		observer->onProgress( p );
	}
}

void ReadOnlyArchiveInterface::registerObserver( ArchiveObserver *observer )
{
	m_observers.append( observer );
}

void ReadOnlyArchiveInterface::removeObserver( ArchiveObserver *observer )
{
	m_observers.removeAll( observer );
}

#include "archiveinterface.moc"
