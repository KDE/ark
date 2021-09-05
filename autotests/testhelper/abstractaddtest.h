/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#ifndef ABSTRACTADDTEST_H
#define ABSTRACTADDTEST_H

#include "archive_kerfuffle.h"
#include "pluginmanager.h"

/**
 * Base class for tests about Add/Copy/Move jobs.
 * Make sure to call the constructor in order to load the plugins.
 */
class AbstractAddTest : public QObject
{
    Q_OBJECT

public:

    static QStringList getEntryPaths(Kerfuffle::Archive *archive);

    /**
     * Setup test cases for each format and for each plugin.
     * @param testName Name of the test case shown in the debug output
     * @param archiveName Archive name. The extension of the tested formats will be appended to it.
     * @param targetEntries Entries passed to the job.
     * @param destination Destination entry passed to the job.
     * @param expectedNewPaths New expected paths that the job should create.
     * @param numberOfEntries Number of entries in the archive expected after the job ends.
     */
    void setupRows(const QString &testName, const QString &archiveName, const QVector<Kerfuffle::Archive::Entry*> &targetEntries, Kerfuffle::Archive::Entry *destination, const QStringList &expectedNewPaths, uint numberOfEntries) const;

protected:

    Kerfuffle::PluginManager m_pluginManager;
};

#endif //ABSTRACTADDTEST_H
