/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archiveinterface.h"
#include "jobs.h"
#include "queries.h"

#include <KPluginMetaData>

#include <QTest>

#include <memory>

using namespace Kerfuffle;

namespace
{

// A question the test can answer itself, since the ones the plugins ask all put a dialog
// on the screen.
class TestQuery : public Query
{
public:
    TestQuery()
        : Query()
    {
    }

    void execute() override
    {
    }

    void answer()
    {
        setResponse(QVariant(true));
    }

    QVariant cancelledResponse() const override
    {
        return QVariant(false);
    }
};

// Asks a question from the thread the job runs in, the way the archive plugins do for a
// password, and waits for the answer.
class QueryingInterface : public ReadOnlyArchiveInterface
{
    Q_OBJECT

public:
    explicit QueryingInterface(QObject *parent)
        : ReadOnlyArchiveInterface(parent, {QStringLiteral("archive.json"), QVariant::fromValue(KPluginMetaData())})
    {
    }

    bool list() override
    {
        auto query = std::make_shared<TestQuery>();
        Q_EMIT userQuery(query);
        query->waitForResponse();
        m_answer = query->response().toBool();
        Q_EMIT finished(true);
        return true;
    }

    bool testArchive() override
    {
        return true;
    }

    bool extractFiles(const QList<Archive::Entry *> &, const QString &, const ExtractionOptions &) override
    {
        return true;
    }

    bool answer() const
    {
        return m_answer;
    }

private:
    bool m_answer = false;
};

}

class QueryJobTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void givesUpOnTheQuestionWhenTheJobGoesAway();
};

void QueryJobTest::givesUpOnTheQuestionWhenTheJobGoesAway()
{
    auto interface = new QueryingInterface(this);
    auto job = new LoadJob(interface);

    std::shared_ptr<Query> asked;
    connect(job, &Job::userQuery, this, [&asked](std::shared_ptr<Query> query) {
        // The question is left open, as it is while its dialog waits for the user.
        asked = query;
    });

    job->start();
    QTRY_VERIFY(asked != nullptr);

    // Deleting the job outright is what a job holding another one does. The thread is
    // still waiting for the answer, so the job has to give the question up to end.
    delete job;

    QCOMPARE(asked->response().toBool(), false);
    QVERIFY(!interface->answer());
}

QTEST_GUILESS_MAIN(QueryJobTest)

#include "queryjobtest.moc"
