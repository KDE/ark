/*
    SPDX-FileCopyrightText: 2007 Henrique Pinto <henrique.pinto@kdemail.net>
    SPDX-FileCopyrightText: 2008 Harald Hvaal <haraldhv@stud.ntnu.no>

    SPDX-License-Identifier: GPL-2.0-or-later
*/
#ifndef JOBTRACKER_H
#define JOBTRACKER_H

#include "ui_jobtracker.h"

#include <KAbstractWidgetJobTracker>

class KJob;

class JobTrackerWidget : public QFrame, public Ui::JobTrackerWidget
{
    Q_OBJECT

public:
    explicit JobTrackerWidget(QWidget *parent = nullptr);
};

class JobTracker : public KAbstractWidgetJobTracker
{
    Q_OBJECT

public:
    explicit JobTracker(QWidget *parent = nullptr);
    ~JobTracker() override;

    QWidget *widget(KJob *) override;

public Q_SLOTS:
    void registerJob(KJob *job) override;
    void unregisterJob(KJob *job) override;

protected Q_SLOTS:
    void description(KJob *job, const QString &title, const QPair<QString, QString> &f1, const QPair<QString, QString> &f2) override;
    void infoMessage(KJob *job, const QString &message) override;
    void warning(KJob *job, const QString &message) override;

    void percent(KJob *job, unsigned long percent) override;

private Q_SLOTS:
    void resetUi();

private:
    JobTrackerWidget *m_ui;
    QSet<KJob *> m_jobs;
};

#endif // JOBTRACKER_H
