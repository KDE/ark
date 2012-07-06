/*
 * ark -- archiver for the KDE project
 *
 * Copyright (C) 2008 Harald Hvaal <haraldhv@stud.ntnu.no>
 * Copyright (C) 2009,2011 Raphael Kubo da Costa <kubito@gmail.com>
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

#include <KDebug>
#include <KGlobal>
#include <KMimeType>

#include "createdialog.h"
#include "createdialogui.h"

namespace Kerfuffle
{
CreateDialog::CreateDialog(QWidget * parent) : KDialog()
{
    m_ui = new CreateDialogUI(this);
    setMainWidget( m_ui );
    setCaption(i18nc("@title:window", "Create Archive"));
    setButtons( KDialog::User1 | KDialog::Reset |  KDialog::Ok | KDialog::Cancel );
    setButtonGuiItem( KDialog::User1,
                      KGuiItem( i18n("Set As Default"),
                                QLatin1String("configure"),
                                i18n("Set the selected values as default values"),
                                i18n("Sets the selected values as default values for new archives")));


    m_config = KConfigGroup(KGlobal::config()->group("CreateDialog"));
    loadConfiguration();

    connect(this, SIGNAL(resetClicked()), SLOT(loadConfiguration()));
    connect(this, SIGNAL(user1Clicked()), SLOT(saveConfiguration()));
}

void CreateDialog::loadConfiguration()
{
    CompressionOptions options;
    QString str;
    foreach( str, m_config.keyList() ) {
        options[str] = m_config.readEntry(str);
    }

    m_ui->setOptions( options );
}

void CreateDialog::saveConfiguration()
{
    QHashIterator<QString, QVariant> it((QHash<QString, QVariant>)m_ui->options());
    while (it.hasNext()) {
         it.next();
         m_config.writeEntry(it.key(), it.value());
     }
}

void CreateDialog::slotButtonClicked(int button)
{
    if (button == KDialog::Ok) {
        if(m_ui->checkArchiveUrl()) {
            accept();
        }
    }
    else {
        KDialog::slotButtonClicked(button);
    }
}

KUrl CreateDialog::archiveUrl() const
{
    return m_ui->archiveUrl();
}

void CreateDialog::setArchiveUrl(const KUrl& url)
{
    m_ui->setArchiveUrl( url );
}

CompressionOptions CreateDialog::options() const
{
    return m_ui->options();
}

void CreateDialog::setOptions(const CompressionOptions& options)
{
    m_ui->setOptions(options);
}

}

#include "createdialog.moc"
