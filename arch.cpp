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
#include "arkwidget.h"
#include "arkutils.h"
#include "filelistview.h"

// C includes
#include <stdlib.h>
#include <time.h>

// QT includes
#include <QApplication>
#include <QFile>
#include <QByteArray>

// KDE includes
#include <KDebug>
#include <KMessageBox>
#include <KMimeType>
#include <KLocale>
#include <KPasswordDialog>
#include <K3Process>
#include <KStandardDirs>

// the archive types
#include "libarchivehandler.h"

Arch::Arch( ArkWidget *gui, const QString &filename )
  : m_filename( filename ), m_gui( gui ),
    m_readOnly( false )
{
}

Arch::~Arch()
{
}

Arch *Arch::archFactory( ArchType aType,
                         ArkWidget *parent, const QString &filename,
                         const QString &openAsMimeType )
{
	return new LibArchiveHandler( parent, filename );
}
#include "arch.moc"
