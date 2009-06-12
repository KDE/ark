/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
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
#include "jobtracker.h"

#include <KIconLoader>
#include <QTimer>
#include <KDebug>

JobTrackerWidget::JobTrackerWidget(QWidget *parent)
        : QFrame(parent)
{
    setupUi(this);
}

JobTracker::JobTracker(QWidget *parent)
        : KAbstractWidgetJobTracker(parent), m_currentJob(0)
{
    m_ui = new JobTrackerWidget(parent);
    resetUi();
}

void JobTracker::description(KJob *job, const QString &title, const QPair< QString, QString > &f1, const QPair< QString, QString > &f2)
{
    Q_UNUSED(job);
    Q_UNUSED(f1);
    Q_UNUSED(f2);
    m_ui->descriptionLabel->setText(QString("<b>%1</b>").arg(title));
    m_ui->descriptionLabel->show();
}

void JobTracker::infoMessage(KJob *job, const QString &plain, const QString &rich)
{
    Q_UNUSED(job);
    Q_UNUSED(rich);
    m_ui->informationLabel->setText(plain);
    m_ui->informationLabel->show();
}

void JobTracker::warning(KJob *job, const QString &plain, const QString &rich)
{
    Q_UNUSED(job);
    Q_UNUSED(rich);
    m_ui->informationLabel->setText(plain);
}

void JobTracker::registerJob(KJob *job)
{
    KJobTrackerInterface::registerJob(job);
    m_ui->show();
    m_ui->informationLabel->hide();
    m_ui->progressBar->show();
    m_currentJob = job;
}

void JobTracker::percent(KJob *job, unsigned long  percent)
{
    Q_UNUSED(job);
    m_ui->progressBar->setMaximum(100);
    m_ui->progressBar->setMinimum(0);
    m_ui->progressBar->setValue(percent);
}

void JobTracker::unregisterJob(KJob *job)
{
    kDebug(1601);
    resetUi();
    m_currentJob = 0;
    KJobTrackerInterface::unregisterJob(job);
}

void JobTracker::resetUi()
{
    m_ui->hide();
    m_ui->descriptionLabel->hide();
    m_ui->informationLabel->hide();
    m_ui->progressBar->setMaximum(0);
    m_ui->progressBar->setMinimum(0);
}

void JobTracker::finished(KJob *job)
{
    QTimer::singleShot(1500, this, SLOT(resetUi()));
    m_ui->informationLabel->setText(i18n("Operation finished."));
    m_ui->informationLabel->show();
    m_ui->progressBar->hide();
    KJobTrackerInterface::unregisterJob(job);
}
