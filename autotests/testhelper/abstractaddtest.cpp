/*
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES ( INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION ) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * ( INCLUDING NEGLIGENCE OR OTHERWISE ) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
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
    QObject::connect(loadJob, &Job::newEntry, [&paths](Archive::Entry* entry) { paths << entry->fullPath(); });
    TestHelper::startAndWaitForResult(loadJob);

    return paths;
}

void AbstractAddTest::setupRows(const QString &testName, const QString &archiveName, const QVector<Archive::Entry *> &targetEntries, Archive::Entry *destination, const QStringList &expectedNewPaths, uint numberOfEntries) const
{
    // Repeat the same test case for each format and for each plugin supporting the format.
    const QStringList formats = TestHelper::testFormats();
    for (const QString &format : formats) {
        const QString filename = QStringLiteral("%1.%2").arg(archiveName, format);
        const auto mime = QMimeDatabase().mimeTypeForFile(filename, QMimeDatabase::MatchExtension);

        const auto plugins = m_pluginManager.preferredWritePluginsFor(mime);
        for (const auto plugin : plugins) {
            QTest::newRow(QStringLiteral("%1 (%2, %3)").arg(testName, format, plugin->metaData().pluginId()).toUtf8().constData())
                << filename
                << plugin
                << targetEntries
                << destination
                << expectedNewPaths
                << numberOfEntries;
        }
    }
}
