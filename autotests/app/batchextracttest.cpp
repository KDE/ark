/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "batchextract.h"

#include <QTest>

class BatchExtractTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testBatchExtraction_data();
    void testBatchExtraction();

private:
    QString m_expectedWorkingDir;
};

QTEST_MAIN(BatchExtractTest)

void BatchExtractTest::initTestCase()
{
    // #395939: after each extraction, the cwd must be the one we started from.
    m_expectedWorkingDir = QDir::currentPath();
}

void BatchExtractTest::testBatchExtraction_data()
{
    QTest::addColumn<QString>("archivePath");
    QTest::addColumn<bool>("autoSubfolder");
    // Expected numbers of entries (files + folders) in the temporary extraction folder.
    // This is the number of entries in the archive (+ 1, if the autosubfolder is expected).
    QTest::addColumn<int>("expectedExtractedEntriesCount");
    QTest::addColumn<bool>("createExistingDir");
    QTest::addColumn<int>("expectedTopLevelEntriesCount");

    QTest::newRow("extract the whole simple%archive.tar.gz (bug #365798), empty destination")
        << QFINDTESTDATA("data/simple%archive.tar.gz") << true << 5 << false << 1;

    QTest::newRow("single-folder, no autosubfolder, empty destination")
        << QFINDTESTDATA("../kerfuffle/data/one_toplevel_folder.zip") << false << 9 << false << 1;

    QTest::newRow("single-folder, autosubfolder, existing folder in destination")
        << QFINDTESTDATA("../kerfuffle/data/one_toplevel_folder.zip") << true << 11 << true << 2;

    QTest::newRow("single-folder, autosubfolder, empty destination") << QFINDTESTDATA("../kerfuffle/data/one_toplevel_folder.zip") << true << 9 << false << 1;

    QTest::newRow("non single-folder, no autosubfolder, empty destination")
        << QFINDTESTDATA("../kerfuffle/data/simplearchive.tar.gz") << false << 4 << false << 3;

    QTest::newRow("non single-folder, autosubfolder, empty destination") << QFINDTESTDATA("../kerfuffle/data/simplearchive.tar.gz") << true << 5 << false << 1;

    QTest::newRow("single-file, no autosubfolder, empty destination") << QFINDTESTDATA("data/test.txt.gz") << false << 1 << false << 1;

    QTest::newRow("single-file, autosubfolder, empty destination") << QFINDTESTDATA("data/test.txt.gz") << true << 1 << false << 1;
}

void BatchExtractTest::testBatchExtraction()
{
    auto batchJob = new BatchExtract(this);

    QFETCH(QString, archivePath);
    batchJob->addInput(QUrl::fromUserInput(archivePath));

    QFETCH(bool, autoSubfolder);
    batchJob->setAutoSubfolder(autoSubfolder);

    QTemporaryDir destDir;
    if (!destDir.isValid()) {
        QSKIP("Could not create a temporary directory for extraction. Skipping test.", SkipSingle);
    }

    batchJob->setDestinationFolder(destDir.path());

    QFETCH(bool, createExistingDir);
    if (createExistingDir) {
        if (!QDir(destDir.path()).mkdir(QStringLiteral("A"))) {
            QSKIP("Could not create a temporary directory for existing directory use-case. Skipping test.", SkipSingle);
        }
    }

    QEventLoop eventLoop(this);
    connect(batchJob, &KJob::result, &eventLoop, &QEventLoop::quit);
    batchJob->start();
    eventLoop.exec(); // krazy:exclude=crashy

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    for (const auto _ : QDirListing(destDir.path(), QDirListing::IteratorFlag::Recursive | QDirListing::IteratorFlag::IncludeHidden)) {
        extractedEntriesCount++;
    }

    QFETCH(int, expectedTopLevelEntriesCount);
    int extractedTopLevelEntriesCount = 0;

    for (const auto _ : QDirListing(destDir.path(), QDirListing::IteratorFlag::IncludeHidden)) {
        extractedTopLevelEntriesCount++;
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);
    QCOMPARE(extractedTopLevelEntriesCount, expectedTopLevelEntriesCount);
    QCOMPARE(QDir::currentPath(), m_expectedWorkingDir);
}

#include "batchextracttest.moc"
