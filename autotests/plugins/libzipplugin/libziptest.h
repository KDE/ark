/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef LIBZIPTEST_H
#define LIBZIPTEST_H

#include "pluginmanager.h"

using namespace Kerfuffle;

class LibzipTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testPathSeparator_data();
    void testPathSeparator();
    void testExtractionWithWindowsSeparators();
    void testMoveWithWindowsSeparators();
    void testLegacyCodePageNames_data();
    void testLegacyCodePageNames();
    void testReplaceLegacyCodePageEntry();

private:
    PluginManager m_pluginManager;
    Plugin *m_plugin;
};

#endif
