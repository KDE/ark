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
    for (const auto &expectedPath : qAsConst(expectedNewPaths)) {
        QVERIFY(!oldPaths.contains(expectedPath));
    }

    CompressionOptions options;
    options.setGlobalWorkDir(QFINDTESTDATA("data"));
    AddJob *addJob = archive->addFiles(targetEntries, destination, options);
    TestHelper::startAndWaitForResult(addJob);

    // Retrieve the resulting paths.
    QStringList newPaths = getEntryPaths(archive);

    // Check that the expected paths are now in the archive.
    for (const auto &path : qAsConst(expectedNewPaths)) {
        QVERIFY(newPaths.contains(path));
    }

    QFETCH(uint, numberOfEntries);
    QCOMPARE(archive->numberOfEntries(), numberOfEntries);

    loadJob->deleteLater();
    archive->deleteLater();
}

#include "addtest.moc"
