//
// Created by mvlabat on 7/21/16.
//

#include "testhelper.h"

QEventLoop TestHelper::m_eventLoop;

void TestHelper::startAndWaitForResult(KJob *job)
{
    QObject::connect(job, &KJob::result, &m_eventLoop, &QEventLoop::quit);
    job->start();
    m_eventLoop.exec();
}

QList<Archive::Entry*> TestHelper::getEntryList(ReadOnlyArchiveInterface *iface)
{
    QList<Archive::Entry*> list = QList<Archive::Entry*>();
    ListJob *listJob = new ListJob(iface);
    QObject::connect(listJob, &Job::newEntry, [&list](Archive::Entry* entry) { list << entry; });
    startAndWaitForResult(listJob);
    return list;
}

QStringList TestHelper::getExpectedEntryPaths(const QList<Archive::Entry*> &entryList, const Archive::Entry *destination)
{
    QStringList expectedPaths = QStringList();
    if (entryList.count() > 1) {
        foreach (const Archive::Entry *entry, entryList) {
            expectedPaths << destination->property("fullPath").toString() + entry->property("fullPath").toString();
        }
    }
    else {
        expectedPaths << destination->property("fullPath").toString();
    }
    return expectedPaths;
}

void TestHelper::verifyAddedEntriesWithDestination(const QList<Archive::Entry *> &argumentEntries,
                                                   const Archive::Entry *destination,
                                                   const QList<Archive::Entry *> &newEntries)
{
    QStringList expectedPaths = getExpectedEntryPaths(argumentEntries, destination);
    QStringList actualPaths = ReadOnlyArchiveInterface::entryFullPaths(newEntries);
    foreach (const QString &path, expectedPaths) {
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive")).toStdString().c_str());
    }
}

void TestHelper::verifyMovedEntriesWithDestination(const QList<Archive::Entry *> &argumentEntries,
                                                   const Archive::Entry *destination,
                                                   const QList<Archive::Entry *> &newEntries)
{
    QStringList oldPaths = ReadOnlyArchiveInterface::entryFullPaths(argumentEntries);
    QStringList expectedPaths = getExpectedEntryPaths(argumentEntries, destination);
    QStringList actualPaths = ReadOnlyArchiveInterface::entryFullPaths(newEntries);
    foreach (const QString &path, expectedPaths) {
        QVERIFY2(actualPaths.contains(path), (QStringLiteral("No ") + path + QStringLiteral(" inside the archive")).toStdString().c_str());
        QVERIFY2(!oldPaths.contains(path), (QStringLiteral("Entry ") + path + QStringLiteral(" is still inside the archive, when it shouldn't be")).toStdString().c_str());
    }
}
