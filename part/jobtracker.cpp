/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "jobtracker.h"
#include "ark_debug.h"

JobTrackerWidget::JobTrackerWidget(QWidget *parent)
        : QFrame(parent)
{
    setupUi(this);
}

JobTracker::JobTracker(QWidget *parent)
        : KAbstractWidgetJobTracker(parent)
{
    m_ui = new JobTrackerWidget(parent);
    resetUi();
}

JobTracker::~JobTracker()
{
    QSetIterator<KJob *> it(m_jobs);
    while (it.hasNext()) {
        auto job = it.next();
        job->kill();
    }
}

void JobTracker::description(KJob *job, const QString &title, const QPair< QString, QString > &f1, const QPair< QString, QString > &f2)
{
    Q_UNUSED(job)
    Q_UNUSED(f1)
    Q_UNUSED(f2)
    m_ui->descriptionLabel->setText(QStringLiteral("<b>%1</b>").arg(title));
    m_ui->descriptionLabel->show();
}

void JobTracker::infoMessage(KJob *job, const QString &message)
{
    Q_UNUSED(job)
    m_ui->informationLabel->setText(message);
    m_ui->informationLabel->show();
}

void JobTracker::warning(KJob *job, const QString &message)
{
    Q_UNUSED(job)
    m_ui->informationLabel->setText(message);
}

void JobTracker::registerJob(KJob *job)
{
    m_jobs << job;
    KAbstractWidgetJobTracker::registerJob(job);
    m_ui->show();
    m_ui->informationLabel->hide();
    m_ui->progressBar->show();
}

void JobTracker::percent(KJob *job, unsigned long percent)
{
    Q_UNUSED(job)
    m_ui->progressBar->setMaximum(100);
    m_ui->progressBar->setMinimum(0);
    m_ui->progressBar->setValue(static_cast<int>(percent));
}

void JobTracker::unregisterJob(KJob *job)
{
    m_jobs.remove(job);
    KAbstractWidgetJobTracker::unregisterJob(job);
    resetUi();
}

void JobTracker::resetUi()
{
    m_ui->hide();
    m_ui->descriptionLabel->hide();
    m_ui->informationLabel->hide();
    m_ui->progressBar->setMaximum(0);
    m_ui->progressBar->setMinimum(0);
}

QWidget* JobTracker::widget(KJob *)
{
    return m_ui;
}

#include "moc_jobtracker.cpp"
