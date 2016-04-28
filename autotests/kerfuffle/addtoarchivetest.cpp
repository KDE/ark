/*
 * Copyright (c) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
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

#include "kerfuffle/addtoarchive.h"
#include "kerfuffle/archive_kerfuffle.h"

#include <QEventLoop>
#include <QStandardPaths>
#include <QTest>

using namespace Kerfuffle;

class AddToArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void testCompressHere_data();
    void testCompressHere();
};

void AddToArchiveTest::testCompressHere_data()
{
    QTest::addColumn<QString>("expectedSuffix");
    QTest::addColumn<QStringList>("inputFiles");
    QTest::addColumn<QString>("expectedArchiveName");
    QTest::addColumn<qulonglong>("expectedNumberOfFiles");

    QTest::newRow("compress here (as TAR) - whole folder")
        << QStringLiteral("tar.gz")
        << QStringList {QFINDTESTDATA("data/testdir")}
        << QStringLiteral("testdir.tar.gz")
        << 2ULL;

    QTest::newRow("compress here (as TAR) - single file")
        << QStringLiteral("tar.gz")
        << QStringList {QFINDTESTDATA("data/testfile.txt")}
        << QStringLiteral("testfile.tar.gz")
        << 1ULL;

    QTest::newRow("compress here (as TAR) - file + folder")
        << QStringLiteral("tar.gz")
        << QStringList {
               QFINDTESTDATA("data/testdir"),
               QFINDTESTDATA("data/testfile.txt")
           }
        << QStringLiteral("data.tar.gz")
        << 3ULL;

    if (!QStandardPaths::findExecutable(QStringLiteral("zip")).isEmpty()) {
        QTest::newRow("compress here (as ZIP) - whole folder")
            << QStringLiteral("zip")
            << QStringList {QFINDTESTDATA("data/testdir")}
            << QStringLiteral("testdir.zip")
            << 2ULL;

        QTest::newRow("compress here (as ZIP) - single file")
            << QStringLiteral("zip")
            << QStringList {QFINDTESTDATA("data/testfile.txt")}
            << QStringLiteral("testfile.zip")
            << 1ULL;

        QTest::newRow("compress here (as ZIP) - file + folder")
            << QStringLiteral("zip")
            << QStringList {
                   QFINDTESTDATA("data/testdir"),
                   QFINDTESTDATA("data/testfile.txt")
               }
            << QStringLiteral("data.zip")
            << 3ULL;
    } else {
        qDebug() << "zip executable not found in path. Skipping compress-here-(ZIP) tests.";
    }
}

void AddToArchiveTest::testCompressHere()
{
    AddToArchive *addToArchiveJob = new AddToArchive(this);
    addToArchiveJob->setChangeToFirstPath(true);

    QFETCH(QString, expectedSuffix);
    addToArchiveJob->setAutoFilenameSuffix(expectedSuffix);

    QFETCH(QStringList, inputFiles);
    foreach (const QString &file, inputFiles) {
        addToArchiveJob->addInput(QUrl::fromUserInput(file));
    }

    // Run the job in the following event loop.
    QEventLoop eventLoop(this);
    connect(addToArchiveJob, &KJob::result, &eventLoop, &QEventLoop::quit);
    addToArchiveJob->start();
    eventLoop.exec(); // krazy:exclude=crashy

    // Check the properties of the generated test archive, then remove it.
    QFETCH(QString, expectedArchiveName);
    Archive *archive = Archive::create(QFINDTESTDATA(QStringLiteral("data/%1").arg(expectedArchiveName)));

    QVERIFY(archive);
    QVERIFY(archive->isValid());
    QCOMPARE(archive->completeBaseName() + QLatin1Char('.') + expectedSuffix, expectedArchiveName);

    QFETCH(qulonglong, expectedNumberOfFiles);
    QCOMPARE(archive->numberOfFiles(), expectedNumberOfFiles);

    QVERIFY(QFile(archive->fileName()).remove());
}

QTEST_MAIN(AddToArchiveTest)

#include "addtoarchivetest.moc"
