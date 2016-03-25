/*
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#ifndef CLIUNARCHIVERTEST_H
#define CLIUNARCHIVERTEST_H

#include "cliplugin.h"
#include <QObject>

using namespace Kerfuffle;

class CliUnarchiverTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase();
    void testArchive_data();
    void testArchive();
    void testList_data();
    void testList();
    void testExtraction_data();
    void testExtraction();
    void testExtractArgs_data();
    void testExtractArgs();

private:

    KPluginMetaData m_pluginMetadata;
};

Q_DECLARE_METATYPE(ArchiveEntry)

#endif
