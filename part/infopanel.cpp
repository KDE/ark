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
#include "kerfuffle/archive.h"

#include <QLabel>
#include <QVBoxLayout>

#include <KLocale>
#include <KMimeType>
#include <KIconLoader>
#include <KIO/NetAccess>

using namespace Kerfuffle;

static QPixmap EnormousMimeIcon( const QString& mimeName )
{
	return KIconLoader::global()->loadMimeTypeIcon( mimeName, K3Icon::Desktop, K3Icon::SizeEnormous );
}

InfoPanel::InfoPanel( ArchiveModel *model, QWidget *parent )
	: QFrame( parent ), m_model( model )
{
	setupUi( this );
	setDefaultValues();
	iconLabel->setFixedHeight( K3Icon::SizeEnormous );
	iconLabel->setMinimumWidth( K3Icon::SizeEnormous );
	setMaximumWidth( 2 * K3Icon::SizeEnormous );
}

InfoPanel::~InfoPanel()
{
}

void InfoPanel::setDefaultValues()
{
	iconLabel->setPixmap( KIconLoader::global()->loadIcon( "ark", K3Icon::Desktop, K3Icon::SizeEnormous ) );
	if ( !m_model->archive() )
	{
		fileName->setText( QString( "<font size=+1><b>%1</b></font>" ).arg( i18n( "No archive loaded" ) ) );
		additionalInfo->setText( QString() );
	}
	else
	{
		fileName->setText( QString( "<font size=+1><b>%1</b></font>" ).arg( i18n( "No file selected" ) ) );
		additionalInfo->setText( QString() );
	}
	hideMetaData();
	hideActions();
}

void InfoPanel::setIndex( const QModelIndex& index )
{
	if ( !index.isValid() )
	{
		setDefaultValues();
	}
	else
	{
		const ArchiveEntry& entry = m_model->entryForIndex( index );

		if ( entry[ IsDirectory ].toBool() )
		{
			iconLabel->setPixmap( EnormousMimeIcon( KMimeType::mimeType( "inode/directory" )->iconName() ) );
			additionalInfo->setText( i18np( "One item", "%1 items", m_model->childCount( index ) ) );
		}
		else
		{
			KMimeType::Ptr mimeType = KMimeType::findByPath( entry[ FileName ].toString(), 0, true );
			iconLabel->setPixmap( EnormousMimeIcon( mimeType->iconName() ) );
			additionalInfo->setText( mimeType->comment() );
		}

		QStringList nameParts = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts );
		QString name = ( nameParts.count() > 0 )? nameParts.last() : entry[ FileName ].toString();
		fileName->setText( QString( "<font size=+1><b>%1</b></font>" ).arg( name ) );

		metadataLabel->setText( metadataTextFor( entry ) );
		showMetaData();
	}
}

void InfoPanel::showMetaData()
{
	firstSeparator->show();
	metadataLabel->show();
}

void InfoPanel::hideMetaData()
{
	firstSeparator->hide();
	metadataLabel->hide();
}

void InfoPanel::showActions()
{
	secondSeparator->show();
	actionsLabel->show();
}

void InfoPanel::hideActions()
{
	secondSeparator->hide();
	actionsLabel->hide();
}

QString InfoPanel::metadataTextFor( const Kerfuffle::ArchiveEntry& entry )
{
	QString text;

	if ( entry.contains( Size ) )
	{
		text += QString( "<b>%1:</b> %2<br/>" ).arg( i18n( "Size" ) ).arg( KIO::convertSize( entry[ Size ].toULongLong() ) );
	}

	if ( entry.contains( Owner ) )
	{
		text += QString( "<b>%1:</b> %2<br/>" ).arg( i18n( "Owner" ) ).arg( entry[ Owner ].toString() );
	}

	if ( entry.contains( Group ) )
	{
		text += QString( "<b>%1:</b> %2<br/>" ).arg( i18n( "Group" ) ).arg( entry[ Group ].toString() );
	}

	if ( text.isEmpty() )
	{
		text = i18n( "No metadata available for this file." );
	}

	return text;
}

#include "infopanel.moc"
