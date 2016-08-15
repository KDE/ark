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

#include "autotests/testhelper/testhelper.h"

using namespace Kerfuffle;

class AddTest : public QObject
{
    Q_OBJECT

private:
void addAllFormatsRows(const QString testName, const QString archiveName, QList<Archive::Entry*> entries, Archive::Entry *destination) {
    QStringList formats = QStringList()
            << QStringLiteral("7z")
            << QStringLiteral("rar")
            << QStringLiteral("tar.bz2")
            << QStringLiteral("zip");

    foreach (QString format, formats) {
        const QString testNameWithFormat = testName + QStringLiteral(" (") + format + QStringLiteral(")");
        QTest::newRow(testNameWithFormat.toUtf8())
            << archiveName + QLatin1Char('.') + format
            << entries
            << destination;
    }
}

private Q_SLOTS:
    void testAdding_data();
    void testAdding();
};

QTEST_GUILESS_MAIN(AddTest)

void AddTest::testAdding_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QList<Archive::Entry*>>("files");
    QTest::addColumn<Archive::Entry*>("destination");

    addAllFormatsRows(QStringLiteral("without destination"),
                      QStringLiteral("test"),
                      QList<Archive::Entry*> {
                          new Archive::Entry(this, QStringLiteral("textfile1.txt")),
                          new Archive::Entry(this, QStringLiteral("textfile2.txt")),
                      },
                      new Archive::Entry(this));

    addAllFormatsRows(QStringLiteral("with destination, files"),
                      QStringLiteral("test"),
                      QList<Archive::Entry*> {
                          new Archive::Entry(this, QStringLiteral("textfile1.txt")),
                          new Archive::Entry(this, QStringLiteral("textfile2.txt")),
                      },
                      new Archive::Entry(this, QStringLiteral("empty_dir/")));

    addAllFormatsRows(QStringLiteral("with destination, directory"),
                      QStringLiteral("test"),
                      QList<Archive::Entry*> {
                          new Archive::Entry(this, QStringLiteral("testdir/")),
                      },
                      new Archive::Entry(this, QStringLiteral("empty_dir/")));

    addAllFormatsRows(QStringLiteral("without destination, directory 2"),
                      QStringLiteral("test"),
                      QList<Archive::Entry*> {
                          new Archive::Entry(this, QStringLiteral("testdir2/")),
                      },
                      new Archive::Entry(this));

    addAllFormatsRows(QStringLiteral("with destination, directory 2"),
                      QStringLiteral("test"),
                      QList<Archive::Entry*> {
                          new Archive::Entry(this, QStringLiteral("testdir2/")),
                      },
                      new Archive::Entry(this, QStringLiteral("empty_dir/")));
}

void AddTest::testAdding()
{
    QTemporaryDir temporaryDir;

    QFETCH(QString, archiveName);
    const QString archivePath = temporaryDir.path() + QLatin1Char('/') + archiveName;
    Q_ASSERT(QFile::copy(QFINDTESTDATA(QStringLiteral("data/") + archiveName), archivePath));
    Archive *archive = Archive::create(archivePath, this);
    QVERIFY(archive);

    if (!archive->isValid()) {
        QSKIP("Could not find a plugin to handle the archive. Skipping test.", SkipSingle);
    }

    QFETCH(QList<Archive::Entry*>, files);
    QFETCH(Archive::Entry*, destination);

    QList<Archive::Entry*> oldEntries = TestHelper::getEntryList(archive);

    CompressionOptions options = CompressionOptions();
    options.insert(QStringLiteral("GlobalWorkDir"), QFINDTESTDATA("data"));
    AddJob *addJob = archive->addFiles(files, destination, options);
    TestHelper::startAndWaitForResult(addJob);

    QList<Archive::Entry*> resultedEntries = TestHelper::getEntryList(archive);
    TestHelper::verifyAddedEntriesWithDestination(files, destination, oldEntries, resultedEntries);

    archive->deleteLater();
}

#include "addtest.moc"
