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
#include "infopanel.h"
#include "kerfuffle/arch.h"

#include <QLabel>
#include <QVBoxLayout>

#include <KLocale>
#include <KMimeType>
#include <KIconLoader>

InfoPanel::InfoPanel( QWidget *parent )
	: QWidget( parent ), m_icon( new QLabel( this ) ), m_name( new QLabel( this ) ), m_mimetype( new QLabel( this ) )
{
	QVBoxLayout *layout = new QVBoxLayout;
	layout->addWidget( m_icon );
	layout->addWidget( m_name );
	layout->addWidget( m_mimetype );
	layout->addStretch();
	setLayout( layout );
	setDefaultValues();

	m_icon->setAlignment( Qt::AlignCenter );
	m_name->setAlignment( Qt::AlignCenter );
	m_mimetype->setAlignment( Qt::AlignCenter );
}

InfoPanel::~InfoPanel()
{
}

void InfoPanel::setDefaultValues()
{
	KMimeType::Ptr defaultMime = KMimeType::defaultMimeTypePtr();
	m_icon->setPixmap( KIconLoader::global()->loadMimeTypeIcon( defaultMime->iconName(), K3Icon::Desktop ) );
	m_name->setText( QString( "<font size=+1><b>%1</b></font>" ).arg( i18n( "No file selected" ) ) );
	m_mimetype->setText( QString() );
}

void InfoPanel::setEntry( const ArchiveEntry& entry )
{
	if ( entry.isEmpty() )
	{
		setDefaultValues();
	}
	else
	{
		KMimeType::Ptr mimeType = KMimeType::findByPath( entry[ FileName ].toString(), 0, true );
		m_icon->setPixmap( KIconLoader::global()->loadMimeTypeIcon( mimeType->iconName(), K3Icon::Desktop ) );
		QStringList nameParts = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts );
		QString name = ( nameParts.count() > 0 )? nameParts.last() : entry[ FileName ].toString();
		m_name->setText( QString( "<font size=+1><b>%1</b></font>" ).arg( name ) );
		m_mimetype->setText( mimeType->comment() );
	}
}

#include "infopanel.moc"
