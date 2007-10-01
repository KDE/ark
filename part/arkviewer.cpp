/*
 * ark: A program for modifying archives via a GUI.
 *
 * Copyright (C) 2004, Henrique Pinto <henrique.pinto@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
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

#include "arkviewer.h"

#include <KLocale>
#include <KParts/ComponentFactory>
#include <KMimeType>
#include <KDebug>
#include <KUrl>
#include <KGlobal>
#include <KIconLoader>
#include <KVBox>

#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>

ArkViewer::ArkViewer( QWidget * parent )
	: KDialog( parent), m_part( 0 )
{
	setButtons( Close );
	m_widget = new KVBox( this );
	m_widget->layout()->setSpacing( 10 );

	connect( this, SIGNAL( finished() ), this, SLOT( slotFinished() ) );

	setMainWidget( m_widget );
}

ArkViewer::~ArkViewer()
{
}

void ArkViewer::slotFinished()
{
	delete m_part;
	m_part = 0;
	delayedDestruct();
}

bool ArkViewer::view( const QString& filename )
{
	KUrl u( filename );

	KMimeType::Ptr mimetype = KMimeType::findByUrl( u, 0, true );

	setCaption( u.fileName() );
	// TODO: Load the size from the config file
	QSize size = QSize();
	if (size.width() < 200)
		size = QSize(560, 400);
	setInitialSize( size );

	QFrame *header = new QFrame( m_widget );
	QHBoxLayout *headerLayout = new QHBoxLayout( header );

	QLabel *iconLabel = new QLabel( header );
        headerLayout->addWidget(iconLabel);
	iconLabel->setPixmap( KIconLoader::global()->loadMimeTypeIcon(mimetype->iconName(), KIconLoader::Desktop ) );
	iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

	KVBox *headerRight = new KVBox( header );
        headerLayout->addWidget(headerRight);
	new QLabel( QString( "<qt><b>%1</b></qt>" )
	                     .arg( KUrl( filename ).fileName() ), headerRight
	          );
	new QLabel( mimetype->comment(), headerRight );

	header->setSizePolicy( QSizePolicy::Expanding, QSizePolicy::Maximum );

	m_part = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>( mimetype->name(), QString(), m_widget, this );

	if ( m_part )
	{
		m_part->openUrl( filename );
		exec();
		return true;
	}
	else
	{
		return false;
	}
}

#include "arkviewer.moc"
