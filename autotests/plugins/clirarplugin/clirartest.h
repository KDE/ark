/*
    SPDX-FileCopyrightText: 2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2015, 2016 Ragnar Thomsen <rthomsen6@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef CLIRARTEST_H
#define CLIRARTEST_H

#include "pluginmanager.h"

using namespace Kerfuffle;

class CliRarTest : public QObject
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
    void testAddArgs_data();
    void testAddArgs();
    void testExtractArgs_data();
    void testExtractArgs();

private:
    PluginManager m_pluginManager;
    Plugin *m_plugin;
};

#endif
