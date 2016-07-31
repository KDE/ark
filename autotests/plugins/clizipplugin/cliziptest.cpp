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

#include "cliziptest.h"
#include <QTest>

QTEST_GUILESS_MAIN(CliZipTest)

using namespace Kerfuffle;

void CliZipTest::testListArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("fake zip")
            << QStringLiteral("/tmp/foo.zip")
            << QStringList {
                   QStringLiteral("-l"),
                   QStringLiteral("-T"),
                   QStringLiteral("-z"),
                   QStringLiteral("/tmp/foo.zip")
               };
}

void CliZipTest::testListArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList listArgs = { QStringLiteral("-l"),
                                   QStringLiteral("-T"),
                                   QStringLiteral("-z"),
                                   QStringLiteral("$Archive") };

    const auto replacedArgs = plugin->substituteListVariables(listArgs, QString());

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliZipTest::testAddArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QString>("password");
    QTest::addColumn<int>("compressionLevel");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("unencrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QString() << 3
            << QStringList {
                   QStringLiteral("-r"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("-3")
               };

    QTest::newRow("encrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QStringLiteral("1234") << 3
            << QStringList {
                   QStringLiteral("-r"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("-P1234"),
                   QStringLiteral("-3")
               };
}

void CliZipTest::testAddArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList addArgs = { QStringLiteral("-r"),
                                  QStringLiteral("$Archive"),
                                  QStringLiteral("$PasswordSwitch"),
                                  QStringLiteral("$CompressionLevelSwitch"),
                                  QStringLiteral("$Files") };

    QFETCH(QString, password);
    QFETCH(int, compressionLevel);

    QStringList replacedArgs = plugin->substituteAddVariables(addArgs, {}, password, false, compressionLevel);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}

void CliZipTest::testExtractArgs_data()
{
    QTest::addColumn<QString>("archiveName");
    QTest::addColumn<QList<Archive::Entry*>>("files");
    QTest::addColumn<bool>("preservePaths");
    QTest::addColumn<QString>("password");
    QTest::addColumn<QStringList>("expectedArgs");

    QTest::newRow("preserve paths, encrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-P1234"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("preserve paths, unencrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << true << QString()
            << QStringList {
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, encrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QStringLiteral("1234")
            << QStringList {
                   QStringLiteral("-j"),
                   QStringLiteral("-P1234"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };

    QTest::newRow("without paths, unencrypted")
            << QStringLiteral("/tmp/foo.zip")
            << QList<Archive::Entry*> {
                   new Archive::Entry(this, QStringLiteral("aDir/textfile2.txt"), QStringLiteral("aDir")),
                   new Archive::Entry(this, QStringLiteral("c.txt"), QString())
               }
            << false << QString()
            << QStringList {
                   QStringLiteral("-j"),
                   QStringLiteral("/tmp/foo.zip"),
                   QStringLiteral("aDir/textfile2.txt"),
                   QStringLiteral("c.txt"),
               };
}

void CliZipTest::testExtractArgs()
{
    QFETCH(QString, archiveName);
    CliPlugin *plugin = new CliPlugin(this, {QVariant(archiveName)});
    QVERIFY(plugin);

    const QStringList extractArgs = { QStringLiteral("$PreservePathSwitch"),
                                      QStringLiteral("$PasswordSwitch"),
                                      QStringLiteral("$Archive"),
                                      QStringLiteral("$Files") };

    QFETCH(QList<Archive::Entry*>, files);
    QFETCH(bool, preservePaths);
    QFETCH(QString, password);

    QStringList replacedArgs = plugin->substituteCopyVariables(extractArgs, files, preservePaths, password);

    QFETCH(QStringList, expectedArgs);
    QCOMPARE(replacedArgs, expectedArgs);

    plugin->deleteLater();
}
