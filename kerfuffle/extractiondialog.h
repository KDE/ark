/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2007 Henrique Pinto <henrique.pinto@kdemail.net>
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2012 basysKom GmbH <info@basyskom.com>
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

#ifndef EXTRACTIONDIALOG_H
#define EXTRACTIONDIALOG_H

#include <KDialog>

#include "archive.h"
#include "kerfuffle_export.h"

class KConfigGroup;
class KUrl;

namespace Kerfuffle
{
class KERFUFFLE_EXPORT ExtractionDialog : public KDialog
{
    Q_OBJECT
public:
    ExtractionDialog(QWidget *parent = 0);
    virtual ~ExtractionDialog();

    void setBatchMode(bool enabled);
    KUrl destination() const;
    virtual void accept();
    void setOptions(const ExtractionOptions& options = ExtractionOptions());
    ExtractionOptions options() const;

public Q_SLOTS:
    void setDestination(const KUrl& url);

private Q_SLOTS:
    void loadSettings();
    void writeSettings();
    void updateView();

private:
    class ExtractionDialogUI *m_ui;
    KUrl m_url;
    KConfigGroup m_config;
};
}

#endif // EXTRACTIONDIALOG_H
