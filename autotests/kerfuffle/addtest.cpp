/*
    SPDX-FileCopyrightText: 2016 Vladyslav Batyrenko <mvlabat@gmail.com>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "abstractaddtest.h"
#include "archiveentry.h"
#include "jobs.h"
#include "testhelper.h"

#include <QTest>

using namespace Kerfuffle;

class AddTest : public AbstractAddTest
{
    Q_OBJECT

public:
    AddTest() : AbstractAddTest() {}

private Q_SLOTS:
    void testAdding_data();
    void testAdding();
};

QTEST_GUILESS_MAIN(AddTest)

void AddTest::testAdding_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<Plugin*>("plugin");
    QTest::addColumn<QVector<Archive::Entry*>>("targetEntries");
    QTest::addColumn<Archive::Entry*>("destination");
    QTest::addColumn<QStringList>("expectedNewPaths");
    QTest::addColumn<uint>("numberOfEntries");

    setupRows(QStringLiteral("without destination"),
              QStringLiteral("test"),
              QVector<Archive::Entry*> {
                  new Archive::Entry(this, QStringLiteral("textfile1.txt")),
                  new Archive::Entry(this, QStringLiteral("textfile2.txt"))
              },
              new Archive::Entry(this),
              QStringList {
                  QStringLiteral("textfile1.txt"),
                  QStringLiteral("textfile2.txt")
              },
              15);

    setupRows(QStringLiteral("with destination, files"),
              QStringLiteral("test"),
              QVector<Archive::Entry*> {
                  new Archive::Entry(this, QStringLiteral("textfile1.txt")),
                  new Archive::Entry(this, QStringLiteral("textfile2.txt"))
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList {
                  QStringLiteral("empty_dir/textfile1.txt"),
                  QStringLiteral("empty_dir/textfile2.txt")
              },
              15);

    setupRows(QStringLiteral("with destination, directory"),
              QStringLiteral("test"),
              QVector<Archive::Entry*> {
                  new Archive::Entry(this, QStringLiteral("testdir/")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList {
                  QStringLiteral("empty_dir/testdir/testfile1.txt"),
                  QStringLiteral("empty_dir/testdir/testfile2.txt")
              },
              16);

    setupRows(QStringLiteral("without destination, directory 2"),
              QStringLiteral("test"),
              QVector<Archive::Entry*> {
                  new Archive::Entry(this, QStringLiteral("testdir2/")),
              },
              new Archive::Entry(this),
              QStringList {
                  QStringLiteral("testdir2/testdir/testfile1.txt"),
                  QStringLiteral("testdir2/testdir/testfile2.txt")
              },
              17);

    setupRows(QStringLiteral("with destination, directory 2"),
              QStringLiteral("test"),
              QVector<Archive::Entry*> {
                  new Archive::Entry(this, QStringLiteral("testdir2/")),
              },
              new Archive::Entry(this, QStringLiteral("empty_dir/")),
              QStringList {
                  QStringLiteral("empty_dir/testdir2/testdir/testfile1.txt"),
                  QStringLiteral("empty_dir/testdir2/testdir/testfile2.txt")
              },
              17);

    setupRows(QStringLiteral("overwriting an existing entry"),
              QStringLiteral("test"),
              QVector<Archive::Entry*> {
                  new Archive::Entry(this, QStringLiteral("a.txt")),
              },
              new Archive::Entry(this),
              QStringList(),
              13);
}

void AddTest::testAdding()
{
    QTemporaryDir temporaryDir;

    QFETCH(QString, archiveName);
    const QString archivePath = temporaryDir.path() + QLatin1Char('/') + archiveName;
    QVERIFY(QFile::copy(QFINDTESTDATA(QStringLiteral("data/") + archiveName), archivePath));

    QFETCH(Plugin*, plugin);
    QVERIFY(plugin);

    auto loadJob = Archive::load(archivePath, plugin);
    QVERIFY(loadJob);
    loadJob->setAutoDelete(false);

    TestHelper::startAndWaitForResult(loadJob);
    auto archive = loadJob->archive();
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QFETCH(QVector<Archive::Entry*>, targetEntries);
    QFETCH(Archive::Entry*, destination);
    QFETCH(QStringList, expectedNewPaths);

    // Retrieve current paths in the archive.
    QStringList oldPaths = getEntryPaths(archive);

    // Check that the expected paths (after the AddJob) are not in the archive.
    for (const auto &expectedPath : std::as_const(expectedNewPaths)) {
        QVERIFY(!oldPaths.contains(expectedPath));
    }

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA("data"));
    AddJob *addJob = archive->addFiles(targetEntries, destination, options);
    TestHelper::startAndWaitForResult(addJob);

    // Retrieve the resulting paths.
    QStringList newPaths = getEntryPaths(archive);

    // Check that the expected paths are now in the archive.
    for (const auto &path : std::as_const(expectedNewPaths)) {
        QVERIFY(newPaths.contains(path));
    }

    QFETCH(uint, numberOfEntries);
    QCOMPARE(archive->numberOfEntries(), numberOfEntries);

    loadJob->deleteLater();
    archive->deleteLater();
}

#include "addtest.moc"
