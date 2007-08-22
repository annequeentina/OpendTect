/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Lammertink
 Date:          08/12/1999
 RCS:           $Id: pixmap.cc,v 1.25 2007-08-22 10:53:57 cvsbert Exp $
________________________________________________________________________

-*/

#include "pixmap.h"
#include "errh.h"

#include "color.h"
#include "arraynd.h"
#include "arrayndimpl.h"
#include "arrayrgb.h"
#include "uirgbarray.h"
#include "colortab.h"
#include "separstr.h"
#include "filegen.h"
#include "filepath.h"
#include "oddirs.h"

#include <qpixmap.h>
#include <qbitmap.h>
#include <qcolor.h>


ioPixmap::ioPixmap( const ioPixmap& pm )
    : qpixmap_(new QPixmap(*pm.qpixmap_))
    , srcname_(pm.srcname_)
{
}


ioPixmap::ioPixmap( const ArrayRGB& anImage )
    : qpixmap_(new QPixmap)
    , srcname_("[ArrayRGB]")
{
    convertFromArrayRGB( anImage );
}


ioPixmap::ioPixmap( const uiRGBArray& anImage )
    : qpixmap_(new QPixmap)
    , srcname_("[uiRGBArray]")
{
    convertFromRGBArray( anImage );
}


ioPixmap::ioPixmap( const char* xpm[] )
    : qpixmap_(new QPixmap(xpm))
    , srcname_("[xpm]")
{
}


ioPixmap::ioPixmap( int w, int h )
    : qpixmap_(new QPixmap(w,h))
    , srcname_("[created]")
{
}


ioPixmap::ioPixmap( const QPixmap& pm )
    : qpixmap_(new QPixmap(pm))
{
}


ioPixmap::ioPixmap( const char* fnm, const char* fmt )
    : qpixmap_(0)
    , srcname_(fnm)
{
    if ( fmt )
    {
	FileMultiString fms( fnm );
	fms += fmt;
	srcname_ = fms;
    }

    BufferString fname( srcname_ );
    FilePath fp( fname );
    if ( !fp.isAbsolute() )
    {
	fp.setPath( GetSettingsFileName("icons") );
	fname = fp.fullPath();
	if ( !File_exists(fname) )
	{
	    fp.setPath( mGetSetupFileName("icons.cur") );
	    fname = fp.fullPath();
	    if ( !File_exists(fname) )
	    {
		fp.setPath( mGetSetupFileName("icons.Default") );
		fname = fp.fullPath();
	    }
	}
    }

    qpixmap_ = new QPixmap( fname.buf(), fmt );
}

    
ioPixmap::ioPixmap( const ColorTable& ctabin, int width, int height )
    : qpixmap_(0)
    , srcname_("[colortable]")
{
    ColorTable ctab = ctabin;
    ctab.calcList( width );
    if ( ctab.nrColors() == 0 )
    {
	qpixmap_ = new QPixmap( width, height );
	return;
    }

    ArrayRGB rgb( height, width );
    for ( int idx1=0; idx1<rgb.info().getSize(1); idx1++ )
    {
	const Color color = ctab.tableColor( idx1 );
	for ( int idx2=0; idx2<rgb.info().getSize(0); idx2++ )
	    rgb.set( idx2, idx1, color );
    }

    qpixmap_ = new QPixmap;
    convertFromArrayRGB( rgb );
}


ioPixmap::~ioPixmap()
{
    releaseDrawTool();
    if ( qpixmap_ )
	delete qpixmap_;
}


void ioPixmap::convertFromArrayRGB( const ArrayRGB& theImage )
{
    releaseDrawTool();

    if( !qpixmap_ ) qpixmap_ = new QPixmap;
#ifdef USEQT3
    qpixmap_->convertFromImage( theImage.qImage(), Qt::OrderedAlphaDither);
#else
    *qpixmap_ = QPixmap::fromImage( theImage.qImage(), Qt::OrderedAlphaDither);
#endif
}    


void ioPixmap::convertFromRGBArray( const uiRGBArray& theImage )
{
    releaseDrawTool();

    if( !qpixmap_ ) qpixmap_ = new QPixmap;
#ifdef USEQT3
    qpixmap_->convertFromImage( theImage.qImage(), Qt::OrderedAlphaDither);
#else
    *qpixmap_ = QPixmap::fromImage( theImage.qImage(), Qt::OrderedAlphaDither);
#endif
}    


QPaintDevice* ioPixmap::qPaintDevice()
{ return qpixmap_; }

int ioPixmap::width() const
{ return qpixmap_->width(); }

int ioPixmap::height() const
{ return qpixmap_->height(); }

bool ioPixmap::isEmpty() const
{ return !qpixmap_ || qpixmap_->isNull(); }

void ioPixmap::fill( const Color& col )
{ qpixmap_->fill( QColor(col.r(),col.g(),col.b()) ); }


ioBitmap::ioBitmap( const char* filenm, const char * format )
{
    qpixmap_ = new QBitmap( filenm, format );
    srcname_ = filenm;
}


QBitmap* ioBitmap::Bitmap() { return (QBitmap*)qpixmap_; }
const QBitmap* ioBitmap::Bitmap() const { return (const QBitmap*)qpixmap_; }
