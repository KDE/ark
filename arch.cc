/* (c)1997 Robert Palmbos
   See main.cc for license details */
#include "arch.h"
#include "arch.moc"

Arch::Arch()
	: QObject()
{
	pd = 0;
}

Arch::~Arch()
{
}

unsigned char Arch::setOptions( bool, bool, bool )
{
	return 0;
}

void Arch::newProgressDialog( long int initial, long int max )
{
	delete pd;

	total = max;
	canceled = FALSE;
	if( (max-initial) < 2 )
	{
		kp = 0;
		return;
	}
	pd = new QDialog;
	kp = new KProgress( initial, max, 1, KProgress::Horizontal, pd );
	kp->setGeometry( 10, 10, 200, 20 );
	QPushButton *can = new QPushButton( klocale->translate("Cancel"), pd );
	can->setGeometry( 75, 35, 60, 30 );
	connect( can, SIGNAL( clicked() ), this, SLOT( cancel() ) );
	pd->show();
	kapp->processEvents();
}

void Arch::setProgress( long int num_done )
{
	if( kp!=0 )
		kp->setValue( num_done );
	kapp->processEvents();
	if( num_done == total )
	{
		delete pd;
		pd = 0;
	}
}

void Arch::cancel()
{
	delete pd;
	pd = 0;
	canceled = TRUE;
}

int Arch::isCanceled()
{
	return canceled;
}

void Arch::onlyUpdate( bool )
{
}

void Arch::addPath( bool )
{
}

void Arch::openArch( QString )
{
}

void Arch::createArch( QString )
{
}

const QStrList *Arch::getListing()
{
	return 0;
}


int Arch::addFile( QStrList *)
{
	return FALSE;
}

void Arch::extractTo( QString )
{
}

QString Arch::unarchFile( int, QString )
{
}

void Arch::deleteFile( int )
{
}

const char * Arch::getHeaders()
{
	return 0;
}