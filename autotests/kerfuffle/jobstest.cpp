/*
    SPDX-FileCopyrightText: 2010-2011 Raphael Kubo da Costa <rakuco@FreeBSD.org>
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "jobs.h"
#include "jsonarchiveinterface.h"

#include <KPluginMetaData>

#include <QDebug>
#include <QEventLoop>
#include <QTest>

using namespace Kerfuffle;

class JobsTest : public QObject
{
    Q_OBJECT

public:
    JobsTest();

protected Q_SLOTS:
    void init();
    void slotNewEntry(Archive::Entry *entry);

private Q_SLOTS:
    // ListJob-related tests
    void testLoadJob_data();
    void testLoadJob();

    // ExtractJob-related tests
    void testExtractJobAccessors();
    void testTempExtractJob();

    // DeleteJob-related tests
    void testRemoveEntries_data();
    void testRemoveEntries();

    // AddJob-related tests
    void testAddEntries_data();
    void testAddEntries();

private:
    JSONArchiveInterface *createArchiveInterface(const QString &filePath);
    QVector<Archive::Entry *> listEntries(JSONArchiveInterface *iface);
    void startAndWaitForResult(KJob *job);

    QVector<Archive::Entry *> m_entries;
    QEventLoop m_eventLoop;
};

QTEST_GUILESS_MAIN(JobsTest)

JobsTest::JobsTest()
    : QObject(nullptr)
    , m_eventLoop(this)
{
}

void JobsTest::init()
{
    m_entries.clear();
}

void JobsTest::slotNewEntry(Archive::Entry *entry)
{
    m_entries.append(entry);
}

JSONArchiveInterface *JobsTest::createArchiveInterface(const QString &filePath)
{
    JSONArchiveInterface *iface = new JSONArchiveInterface(this, {filePath, QVariant().fromValue(KPluginMetaData())});
    if (!iface->open()) {
        qDebug() << "Could not open" << filePath;
        return nullptr;
    }

    return iface;
}

QVector<Archive::Entry *> JobsTest::listEntries(JSONArchiveInterface *iface)
{
    m_entries.clear();

    auto job = new LoadJob(iface);
    connect(job, &Job::newEntry, this, &JobsTest::slotNewEntry);

    startAndWaitForResult(job);

    return m_entries;
}

void JobsTest::startAndWaitForResult(KJob *job)
{
    connect(job, &KJob::result, &m_eventLoop, &QEventLoop::quit);
    job->start();
    m_eventLoop.exec();
}

void JobsTest::testLoadJob_data()
{
    QTest::addColumn<QString>("jsonArchive");
    QTest::addColumn<qlonglong>("expectedExtractedFilesSize");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<bool>("isSingleFolder");
    QTest::addColumn<QStringList>("expectedEntryNames");

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json") << 0LL << false << false
                                     << QStringList{QStringLiteral("a.txt"), QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt"), QStringLiteral("c.txt")};

    QTest::newRow("archive002.json") << QFINDTESTDATA("data/archive002.json") << 45959LL << false << false
                                     << QStringList{QStringLiteral("a.txt"), QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt"), QStringLiteral("c.txt")};

    QTest::newRow("archive-deepsinglehierarchy.json") << QFINDTESTDATA("data/archive-deepsinglehierarchy.json") << 0LL << false << true
                                                      << QStringList{// Depth-first order!
                                                                     QStringLiteral("aDir/"),
                                                                     QStringLiteral("aDir/aDirInside/"),
                                                                     QStringLiteral("aDir/aDirInside/anotherDir/"),
                                                                     QStringLiteral("aDir/aDirInside/anotherDir/file.txt"),
                                                                     QStringLiteral("aDir/b.txt")};

    QTest::newRow("archive-multiplefolders.json") << QFINDTESTDATA("data/archive-multiplefolders.json") << 0LL << false << false
                                                  << QStringList{QStringLiteral("aDir/"),
                                                                 QStringLiteral("aDir/b.txt"),
                                                                 QStringLiteral("anotherDir/"),
                                                                 QStringLiteral("anotherDir/file.txt")};

    QTest::newRow("archive-nodir-manyfiles.json") << QFINDTESTDATA("data/archive-nodir-manyfiles.json") << 0LL << false << false
                                                  << QStringList{QStringLiteral("a.txt"), QStringLiteral("file.txt")};

    QTest::newRow("archive-onetopfolder.json") << QFINDTESTDATA("data/archive-onetopfolder.json") << 0LL << false << true
                                               << QStringList{QStringLiteral("aDir/"), QStringLiteral("aDir/b.txt")};

    QTest::newRow("archive-password.json") << QFINDTESTDATA("data/archive-password.json") << 0LL << true
                                           << false
                                           // Possibly unexpected behavior of listing:
                                           // 1. Directories are listed before files, if they are empty!
                                           // 2. Files are sorted alphabetically.
                                           << QStringList{QStringLiteral("aDirectory/"), QStringLiteral("bar.txt"), QStringLiteral("foo.txt")};

    QTest::newRow("archive-singlefile.json") << QFINDTESTDATA("data/archive-singlefile.json") << 0LL << false << false << QStringList{QStringLiteral("a.txt")};

    QTest::newRow("archive-emptysinglefolder.json") << QFINDTESTDATA("data/archive-emptysinglefolder.json") << 0LL << false << true
                                                    << QStringList{QStringLiteral("aDir/")};

    QTest::newRow("archive-unorderedsinglefolder.json")
        << QFINDTESTDATA("data/archive-unorderedsinglefolder.json") << 0LL << false << true
        << QStringList{QStringLiteral("aDir/"), QStringLiteral("aDir/anotherDir/"), QStringLiteral("aDir/anotherDir/bar.txt"), QStringLiteral("aDir/foo.txt")};
}

void JobsTest::testLoadJob()
{
    QFETCH(QString, jsonArchive);
    JSONArchiveInterface *iface = createArchiveInterface(jsonArchive);
    QVERIFY(iface);

    auto loadJob = new LoadJob(iface);
    loadJob->setAutoDelete(false);
    startAndWaitForResult(loadJob);

    QFETCH(qlonglong, expectedExtractedFilesSize);
    QCOMPARE(loadJob->extractedFilesSize(), expectedExtractedFilesSize);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(loadJob->isPasswordProtected(), isPasswordProtected);

    QFETCH(bool, isSingleFolder);
    QCOMPARE(loadJob->isSingleFolderArchive(), isSingleFolder);

    QFETCH(QStringList, expectedEntryNames);
    auto archiveEntries = listEntries(iface);

    QCOMPARE(archiveEntries.size(), expectedEntryNames.size());

    for (int i = 0; i < archiveEntries.size(); i++) {
        QCOMPARE(archiveEntries.at(i)->fullPath(), expectedEntryNames.at(i));
    }

    loadJob->deleteLater();
}

void JobsTest::testExtractJobAccessors()
{
    JSONArchiveInterface *iface = createArchiveInterface(QFINDTESTDATA("data/archive001.json"));
    ExtractJob *job = new ExtractJob(QVector<Archive::Entry *>(), QStringLiteral("/tmp/some-dir"), ExtractionOptions(), iface);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/tmp/some-dir"));
    QVERIFY(job->extractionOptions().preservePaths());

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/tmp/some-dir"));
    delete job;

    ExtractionOptions options;
    options.setPreservePaths(false);

    job = new ExtractJob(QVector<Archive::Entry *>(), QStringLiteral("/root"), options, iface);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/root"));
    QVERIFY(!job->extractionOptions().preservePaths());

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QCOMPARE(job->destinationDirectory(), QLatin1String("/root"));
    QVERIFY(!job->extractionOptions().preservePaths());
    delete job;
}

void JobsTest::testTempExtractJob()
{
    JSONArchiveInterface *iface = createArchiveInterface(QFINDTESTDATA("data/archive-malicious.json"));
    PreviewJob *job = new PreviewJob(new Archive::Entry(this, QStringLiteral("anotherDir/../../file.txt")), false, iface);

    const QString tempDirPath = job->tempDir()->path();
    QVERIFY(QFileInfo::exists(tempDirPath));
    QVERIFY(job->validatedFilePath().endsWith(QLatin1String("anotherDir/file.txt")));
    QVERIFY(job->extractionOptions().preservePaths());

    job->setAutoDelete(false);
    startAndWaitForResult(job);

    QVERIFY(job->validatedFilePath().endsWith(QLatin1String("anotherDir/file.txt")));
    QVERIFY(job->extractionOptions().preservePaths());

    delete job->tempDir();
    QVERIFY(!QFileInfo::exists(tempDirPath));

    delete job;
}

void JobsTest::testRemoveEntries_data()
{
    QTest::addColumn<QString>("jsonArchive");
    QTest::addColumn<QVector<Archive::Entry *>>("entries");
    QTest::addColumn<QVector<Archive::Entry *>>("entriesToDelete");

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("c.txt"))};

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))};

    // Error test: if we delete non-existent entries, the archive must not change.
    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("foo.txt"))};
}

void JobsTest::testRemoveEntries()
{
    QFETCH(QString, jsonArchive);
    JSONArchiveInterface *iface = createArchiveInterface(jsonArchive);
    QVERIFY(iface);

    QFETCH(QVector<Archive::Entry *>, entries);
    QFETCH(QVector<Archive::Entry *>, entriesToDelete);
    QStringList fullPathsToDelete = iface->entryFullPaths(entriesToDelete);

    QVector<Archive::Entry *> expectedRemainingEntries;
    for (Archive::Entry *entry : std::as_const(entries)) {
        if (!fullPathsToDelete.contains(entry->fullPath())) {
            expectedRemainingEntries.append(entry);
        }
    }

    DeleteJob *deleteJob = new DeleteJob(entriesToDelete, iface);
    startAndWaitForResult(deleteJob);

    auto remainingEntries = listEntries(iface);
    QCOMPARE(remainingEntries.size(), expectedRemainingEntries.size());

    for (int i = 0; i < remainingEntries.size(); i++) {
        QCOMPARE(*remainingEntries.at(i), *expectedRemainingEntries.at(i));
    }

    iface->deleteLater();
}

void JobsTest::testAddEntries_data()
{
    QTest::addColumn<QString>("jsonArchive");
    QTest::addColumn<QVector<Archive::Entry *>>("originalEntries");
    QTest::addColumn<QVector<Archive::Entry *>>("entriesToAdd");
    QTest::addColumn<Archive::Entry *>("destinationEntry");

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("foo.txt"))} << new Archive::Entry(this);

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("foo.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("bar.txt"))}
                                     << new Archive::Entry(this);

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("foo.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("bar.txt"))}
                                     << new Archive::Entry(this, QStringLiteral("aDir/"));

    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << new Archive::Entry(this, QStringLiteral("aDir/"));

    // Error test: if we add an already existent entry, the archive must not change.
    QTest::newRow("archive001.json") << QFINDTESTDATA("data/archive001.json")
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("a.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/")),
                                                                  new Archive::Entry(this, QStringLiteral("aDir/b.txt")),
                                                                  new Archive::Entry(this, QStringLiteral("c.txt"))}
                                     << QVector<Archive::Entry *>{new Archive::Entry(this, QStringLiteral("c.txt"))} << new Archive::Entry(this);
}

void JobsTest::testAddEntries()
{
    QFETCH(QString, jsonArchive);
    JSONArchiveInterface *iface = createArchiveInterface(jsonArchive);
    QVERIFY(iface);

    QFETCH(QVector<Archive::Entry *>, originalEntries);
    QStringList originalFullPaths = QStringList();
    for (const Archive::Entry *entry : std::as_const(originalEntries)) {
        originalFullPaths.append(entry->fullPath());
    }
    auto currentEntries = listEntries(iface);
    QCOMPARE(currentEntries.size(), originalEntries.size());

    QFETCH(QVector<Archive::Entry *>, entriesToAdd);
    QFETCH(Archive::Entry *, destinationEntry);
    AddJob *addJob = new AddJob(entriesToAdd, destinationEntry, CompressionOptions(), iface);
    startAndWaitForResult(addJob);

    QStringList expectedAddedFullPaths = QStringList();
    const QString destinationPath = destinationEntry->fullPath();
    int expectedEntriesCount = originalEntries.size();
    for (const Archive::Entry *entry : std::as_const(entriesToAdd)) {
        const QString fullPath = destinationPath + entry->fullPath();
        if (!originalFullPaths.contains(fullPath)) {
            expectedEntriesCount++;
            expectedAddedFullPaths << destinationPath + entry->fullPath();
        }
    }

    currentEntries = listEntries(iface);
    QCOMPARE(currentEntries.size(), expectedEntriesCount);

    QStringList currentFullPaths = QStringList();
    for (const Archive::Entry *entry : std::as_const(currentEntries)) {
        currentFullPaths << entry->fullPath();
    }

    for (const QString &fullPath : std::as_const(expectedAddedFullPaths)) {
        QVERIFY(currentFullPaths.contains(fullPath));
    }

    iface->deleteLater();
}

#include "jobstest.moc"
