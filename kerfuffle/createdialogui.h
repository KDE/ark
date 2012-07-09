#ifndef CREATEDIALOGUI_H
#define CREATEDIALOGUI_H

#include <KUrl>
#include <QString>
#include <QMultiHash>

#include "ui_createdialogui.h"
#include "kerfuffle/archive.h"

namespace Kerfuffle
{

class CreateDialogUI: public QWidget, public Ui::CreateDialog
{
    Q_OBJECT

public:
    CreateDialogUI(QWidget *parent = 0);

    CompressionOptions options() const;
    void setOptions(const CompressionOptions& options = CompressionOptions());

    KUrl archiveUrl() const;
    void setArchiveUrl(const KUrl& archiveUrl);

public slots:
    bool checkArchiveUrl();

private slots:
    void browse();
    void updateArchiveExtension(bool updateCombobox = false);
    void updateUi();

private:
    QMultiHash<QString,int> m_mimeTypeOptions;

};
}

#endif // CREATEDIALOGUI_H
