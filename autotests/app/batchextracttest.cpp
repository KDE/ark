/*
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
            << 2;
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
