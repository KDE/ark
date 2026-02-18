/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef CLIZIPTEST_H
#define CLIZIPTEST_H

#include "pluginmanager.h"

using namespace Kerfuffle;

class CliZipTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
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
