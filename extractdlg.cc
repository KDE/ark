/* (c)1997 Robert Palmbos
   (c)1998 David Faure
   See main.cc for license details */
#include "extractdlg.h"
#include "extractdlg.moc"
#include <qlayout.h>
#include <kfiledialog.h>

// Layout code heavily borrowed from the qt example "widgets.cpp"

ExtractDlg::ExtractDlg( int eo, QWidget *parent, char *name )
    : QDialog( parent, name, TRUE )
{
    setCaption(i18n("Extract"));
    // Create a layout to position the widgets
    QVBoxLayout *topLayout = new QVBoxLayout( this, 10 /* border */);
 
    // Create a horizontal layout to hold most of the widgets (not the buttons)
    QHBoxLayout *hLayout = new QHBoxLayout( 8 /* autoBorder */ );
    topLayout->addLayout( hLayout, 3 /* stretch */);

    // Create a vertical layout for the two groupboxes in the left column
    QVBoxLayout *vLayout = new QVBoxLayout( 8 /* autoBorder */ );
    hLayout->addLayout( vLayout, 1 /* stretch */);

    // Left box :
    //   Upper box (Destination)
    QGroupBox *gbDest = new QGroupBox( i18n("Destination"), this );
    vLayout->addWidget(gbDest, 1, AlignLeft);
    QVBoxLayout *vboxDest = new QVBoxLayout(gbDest, 10);
    vboxDest->addSpacing( gbDest->fontMetrics().height() );

    le = new QLineEdit( gbDest );
    le->setText( getenv( "HOME" ) );
    le->setFocus();
    le->setMinimumSize( le->sizeHint() );
    vboxDest->addWidget(le);
 
    QPushButton *pb1 = new QPushButton( i18n("Browse..."), gbDest );
    pb1->setFixedSize( pb1->sizeHint() );
    vboxDest->addWidget(pb1, 0, AlignRight);

    //  Lower box (Extract options)
    QButtonGroup *bgOptions = new QButtonGroup( i18n("Extract Options"), this );
    vLayout->addWidget(bgOptions, 1, AlignLeft);
    QVBoxLayout *vboxOptions = new QVBoxLayout(bgOptions, 10);
    vboxOptions->addSpacing( bgOptions->fontMetrics().height() );

    //cb1 = new QCheckBox( i18n("Overwrite"), bgOptions );
    //vboxOptions->addWidget(cb1);

    cb2 = new QCheckBox( i18n("Preserve Permissions"), bgOptions );
    cb2->setMinimumSize( cb2->sizeHint() );
    bgOptions->insert(cb2);
    vboxOptions->addWidget(cb2);

    cb3 = new QCheckBox( i18n("Filenames to Lowercase"), bgOptions );
    cb3->setMinimumSize( cb3->sizeHint() );
    bgOptions->insert(cb3);
    vboxOptions->addWidget(cb3);

    // Right box (Files) :
    QButtonGroup *bgFiles = new QButtonGroup( i18n( "Files"), this );    
    hLayout->addWidget(bgFiles, 1, AlignLeft);
    QVBoxLayout *vboxFiles = new QVBoxLayout(bgFiles, 10);
    vboxFiles->addSpacing( bgFiles->fontMetrics().height() );

    rb1 = new QRadioButton( i18n("All Files"), bgFiles );
    rb1->setMinimumSize( rb1->sizeHint() );
    bgFiles->insert(rb1);
    vboxFiles->addWidget(rb1);
 
    rb2 = new QRadioButton( i18n("Selected File"), bgFiles );
    rb2->setMinimumSize( rb2->sizeHint() );
    bgFiles->insert(rb2);
    vboxFiles->addWidget(rb2);

    rb3 = new QRadioButton( i18n("Pattern"), bgFiles );
    rb3->setMinimumSize( rb3->sizeHint() );
    bgFiles->insert(rb3);
    vboxFiles->addWidget(rb3);
    rb3->setEnabled( FALSE );

    switch( eo ) {
        case All: {
            rb1->setChecked( true );
            break;
        }
        case Selected: {
            rb2->setChecked( true );
            break;
        }
        case Pattern: {
            rb3->setChecked( true );
            break;
        }
        default: {
            rb1->setChecked( true );
            break;
        }
    }

    QLineEdit *le2 = new QLineEdit( bgFiles );
    le2->setMinimumSize( le2->sizeHint() );
    le2->setEnabled( FALSE );
    vboxFiles->addWidget(le2);

    // Add space at the bottom of each box (in case it grows)
    vboxDest->addStretch(1);
    vboxFiles->addStretch(1);
    vboxOptions->addStretch(1);

    // Create a layout for the two buttons
    QHBoxLayout *buttonLayout = new QHBoxLayout( );
    topLayout->addLayout( buttonLayout, 0/* stretch */ );
    
    buttonLayout->addStretch(1);
    QPushButton *bOk = new QPushButton( i18n("OK"), this );
    buttonLayout->addWidget(bOk,1);
    buttonLayout->addStretch(1);
    bOk->setFixedSize( bOk->sizeHint() );
    QPushButton *bCancel = new QPushButton( i18n("Cancel"), this );
    buttonLayout->addWidget(bCancel,1);
    buttonLayout->addStretch(1);
    bCancel->setFixedSize( bCancel->sizeHint() );

    // Do the connects
    connect( pb1, SIGNAL( clicked() ), SLOT( browse() ) );
    connect( le, SIGNAL( returnPressed() ), SLOT( accept() ) );
    connect( bOk, SIGNAL( clicked() ), SLOT( accept() ) );
    connect( bCancel, SIGNAL( clicked() ), SLOT( reject() ) );

    topLayout->activate(); 

    // Minimum sizes
    bgOptions->setMinimumSize( bgOptions->childrenRect().size() );
    bgFiles->setMinimumSize( bgFiles->childrenRect().size() );
}

void ExtractDlg::setMask( unsigned char mask )
{
    if( ( 0x01 & mask ) == 0 )
        cb2->setEnabled( FALSE );
    if( ( 0x02 & mask ) == 0 )
        cb3->setEnabled( FALSE );
    //if( 0x04 & mask ) == 0 )
    //cb1->setEnabled( FALSE );
}

const char *ExtractDlg::getDest()
{
    return le->text();
}

bool ExtractDlg::doOverwrite()
{
    //return cb1->isChecked();
    return FALSE;
}

bool ExtractDlg::doPreservePerms()
{
    return cb2->isChecked();
}

bool ExtractDlg::doLowerCase()
{
    return cb3->isChecked();
}

const char *ExtractDlg::getPattern()
{
    return le2->text();
}

int ExtractDlg::extractOp()
{
    if( rb1->isChecked() )
        return All;
    if( rb2->isChecked() )
        return Selected;
    if( rb3->isChecked() )
        return Pattern;
    return -1;
}

void ExtractDlg::browse()
{
    KDirDialog dd( le->text(), 0, "dirdialog");
    dd.setCaption(i18n("Extract To"));
    if (dd.exec())
        le->setText( dd.selectedFile() );
}

