/*
    SPDX-FileCopyrightText: 2022 Ilya Pominov <ipominov@astralinux.ru>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#ifndef CLIARJTEST_H
#define CLIARJTEST_H

#include "pluginmanager.h"

using namespace Kerfuffle;

class CliArjTest : public QObject
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
    PluginManager m_pluginManger;
    Plugin *m_plugin;
};

#endif
