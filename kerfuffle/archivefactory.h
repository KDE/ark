#ifndef ARCHIVEFACTORY_H
#define ARCHIVEFACTORY_H

#include "kerfuffle_export.h"
#include "arch.h"
#include "archivebase.h"
#include <QString>
#include <QObject>


class ArchiveFactory
{
	public:
		virtual Arch* createArchive( const QString& filename, QObject *parent ) = 0;
};

template<class T> class ArchiveInterfaceFactory: public ArchiveFactory
{
	public:
		Arch* createArchive( const QString& filename, QObject *parent = 0 )
		{
			return new ArchiveBase( new T( filename, parent ) );
		}
};

#define KERFUFFLE_PLUGIN_FACTORY( classname ) \
	extern "C" { KERFUFFLE_EXPORT ArchiveFactory *pluginFactory() { return new ArchiveInterfaceFactory<classname>(); } }

#endif // ARCHIVEFACTORY_H
