#include "testhelper.h"

QEventLoop TestHelper::m_eventLoop;

void TestHelper::startAndWaitForResult(KJob *job)
{
    QObject::connect(job, &KJob::result, &m_eventLoop, &QEventLoop::quit);
    job->start();
    m_eventLoop.exec();
}

QList<Archive::Entry*> TestHelper::getEntryList(Archive *archive)
{
    QList<Archive::Entry*> list = QList<Archive::Entry*>();
    ListJob *listJob = archive->list();
    QObject::connect(listJob, &Job::newEntry, [&list](Archive::Entry* entry) { list << entry; });
    startAndWaitForResult(listJob);
    return list;
}

void TestHelper::verifyAddedEntriesWithDestination(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &oldEntries, const QList<Archive::Entry*> &newEntries)
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

void TestHelper::verifyMovedEntriesWithDestination(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &oldEntries, const QList<Archive::Entry*> &newEntries)
{
    QStringList expectedPaths = getExpectedMovedEntryPaths(oldEntries, argumentEntries, destination);
    QStringList actualPaths = ReadOnlyArchiveInterface::entryFullPaths(newEntries);
    foreach (const QString &path, expectedPaths) {
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive")).toUtf8());
    }
    foreach (const QString &path, actualPaths) {
        QVERIFY2(expectedPaths.contains(path), (QStringLiteral("Entry ") + path + QStringLiteral(" is not expected to be inside the archive")).toUtf8());
    }
    foreach (const Archive::Entry *entry, argumentEntries) {
        const QString path = entry->fullPath();
        QVERIFY2(!actualPaths.contains(path), (QStringLiteral("Entry ") + path + QStringLiteral(" is still inside the archive, when it shouldn't be")).toUtf8());
    }
}

void TestHelper::verifyCopiedEntriesWithDestination(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination, const QList<Archive::Entry*> &oldEntries, const QList<Archive::Entry*> &newEntries)
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

QStringList TestHelper::getExpectedNewEntryPaths(const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination)
{
    QStringList expectedPaths = QStringList();
    const QString testDataPath = QFINDTESTDATA("data") + QLatin1Char('/');

    foreach (const Archive::Entry *entry, argumentEntries) {
        const QString entryPath = entry->fullPath();
        expectedPaths << destination->fullPath() + entryPath;

        if (entryPath.right(1) == QLatin1String("/")) {
            const QString workingDirectory = testDataPath + QLatin1Char('/') + entry->fullPath(true);
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

QStringList TestHelper::getExpectedMovedEntryPaths(const QList<Archive::Entry*> &entryList, const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination)
{
    QStringList expectedPaths = QStringList();
    QMap<QString, Archive::Entry*> entryMap = getEntryMap(entryList);
    QStringList argumentPaths = ReadOnlyArchiveInterface::entryFullPaths(argumentEntries);
    QString lastMovedFolder;
    if (ReadOnlyArchiveInterface::entriesWithoutChildren(argumentEntries).count() > 1) {
        // Destination path doesn't contain a target entry name, so we have to remember to include it while moving
        // folder contents.
        int nameLength = 0;
        foreach (const Archive::Entry *entry, entryMap) {
            const QString entryPath = entry->fullPath();
            if (lastMovedFolder.count() > 0 && entryPath.startsWith(lastMovedFolder)) {
                expectedPaths << destination->fullPath() + entryPath.right(entryPath.count() - lastMovedFolder.count() + nameLength);
            }
            else if (argumentPaths.contains(entryPath)) {
                QString expectedPath = destination->fullPath() + entry->name();
                if (entryPath.right(1) == QLatin1String("/")) {
                    expectedPath += QLatin1Char('/');
                    nameLength = entry->name().count() + 1; // plus slash
                    lastMovedFolder = entryPath;
                }
                else {
                    nameLength = 0;
                    lastMovedFolder = QString();
                }
                expectedPaths << expectedPath;
            }
            else {
                expectedPaths << entryPath;
                nameLength = 0;
                lastMovedFolder = QString();
            }
        }
    }
    else {
        foreach (const Archive::Entry *entry, entryMap) {
            const QString entryPath = entry->fullPath();
            if (lastMovedFolder.count() > 0 && entryPath.startsWith(lastMovedFolder)) {
                expectedPaths << destination->fullPath() + entryPath.right(entryPath.count() - lastMovedFolder.count());
            }
            else if (argumentPaths.contains(entryPath)) {
                if (entryPath.right(1) == QLatin1String("/")) {
                    lastMovedFolder = entryPath;
                }
                else if (lastMovedFolder.count() > 0) {
                    lastMovedFolder = QString();
                }
                expectedPaths << destination->fullPath();
            }
            else {
                expectedPaths << entryPath;
            }
        }
    }
    return expectedPaths;
}

QStringList TestHelper::getExpectedCopiedEntryPaths(const QList<Archive::Entry*> &entryList, const QList<Archive::Entry*> &argumentEntries, const Archive::Entry *destination)
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
        }
        else if (argumentPaths.contains(entryPath)) {
            QString expectedPath = destination->fullPath() + entry->name();
            if (entryPath.right(1) == QLatin1String("/")) {
                expectedPath += QLatin1Char('/');
                nameLength = entry->name().count() + 1; // plus slash
                lastCopiedFolder = entryPath;
            }
            else {
                nameLength = 0;
                lastCopiedFolder = QString();
            }
            expectedPaths << expectedPath;
        }
        else {
            nameLength = 0;
            lastCopiedFolder = QString();
        }
        expectedPaths << entryPath;
    }
    return expectedPaths;
}

QMap<QString, Archive::Entry*> TestHelper::getEntryMap(const QList<Archive::Entry *> entries)
{
    QMap<QString, Archive::Entry*> map;
    foreach (Archive::Entry* entry, entries) {
        map.insert(entry->fullPath(), entry);
    }
    return map;
}
