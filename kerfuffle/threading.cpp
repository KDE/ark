/*
 * Copyright (c) 2008 Harald Hvaal <haraldhv )@@@( stud(dot)ntnu.no>
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

#include "threading.h"
#include <QApplication>
#include <QTimer>
#include <KDebug>

namespace Kerfuffle
{
	ThreadExecution::ThreadExecution(Kerfuffle::Job *job)
		: m_job(job)
		, m_interface(job->interface())
	{
		m_interface->setParent(NULL);
		m_interface->moveToThread(this);

		moveToThread(this);
	}

	void ThreadExecution::run()
	{
		kDebug(1601) << "Run";
		//schedule to perform the work in this thread
		QTimer::singleShot(0, m_job, SLOT(doWork()));

		//and when finished, quit the event loop
		connect(m_job, SIGNAL(result(KJob*)), this, SLOT(slotJobFinished()));
		connect(m_job, SIGNAL(result(KJob*)), this, SLOT(quit()));

		//start the event loop
		exec();

		kDebug(1601) << "Finished exec";
	}

	void ThreadExecution::slotJobFinished()
	{
		m_interface->moveToThread(QApplication::instance()->thread());
	}
}

#include "threading.moc"
