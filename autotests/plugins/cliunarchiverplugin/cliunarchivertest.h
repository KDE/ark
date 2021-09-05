/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CLIUNARCHIVERTEST_H
#define CLIUNARCHIVERTEST_H

#include "cliplugin.h"
#include "pluginmanager.h"

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
    void testListArgs_data();
    void testListArgs();
    void testExtraction_data();
    void testExtraction();
    void testExtractArgs_data();
    void testExtractArgs();

private:

    PluginManager m_pluginManger;
    Plugin *m_plugin;
};

#endif
