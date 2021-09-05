/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "batchextract.h"

#include <QDirIterator>
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

    QTest::newRow("extract the whole simple%archive.tar.gz (bug #365798)")
            << QFINDTESTDATA("data/simple%archive.tar.gz")
            << true
            << 5;

    QTest::newRow("single-folder, no autosubfolder")
            << QFINDTESTDATA("../kerfuffle/data/one_toplevel_folder.zip")
            << false
            << 9;

    QTest::newRow("single-folder, autosubfolder")
            << QFINDTESTDATA("../kerfuffle/data/one_toplevel_folder.zip")
            << true
            << 9;

    QTest::newRow("non single-folder, no autosubfolder")
            << QFINDTESTDATA("../kerfuffle/data/simplearchive.tar.gz")
            << false
            << 4;

    QTest::newRow("non single-folder, autosubfolder")
            << QFINDTESTDATA("../kerfuffle/data/simplearchive.tar.gz")
            << true
            << 5;

    QTest::newRow("single-file, no autosubfolder")
            << QFINDTESTDATA("data/test.txt.gz")
            << false
            << 1;

    QTest::newRow("single-file, autosubfolder")
            << QFINDTESTDATA("data/test.txt.gz")
            << true
            << 1;
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

    QEventLoop eventLoop(this);
    connect(batchJob, &KJob::result, &eventLoop, &QEventLoop::quit);
    batchJob->start();
    eventLoop.exec(); // krazy:exclude=crashy

    QFETCH(int, expectedExtractedEntriesCount);
    int extractedEntriesCount = 0;

    QDirIterator dirIt(destDir.path(), QDir::AllEntries | QDir::Hidden | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    while (dirIt.hasNext()) {
        extractedEntriesCount++;
        dirIt.next();
    }

    QCOMPARE(extractedEntriesCount, expectedExtractedEntriesCount);
    QCOMPARE(QDir::currentPath(), m_expectedWorkingDir);
}

#include "batchextracttest.moc"
