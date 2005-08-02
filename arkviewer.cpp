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
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "arkviewer.h"

#include <klocale.h>
#include <kparts/componentfactory.h>
#include <kmimetype.h>
#include <kdebug.h>
#include <kurl.h>
#include <kglobal.h>
#include <kiconloader.h>

#include <qvbox.h>
#include <qlayout.h>
#include <qlabel.h>
#include <qframe.h>
#include <qurl.h>


ArkViewer::ArkViewer( QWidget * parent, const char * name )
	: KDialogBase( parent, name, false, QString::null, Close ), m_part( 0 )
{
	m_widget = new QVBox( this );

	connect( this, SIGNAL( finished() ), this, SLOT( slotFinished() ) );

	setMainWidget( m_widget );
}

ArkViewer::~ArkViewer()
{
	saveDialogSize( "ArkViewer" );
}

void ArkViewer::slotFinished()
{
	delete m_part;
	m_part = 0;
	delayedDestruct();
}

bool ArkViewer::view( const QString& filename )
{
	kdDebug( 1601 ) << "ArkViewer::view(): viewing " << filename << endl;

	KURL u( filename );

	KMimeType::Ptr mimetype = KMimeType::findByURL( u, 0, true );

	setCaption( u.fileName() );

	QSize size = configDialogSize( "ArkViewer" );
	if (size.width() < 200)
		size = QSize(560, 400);
	setInitialSize( size );

	kdDebug( 1601 ) << "ArkViewer::view(): mimetype = " << mimetype << endl;

	QFrame *header = new QFrame( m_widget );
	QHBoxLayout *headerLayout = new QHBoxLayout( header );
	headerLayout->setAutoAdd( true );

	QLabel *iconLabel = new QLabel( header );
	iconLabel->setPixmap( mimetype->pixmap( KIcon::Desktop ) );
	iconLabel->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Minimum );

	QVBox *headerRight = new QVBox( header );
	new QLabel( QString( "<qt><b>%1</b></qt>" )
	                     .arg( QUrl( filename ).fileName() ), headerRight
	          );
	new QLabel( mimetype->comment(), headerRight );

	m_part = KParts::ComponentFactory::createPartInstanceFromQuery<KParts::ReadOnlyPart>( mimetype->name(), QString::null, m_widget, 0, this );

	if ( m_part )
	{
		m_part->openURL( filename );
		show();
		return true;
	}
	else
	{
		return false;
	}
}

#include "arkviewer.moc"
