/*
 * Copyright (c) 2016 Vladyslav Batyrenko <mvlabat@gmail.com>
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

#include "testhelper.h"

void TestHelper::startAndWaitForResult(KJob *job)
{
    QEventLoop eventLoop;
    QObject::connect(job, &KJob::result, &eventLoop, &QEventLoop::quit);
    job->start();
    eventLoop.exec(); // krazy:exclude=crashy
}

QVector<Archive::Entry*> TestHelper::getEntryList(Archive *archive)
{
    QVector<Archive::Entry*> list = QVector<Archive::Entry*>();
    auto loadJob = Archive::load(archive->fileName());
    QObject::connect(loadJob, &Job::newEntry, [&list](Archive::Entry* entry) { list << entry; });
    startAndWaitForResult(loadJob);

    return list;
}

void TestHelper::verifyAddedEntriesWithDestination(const QVector<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QVector<Archive::Entry*> &oldEntries, const QVector<Archive::Entry*> &newEntries)
{
    QStringList expectedPaths = getExpectedNewEntryPaths(argumentEntries, destination);
    QStringList actualPaths = ReadOnlyArchiveInterface::entryFullPaths(newEntries);
    foreach (const QString &path, expectedPaths) {
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive (new entry)")).toUtf8());
    }
    foreach (const Archive::Entry *entry, oldEntries) {
        const QString path = entry->fullPath();
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive (old entry)")).toUtf8());
    }
}

void TestHelper::verifyCopiedEntriesWithDestination(const QVector<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QVector<Archive::Entry*> &oldEntries, const QVector<Archive::Entry*> &newEntries)
{
    QStringList expectedPaths = getExpectedCopiedEntryPaths(oldEntries, argumentEntries, destination);
    QStringList actualPaths = ReadOnlyArchiveInterface::entryFullPaths(newEntries);
    foreach (const QString &path, expectedPaths) {
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive")).toUtf8());
    }
    foreach (const QString &path, actualPaths) {
        QVERIFY2(expectedPaths.contains(path), (QStringLiteral("Entry ") + path + QStringLiteral(" is not expected to be inside the archive")).toUtf8());
    }
}

QStringList TestHelper::getExpectedNewEntryPaths(const QVector<Archive::Entry*> &argumentEntries, const Archive::Entry *destination)
{
    QStringList expectedPaths = QStringList();
    const QString testDataPath = QFINDTESTDATA("data") + QLatin1Char('/');

    foreach (const Archive::Entry *entry, argumentEntries) {
        const QString entryPath = entry->fullPath();
        expectedPaths << destination->fullPath() + entryPath;

        if (entryPath.right(1) == QLatin1String("/")) {
            const QString workingDirectory = testDataPath + QLatin1Char('/') + entry->fullPath(NoTrailingSlash);
            QDirIterator it(workingDirectory, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
            while (it.hasNext()) {
                QString path = it.next();
                path = destination->fullPath() + path.right(path.count() - testDataPath.count() - 1);
                if (it.fileInfo().isDir()) {
                    path += QLatin1Char('/');
                }
                expectedPaths << path;
            }
        }
    }
    return expectedPaths;
}

QStringList TestHelper::getExpectedCopiedEntryPaths(const QVector<Archive::Entry*> &entryList, const QVector<Archive::Entry*> &argumentEntries, const Archive::Entry *destination)
{
    QStringList expectedPaths = QStringList();
    QMap<QString, Archive::Entry*> entryMap = getEntryMap(entryList);
    QStringList argumentPaths = ReadOnlyArchiveInterface::entryFullPaths(argumentEntries);
    QString lastCopiedFolder;
    // Destination path doesn't contain a target entry name, so we have to remember to include it while copying
    // folder contents.
    int nameLength = 0;
    foreach (const Archive::Entry *entry, entryMap) {
        const QString entryPath = entry->fullPath();
        if (lastCopiedFolder.count() > 0 && entryPath.startsWith(lastCopiedFolder)) {
            expectedPaths << destination->fullPath() + entryPath.right(entryPath.count() - lastCopiedFolder.count() + nameLength);
        } else if (argumentPaths.contains(entryPath)) {
            QString expectedPath = destination->fullPath() + entry->name();
            if (entryPath.right(1) == QLatin1String("/")) {
                expectedPath += QLatin1Char('/');
                nameLength = entry->name().count() + 1; // plus slash
                lastCopiedFolder = entryPath;
            } else {
                nameLength = 0;
                lastCopiedFolder = QString();
            }
            expectedPaths << expectedPath;
        } else {
            nameLength = 0;
            lastCopiedFolder = QString();
        }
        expectedPaths << entryPath;
    }
    return expectedPaths;
}

QMap<QString, Archive::Entry*> TestHelper::getEntryMap(const QVector<Archive::Entry*> &entries)
{
    QMap<QString, Archive::Entry*> map;
    foreach (Archive::Entry* entry, entries) {
        map.insert(entry->fullPath(), entry);
    }
    return map;
}
