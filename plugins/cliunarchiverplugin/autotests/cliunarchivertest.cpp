/*
 * Copyright (C) 2016 Elvis Angelaccio <elvis.angelaccio@kdemail.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 */

#include "cliunarchivertest.h"

#include <QFile>
#include <QSignalSpy>
#include <QTest>
#include <QTextStream>

QTEST_GUILESS_MAIN(CliUnarchiverTest)

using namespace Kerfuffle;

void CliUnarchiverTest::initTestCase()
{
    qRegisterMetaType<ArchiveEntry>();
}

void CliUnarchiverTest::testList_data()
{
    QTest::addColumn<QString>("jsonFilePath");
    QTest::addColumn<int>("expectedEntriesCount");
    // Index of some entry to be tested.
    QTest::addColumn<int>("someEntryIndex");
    // Entry metadata.
    QTest::addColumn<QString>("expectedName");
    QTest::addColumn<bool>("isDirectory");
    QTest::addColumn<bool>("isPasswordProtected");
    QTest::addColumn<qulonglong>("expectedSize");
    QTest::addColumn<qulonglong>("expectedCompressedSize");
    QTest::addColumn<QString>("expectedTimestamp");

    QTest::newRow("archive with one top-level folder")
            << QFINDTESTDATA("data/one_toplevel_folder.json") << 9
            << 6 << QStringLiteral("A/B/C/") << true << false << (qulonglong) 0 << (qulonglong) 0 << QStringLiteral("2015-12-21 16:57:20 +0100");
    QTest::newRow("archive with multiple top-level entries")
            << QFINDTESTDATA("data/multiple_toplevel_entries.json") << 12
            << 4 << QStringLiteral("data/A/B/test1.txt") << false << false << (qulonglong) 7 << (qulonglong) 7 << QStringLiteral("2015-12-21 16:56:28 +0100");
    QTest::newRow("archive with encrypted entries")
            << QFINDTESTDATA("data/encrypted_entries.json") << 9
            << 5 << QStringLiteral("A/test1.txt") << false << true << (qulonglong) 7 << (qulonglong) 32 << QStringLiteral("2015-12-21 16:56:28 +0100");
    QTest::newRow("huge archive")
            << QFINDTESTDATA("data/huge_archive.json") << 250
            << 8 << QStringLiteral("PsycOPacK/Base Dictionnaries/att800") << false << false << (qulonglong) 593687 << (qulonglong) 225219 << QStringLiteral("2011-08-14 03:10:10 +0200");
}

void CliUnarchiverTest::testList()
{
    CliPlugin *unarPlugin = new CliPlugin(this, {QStringLiteral("dummy.rar")});
    QSignalSpy signalSpy(unarPlugin, SIGNAL(entry(ArchiveEntry)));

    QFETCH(QString, jsonFilePath);
    QFETCH(int, expectedEntriesCount);

    QFile jsonFile(jsonFilePath);
    QVERIFY(jsonFile.open(QIODevice::ReadOnly));

    QTextStream stream(&jsonFile);
    unarPlugin->setJsonOutput(stream.readAll());

    QCOMPARE(signalSpy.count(), expectedEntriesCount);

    QFETCH(int, someEntryIndex);
    QVERIFY(someEntryIndex < signalSpy.count());
    ArchiveEntry entry = qvariant_cast<ArchiveEntry>(signalSpy.at(someEntryIndex).at(0));

    QFETCH(QString, expectedName);
    QCOMPARE(entry[FileName].toString(), expectedName);

    QFETCH(bool, isDirectory);
    QCOMPARE(entry[IsDirectory].toBool(), isDirectory);

    QFETCH(bool, isPasswordProtected);
    QCOMPARE(entry[IsPasswordProtected].toBool(), isPasswordProtected);

    QFETCH(qulonglong, expectedSize);
    QCOMPARE(entry[Size].toULongLong(), expectedSize);

    QFETCH(qulonglong, expectedCompressedSize);
    QCOMPARE(entry[CompressedSize].toULongLong(), expectedCompressedSize);

    QFETCH(QString, expectedTimestamp);
    QCOMPARE(entry[Timestamp].toString(), expectedTimestamp);

    unarPlugin->deleteLater();
}
