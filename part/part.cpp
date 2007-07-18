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
#include "part.h"
#include "archivemodel.h"

#include <KParts/GenericFactory>
#include <KAboutData>
#include <QTreeView>

typedef KParts::GenericFactory<Part> Factory;
K_EXPORT_COMPONENT_FACTORY( libarkpartnew, Factory );

Part::Part( QWidget *parentWidget, QObject *parent, const QStringList& args )
	: KParts::ReadWritePart( parent ), m_model( new ArchiveModel( this ) ), m_view( new QTreeView( parentWidget ) )
{
	Q_UNUSED( args );
	setComponentData( Factory::componentData() );
	setXMLFile( "ark_part_new.rc" );
	setWidget( m_view );
	m_view->setModel( m_model );
}

Part::~Part()
{
}

KAboutData* Part::createAboutData()
{
	return new KAboutData( "ark", 0, ki18n( "ArkPart" ), "3.0" );
}

bool Part::openFile()
{
	Arch *archive = Arch::factory( localFilePath() );
	m_model->setArchive( archive );

	return ( archive != 0 );
}

bool Part::saveFile()
{
	return true;
}

QStringList Part::supportedMimeTypes() const
{
	return Arch::supportedMimeTypes();
}
