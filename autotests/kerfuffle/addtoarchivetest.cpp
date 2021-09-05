/*
    SPDX-FileCopyrightText: 2016 Elvis Angelaccio <elvis.angelaccio@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "addtoarchive.h"
#include "jobs.h"
#include "pluginmanager.h"
#include "testhelper.h"

#include <QMimeDatabase>
#include <QStandardPaths>
#include <QTest>

using namespace Kerfuffle;

class AddToArchiveTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void init();
    void testCompressHere_data();
    void testCompressHere();
};

void AddToArchiveTest::init()
{
    // The test needs an empty subfolder, but git doesn't support tracking of empty directories.
    QDir(QFINDTESTDATA("data/testdirwithemptysubdir")).mkdir(QStringLiteral("emptydir"));
}

void AddToArchiveTest::testCompressHere_data()
{
    QTest::addColumn<QString>("expectedSuffix");
    QTest::addColumn<Archive::EncryptionType>("expectedEncryptionType");
    QTest::addColumn<QStringList>("inputFiles");
    QTest::addColumn<QString>("expectedArchiveName");
    QTest::addColumn<qulonglong>("expectedNumberOfEntries");

    QTest::newRow("compress here (as TAR) - dir with files")
        << QStringLiteral("tar.gz")
        << Archive::Unencrypted
        << QStringList {QFINDTESTDATA("data/testdir")}
        << QStringLiteral("testdir.tar.gz")
        << 3ULL;

    QTest::newRow("compress here (as TAR) - dir with subdirs")
        << QStringLiteral("tar.gz")
        << Archive::Unencrypted
        << QStringList {QFINDTESTDATA("data/testdirwithsubdirs")}
        << QStringLiteral("testdirwithsubdirs.tar.gz")
        << 8ULL;

    QTest::newRow("compress here (as TAR) - dir with empty subdir")
        << QStringLiteral("tar.gz")
        << Archive::Unencrypted
        << QStringList {QFINDTESTDATA("data/testdirwithemptysubdir")}
        << QStringLiteral("testdirwithemptysubdir.tar.gz")
        << 4ULL;

    QTest::newRow("compress here (as TAR) - single file")
        << QStringLiteral("tar.gz")
        << Archive::Unencrypted
        << QStringList {QFINDTESTDATA("data/testfile.txt")}
        << QStringLiteral("testfile.tar.gz")
        << 1ULL;

    QTest::newRow("compress here (as TAR) - file + folder")
        << QStringLiteral("tar.gz")
        << Archive::Unencrypted
        << QStringList {
               QFINDTESTDATA("data/testdir"),
               QFINDTESTDATA("data/testfile.txt")
           }
        << QStringLiteral("data.tar.gz")
        << 4ULL;

    QTest::newRow("compress here (as TAR) - bug #362690")
        << QStringLiteral("tar.gz")
        << Archive::Unencrypted
        << QStringList {QFINDTESTDATA("data/test-3.4.0")}
        << QStringLiteral("test-3.4.0.tar.gz")
        << 2ULL;

    if (!PluginManager().preferredWritePluginsFor(QMimeDatabase().mimeTypeForName(QStringLiteral("application/zip"))).isEmpty()) {
        QTest::newRow("compress here (as ZIP) - dir with files")
            << QStringLiteral("zip")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testdir")}
            << QStringLiteral("testdir.zip")
            << 3ULL;

        QTest::newRow("compress here (as ZIP) - dir with subdirs")
            << QStringLiteral("zip")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testdirwithsubdirs")}
            << QStringLiteral("testdirwithsubdirs.zip")
            << 8ULL;

        QTest::newRow("compress here (as ZIP) - dir with empty subdir")
            << QStringLiteral("zip")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testdirwithemptysubdir")}
            << QStringLiteral("testdirwithemptysubdir.zip")
            << 4ULL;

        QTest::newRow("compress here (as ZIP) - single file")
            << QStringLiteral("zip")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testfile.txt")}
            << QStringLiteral("testfile.zip")
            << 1ULL;

        QTest::newRow("compress here (as ZIP) - file + folder")
            << QStringLiteral("zip")
            << Archive::Unencrypted
            << QStringList {
                   QFINDTESTDATA("data/testdir"),
                   QFINDTESTDATA("data/testfile.txt")
               }
            << QStringLiteral("data.zip")
            << 4ULL;

        QTest::newRow("compress here (as TAR) - dir with special name (see #365798)")
            << QStringLiteral("tar.gz")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/test%dir")}
            << QStringLiteral("test%dir.tar.gz")
            << 3ULL;

    } else {
        qDebug() << "7z/zip executable not found in path. Skipping compress-here-(ZIP) tests.";
    }

    if (!PluginManager().preferredWritePluginsFor(QMimeDatabase().mimeTypeForName(QStringLiteral("application/vnd.rar"))).isEmpty()) {
        QTest::newRow("compress here (as RAR) - dir with files")
            << QStringLiteral("rar")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testdir")}
            << QStringLiteral("testdir.rar")
            << 3ULL;

        QTest::newRow("compress here (as RAR) - dir with subdirs")
            << QStringLiteral("rar")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testdirwithsubdirs")}
            << QStringLiteral("testdirwithsubdirs.rar")
            << 8ULL;

        QTest::newRow("compress here (as RAR) - dir with empty subdir")
            << QStringLiteral("rar")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testdirwithemptysubdir")}
            << QStringLiteral("testdirwithemptysubdir.rar")
            << 4ULL;

        QTest::newRow("compress here (as RAR) - single file")
            << QStringLiteral("rar")
            << Archive::Unencrypted
            << QStringList {QFINDTESTDATA("data/testfile.txt")}
            << QStringLiteral("testfile.rar")
            << 1ULL;

        QTest::newRow("compress here (as RAR) - file + folder")
            << QStringLiteral("rar")
            << Archive::Unencrypted
            << QStringList {
                   QFINDTESTDATA("data/testdir"),
                   QFINDTESTDATA("data/testfile.txt")
               }
            << QStringLiteral("data.rar")
            << 4ULL;

        QTest::newRow("compress to encrypted RAR - file + folder")
            << QStringLiteral("rar")
            << Archive::Encrypted
            << QStringList {
                   QFINDTESTDATA("data/testdir"),
                   QFINDTESTDATA("data/testfile.txt")
               }
            << QStringLiteral("data.rar")
            << 4ULL;
    } else {
        qDebug() << "rar executable not found in path. Skipping compress-here-(RAR) tests.";
    }
}

void AddToArchiveTest::testCompressHere()
{
    AddToArchive *addToArchiveJob = new AddToArchive(this);
    addToArchiveJob->setChangeToFirstPath(true);

    QFETCH(QString, expectedSuffix);
    addToArchiveJob->setAutoFilenameSuffix(expectedSuffix);

    QFETCH(Archive::EncryptionType, expectedEncryptionType);
    if (expectedEncryptionType == Archive::Encrypted) {
        addToArchiveJob->setPassword(QStringLiteral("1234"));
    }

    QFETCH(QStringList, inputFiles);
    for (const QString &file : std::as_const(inputFiles)) {
        addToArchiveJob->addInput(QUrl::fromUserInput(file));
    }

    // Run the job.
    TestHelper::startAndWaitForResult(addToArchiveJob);

    // Check the properties of the generated test archive, then remove it.
    QFETCH(QString, expectedArchiveName);
    auto loadJob = Archive::load(QFINDTESTDATA(QStringLiteral("data/%1").arg(expectedArchiveName)));
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();

    QVERIFY(archive);
    QVERIFY(archive->isValid());
    QCOMPARE(QString(archive->completeBaseName() + QLatin1Char('.') + expectedSuffix), expectedArchiveName);

    QCOMPARE(archive->encryptionType(), expectedEncryptionType);

    QFETCH(qulonglong, expectedNumberOfEntries);
    QCOMPARE(archive->numberOfEntries(), expectedNumberOfEntries);

    QVERIFY(QFile(archive->fileName()).remove());

    loadJob->deleteLater();
    archive->deleteLater();
}

QTEST_MAIN(AddToArchiveTest)

#include "addtoarchivetest.moc"
