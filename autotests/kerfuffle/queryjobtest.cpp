/*
    SPDX-FileCopyrightText: 2026 Méven Car <meven@kde.org>

    SPDX-License-Identifier: BSD-2-Clause
*/

#include "archiveinterface.h"
#include "jobs.h"
#include "queries.h"

#include <KPluginMetaData>

#include <QEventLoop>
#include <QPointer>
#include <QTest>
#include <QTimer>

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

    // Whether the job should be asked to delete itself while the question is open, as a
    // job whose work ends on this thread does.
    void setJobToDelete(QObject *job)
    {
        m_jobToDelete = job;
    }

    bool list() override
    {
        auto query = std::make_shared<TestQuery>();
        Q_EMIT userQuery(query);
        if (m_jobToDelete) {
            m_jobToDelete->deleteLater();
        }
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
    QObject *m_jobToDelete = nullptr;
    bool m_answer = false;
};

}

class QueryJobTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void keepsTheJobUntilTheQuestionIsAnswered();
    void givesUpOnTheQuestionWhenTheJobGoesAway();
};

void QueryJobTest::keepsTheJobUntilTheQuestionIsAnswered()
{
    auto interface = new QueryingInterface(this);
    QPointer<LoadJob> job = new LoadJob(interface);
    interface->setJobToDelete(job);

    connect(job.data(), &Job::userQuery, this, [&job](std::shared_ptr<Query> query) {
        // Whoever answers the question runs an event loop of its own while the dialog is
        // up. A deletion asked for by the thread of the job arrives in that loop.
        QEventLoop dialog;
        QTimer::singleShot(200, &dialog, &QEventLoop::quit);
        dialog.exec();

        // The thread of the job waits for this answer and the destructor of the job waits
        // for that thread to end, so the job has to outlive the question.
        QVERIFY(!job.isNull());

        std::static_pointer_cast<TestQuery>(query)->answer();
    });

    job->start();

    // A deletion asked for while an event loop runs is carried out when that loop is done
    // with it, so the wait here runs one rather than spinning on processEvents().
    QEventLoop wait;
    QTimer::singleShot(2000, &wait, &QEventLoop::quit);
    wait.exec();

    QVERIFY(job.isNull());
    QVERIFY(interface->answer());
}

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
