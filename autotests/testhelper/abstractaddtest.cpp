/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "abstractaddtest.h"

#include "jobs.h"
#include "testhelper.h"

#include <QMimeDatabase>
#include <QTest>

using namespace Kerfuffle;

QStringList AbstractAddTest::getEntryPaths(Archive *archive)
{
    QStringList paths;
    auto loadJob = Archive::load(archive->fileName());
    QObject::connect(loadJob, &Job::newEntry, [&paths](Archive::Entry *entry) {
        paths << entry->fullPath();
    });
    TestHelper::startAndWaitForResult(loadJob);

    return paths;
}

void AbstractAddTest::setupRows(const QString &testName,
                                const QString &archiveName,
                                const QList<Archive::Entry *> &targetEntries,
                                Archive::Entry *destination,
                                const QStringList &expectedNewPaths,
                                uint numberOfEntries) const
{
    // Repeat the same test case for each format and for each plugin supporting the format.
    const QStringList formats = TestHelper::testFormats();
    for (const QString &format : formats) {
        const QString filename = QStringLiteral("%1.%2").arg(archiveName, format);
        const auto mime = QMimeDatabase().mimeTypeForFile(filename, QMimeDatabase::MatchExtension);

        const auto plugins = m_pluginManager.preferredWritePluginsFor(mime);
        for (const auto plugin : plugins) {
            QTest::newRow(QStringLiteral("%1 (%2, %3)").arg(testName, format, plugin->metaData().pluginId()).toUtf8().constData())
                << filename << plugin << targetEntries << destination << expectedNewPaths << numberOfEntries;
        }
    }
}

#include "moc_abstractaddtest.cpp"
