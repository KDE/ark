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

#include <KPluginLoader>
#include <KPluginMetaData>

#include <QTest>

class MetaDataTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:

    void initTestCase();
    void testPluginLoading();
    void testPluginMetadata();

private:

    QVector<KPluginMetaData> m_plugins;
};

void MetaDataTest::initTestCase()
{
    m_plugins = KPluginLoader::findPlugins(QStringLiteral("kerfuffle"));
}

// If a plugin has invalid JSON metadata (e.g. an extra comma somewhere)
// it won't occur in the list of available plugins.
void MetaDataTest::testPluginLoading()
{
    QCOMPARE(m_plugins.count() % PLUGINS_COUNT, 0);
}

void MetaDataTest::testPluginMetadata()
{
    for (const KPluginMetaData& metaData : qAsConst(m_plugins)) {
        QVERIFY(!metaData.mimeTypes().isEmpty());

        const QJsonObject json = metaData.rawData();
        QVERIFY(json.keys().contains(QLatin1String("X-KDE-Priority")));
        QVERIFY(json.keys().contains(QLatin1String("KPlugin")));

        if (json.keys().contains(QLatin1String("X-KDE-Kerfuffle-ReadOnlyExecutables"))) {
            QVERIFY(json[QStringLiteral("X-KDE-Kerfuffle-ReadOnlyExecutables")].isArray());
        }

        if (json.keys().contains(QLatin1String("X-KDE-Kerfuffle-ReadWriteExecutables"))) {
            QVERIFY(json[QStringLiteral("X-KDE-Kerfuffle-ReadWriteExecutables")].isArray());

            // If there is a list of read-write executables, the plugin has to be read-write.
            QVERIFY(json.keys().contains(QLatin1String("X-KDE-Kerfuffle-ReadWrite")));
            QVERIFY(json[QStringLiteral("X-KDE-Kerfuffle-ReadWrite")].toBool());
        }
    }
}

QTEST_GUILESS_MAIN(MetaDataTest)

#include "metadatatest.moc"
