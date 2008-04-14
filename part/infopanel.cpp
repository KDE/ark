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
#include <QFileInfo>

#include <KLocale>
#include <KMimeType>
#include <KIconLoader>
#include <KIO/NetAccess>

using namespace Kerfuffle;

static QPixmap EnormousMimeIcon( const QString& mimeName )
{
	return KIconLoader::global()->loadMimeTypeIcon( mimeName, KIconLoader::Desktop, KIconLoader::SizeEnormous );
}

InfoPanel::InfoPanel( ArchiveModel *model, QWidget *parent )
	: QFrame( parent ), m_model( model )
{
	setupUi( this );
	setDefaultValues();
	iconLabel->setFixedHeight( KIconLoader::SizeEnormous );
	iconLabel->setMinimumWidth( KIconLoader::SizeEnormous );
	setMaximumWidth( 2 * KIconLoader::SizeEnormous );
}

InfoPanel::~InfoPanel()
{
}

void InfoPanel::setDefaultValues()
{
	iconLabel->setPixmap( KIconLoader::global()->loadIcon( "utilities-file-archiver", KIconLoader::Desktop, KIconLoader::SizeEnormous ) );
	if ( !m_model->archive() )
	{
		fileName->setText( QString( "<center><font size=+1><b>%1</b></font></center>" ).arg( i18n( "No archive loaded" ) ) );
		additionalInfo->setText( QString() );
	}
	else
	{
		QFileInfo archiveInfo( m_model->archive()->fileName() );
		fileName->setText( QString( "<center><font size=+1><b>%1</b></font></center>" ).arg( archiveInfo.fileName() ) );
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

		KMimeType::Ptr mimeType;

		if ( entry[ IsDirectory ].toBool() )
		{
			mimeType = KMimeType::mimeType( "inode/directory" );
		}
		else
		{
			mimeType = KMimeType::findByPath( entry[ FileName ].toString(), 0, true );
		}

		iconLabel->setPixmap( EnormousMimeIcon( mimeType->iconName() ) );
		if ( entry[ IsDirectory ].toBool() )
		{
			additionalInfo->setText( i18np( "One item", "%1 items", m_model->childCount( index ) ) );
		}
		else if ( entry.contains( Link ) )
		{
			additionalInfo->setText( i18n( "Symbolic Link" ) );
		}
		else
		{
			additionalInfo->setText( KIO::convertSize( entry[ Size ].toULongLong() ) );
		}

		QStringList nameParts = entry[ FileName ].toString().split( '/', QString::SkipEmptyParts );
		QString name = ( nameParts.count() > 0 )? nameParts.last() : entry[ FileName ].toString();
		fileName->setText( QString( "<center><font size=+1><b>%1</b></font></center>" ).arg( name ) );

		metadataLabel->setText( metadataTextFor( index ) );
		showMetaData();
	}
}

void InfoPanel::setIndexes( const QModelIndexList &list )
{
	if ( list.size() == 0 )
	{
		setIndex( QModelIndex() );
	}
	else if ( list.size() == 1 )
	{
		setIndex( list[ 0 ] );
	}
	else
	{
		// TODO: set the icon
		fileName->setText( QString( "<center><font size=+1><b>%1</b></font></center>" ).arg( i18np( "One file selected", "%1 files selected", list.size() ) ) );
		quint64 totalSize = 0;
		foreach( const QModelIndex& index, list )
		{
			const ArchiveEntry& entry = m_model->entryForIndex( index );
			totalSize += entry[ Size ].toULongLong();
		}
		additionalInfo->setText( KIO::convertSize( totalSize ) );
		hideMetaData();
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

QString InfoPanel::metadataTextFor( const QModelIndex &index )
{
	const ArchiveEntry& entry = m_model->entryForIndex( index );
	QString text;

	KMimeType::Ptr mimeType;

	if ( entry[ IsDirectory ].toBool() )
	{
		mimeType = KMimeType::mimeType( "inode/directory" );
	}
	else
	{
		mimeType = KMimeType::findByPath( entry[ FileName ].toString(), 0, true );
	}

	text += i18n( "<b>Type:</b> %1<br/>",  mimeType->comment() );

	if ( entry.contains( Owner ) )
	{
		text += i18n( "<b>Owner:</b> %1<br/>", entry[ Owner ].toString() );
	}

	if ( entry.contains( Group ) )
	{
		text += i18n( "<b>Group:</b> %1<br/>", entry[ Group ].toString() );
	}

	if ( entry.contains( Link ) )
	{
		text += i18n( "<b>Target:</b> %1<br/>", entry[ Link ].toString() );
	}

	return text;
}

#include "infopanel.moc"
