/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uiaxishandler.cc,v 1.13 2008-10-27 11:12:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "uifont.h"
#include "linear.h"
#include "draw.h"

#include <math.h>

static const float logof2 = log(2);

#define mDefInitList \
      gridlineitngrp_(0) \
    , axisline_(0) \
    , annottextitem_(0) \
    , annotpostxtitemgrp_(0) \
    , nameitem_(0) \
    , annotposlineitmgrp_(0)

uiAxisHandler::uiAxisHandler( uiGraphicsScene* scene,
			      const uiAxisHandler::Setup& su )
    : NamedObject(su.name_)
    , mDefInitList  
    , scene_(scene)
    , setup_(su)
    , height_(su.height_)
    , width_(su.width_)
    , ticsz_(2)
    , beghndlr_(0)
    , endhndlr_(0)
    , ynmtxtvertical_(false)
{
    setRange( StepInterval<float>(0,1,1) );
}

uiAxisHandler::~uiAxisHandler()
{
    if ( axisline_ ) scene_->removeItem( axisline_ );
    if ( gridlineitngrp_ ) scene_->removeItem( gridlineitngrp_ );
    if ( annotpostxtitemgrp_ ) scene_->removeItem( annotpostxtitemgrp_ );
    if ( annotposlineitmgrp_ ) scene_->removeItem( annotposlineitmgrp_ );
    if ( annottextitem_ ) scene_->removeItem( annottextitem_ );
    if ( nameitem_ ) scene_->removeItem( nameitem_ );
}


void uiAxisHandler::setRange( const StepInterval<float>& rg, float* astart )
{
    rg_ = rg;
    annotstart_ = astart ? *astart : rg_.start;

    float fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps < 0 )
	rg_.step = -rg_.step;
    if ( mIsZero(fsteps,1e-6) )
	{ rg_.start -= rg_.step * 1.5; rg_.stop += rg_.step * 1.5; }
    fsteps = (rg_.stop - rg_.start) / rg_.step;
    if ( fsteps > 50 )
    	rg_.step /= (fsteps / 50);

    rgisrev_ = rg_.start > rg_.stop;
    rgwidth_ = rg_.width();

    reCalc();
}


void uiAxisHandler::reCalc()
{
    pos_.erase(); strs_.deepErase();

    StepInterval<float> annotrg( rg_ );
    annotrg.start = annotstart_;

    const uiFont& font = uiFontList::get();
    wdthy_ = font.height();
    BufferString str;

    const int nrsteps = annotrg.nrSteps();
    for ( int idx=0; idx<=nrsteps; idx++ )
    {
	float pos = annotrg.start + idx * rg_.step;
	str = pos; strs_.add( str );
	float relpos = pos - rg_.start;
	if ( rgisrev_ ) relpos = -relpos;
	relpos /= rgwidth_;
	if ( setup_.islog_ )
	    relpos = log( 1 + relpos );
	pos_ += relpos;
	const int wdth = font.width( str );
	if ( idx == 0 )			wdthx_ = font.width( str );
	else if ( wdthx_ < wdth )	wdthx_ = wdth;
    }
    endpos_ = setup_.islog_ ? logof2 : 1;
    newDevSize();
}


void uiAxisHandler::newDevSize()
{
    devsz_ = isHor() ? width_ - 10: height_ - 10;
    axsz_ = devsz_ - pixBefore() - pixAfter();
}


void uiAxisHandler::setNewDevSize( int devsz, int anotherdim )
{
    devsz_ = devsz - 10;
    axsz_ = devsz_ - pixBefore() - pixAfter();
    isHor() ? width_ = devsz_ : height_ = devsz_;
    isHor() ? height_ = anotherdim - 10 : width_ = anotherdim - 10;
}


float uiAxisHandler::getVal( int pix ) const
{
    float relpix;
    if ( isHor() )
	{ pix -= pixBefore(); relpix = pix; }
    else
	{ pix -= pixAfter(); relpix = axsz_-pix; }
    relpix /= axsz_;

    if ( setup_.islog_ )
	relpix = exp( relpix * logof2 );

    return rg_.start + (rgisrev_?-1:1) * rgwidth_ * relpix;
}


float uiAxisHandler::getRelPos( float v ) const
{
    float relv = rgisrev_ ? rg_.start - v : v - rg_.start;
    if ( !setup_.islog_ )
	return relv / rgwidth_;

    if ( relv < -0.9 ) relv = -0.9;
    return log( relv + 1 ) / logof2;
}


int uiAxisHandler::getRelPosPix( float relpos ) const
{
    return isHor() ? (int)(pixBefore() + axsz_ * relpos + .5)
		   : (int)(pixAfter() + axsz_ * (1 - relpos) + .5);
}


int uiAxisHandler::getPix( float pos ) const
{
    return getRelPosPix( getRelPos(pos) );
}


int uiAxisHandler::pixToEdge() const
{
    int ret = setup_.border_.get(setup_.side_);
    if ( setup_.noannot_ ) return ret;

    ret += ticsz_ + (isHor() ? wdthy_ : wdthx_);
    return ret;
}


int uiAxisHandler::pixBefore() const
{
    if ( beghndlr_ ) return beghndlr_->pixToEdge();
    return setup_.border_.get( isHor() ? uiRect::Left : uiRect::Bottom );
}


int uiAxisHandler::pixAfter() const
{
    if ( endhndlr_ ) return endhndlr_->pixToEdge();
    return setup_.border_.get( isHor() ? uiRect::Right : uiRect::Top );
}


Interval<int> uiAxisHandler::pixRange() const
{
    if ( isHor() )
	return Interval<int>( pixBefore(), devsz_ - pixAfter() );
    else
	return Interval<int>( pixAfter(), devsz_ - pixBefore() );
}


void uiAxisHandler::plotAxis()
{
    drawAxisLine();

    if ( gridlineitngrp_ )
    {
	if ( gridlineitngrp_->getSize() > 0 )
	{
	    for ( int idx=0; idx<gridlineitngrp_->getSize(); idx++ )
	    {
		mDynamicCastGet(uiLineItem*,lineitm,gridlineitngrp_->getUiItem(idx))
		uiRect* linerect = lineitm->lineRect();
	    }
	}
    }
    if ( setup_.style_.isVisible() )
    {
	if ( !gridlineitngrp_ )
	{
	    gridlineitngrp_ = new uiGraphicsItemGroup();
	    scene_->addItemGrp( gridlineitngrp_ );
	}
	else
	    gridlineitngrp_->removeAll( true );
	Interval<int> toplot( 0, pos_.size()-1 );
	for ( int idx=0; idx<pos_.size(); idx++ )
	{
	    const float relpos = pos_[idx] / endpos_;
	    if ( relpos>0.01 && relpos<1.01 && (!endhndlr_ || relpos<0.99) )
		drawGridLine( getRelPosPix(relpos) );
	}
    }

    if ( setup_.noannot_ ) return;

    LineStyle ls( setup_.style_ );
    ls.width_ = 1; ls.type_ = LineStyle::Solid;
    if ( !annotpostxtitemgrp_ )
    {
	annotpostxtitemgrp_ = new uiGraphicsItemGroup();
	scene_->addItemGrp( annotpostxtitemgrp_ );
    }
    else
	annotpostxtitemgrp_->removeAll( true );
    if ( !annotposlineitmgrp_ )
    {
	annotposlineitmgrp_ = new uiGraphicsItemGroup();
	scene_->addItemGrp( annotposlineitmgrp_ );
    }
    else
	annotposlineitmgrp_->removeAll( true );
    for ( int idx=0; idx<pos_.size(); idx++ )
    {
	const float relpos = pos_[idx] / endpos_;
	annotPos( getRelPosPix(relpos), strs_.get(idx), ls );
    }

    if ( !name().isEmpty() )
	drawName();
}


void uiAxisHandler::drawAxisLine()
{
    LineStyle ls( setup_.style_ );
    ls.type_ = LineStyle::Solid;

    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const int startpix = pixBefore();
	const int endpix = devsz_-pixAfter();
	const int pixpos = setup_.side_ == uiRect::Top
	    		 ? edgepix : height_ - edgepix;
	if ( !axisline_ )
	    axisline_ = scene_->addLine( startpix, pixpos, endpix, pixpos );
	else
	    axisline_->setLine( startpix, pixpos, endpix, pixpos );
	axisline_->setPenStyle( ls );
	axisline_->setZValue( 3 );
    }
    else
    {
	const int startpix = pixAfter();
	const int endpix = devsz_ - pixBefore();
	const int pixpos = setup_.side_ == uiRect::Left
	    		 ? edgepix : width_ - edgepix;

	if ( !axisline_ )
	    axisline_ = scene_->addLine( pixpos, startpix, pixpos, endpix );
	else
	    axisline_->setLine( pixpos, startpix, pixpos, endpix );
	axisline_->setPenStyle( ls );
	axisline_->setZValue( 3 );
    }
}


void drawLine( uiGraphicsScene& scene, const LinePars& lp,
	       const uiAxisHandler& xah, const uiAxisHandler& yah,
	       const Interval<float>* extxvalrg )
{
    const Interval<int> ypixrg( yah.pixRange() );
    const Interval<float> yvalrg( yah.getVal(ypixrg.start),
	    			  yah.getVal(ypixrg.stop) );
    Interval<int> xpixrg( xah.pixRange() );
    Interval<float> xvalrg( xah.getVal(xpixrg.start), xah.getVal(xpixrg.stop) );
    if ( extxvalrg )
    {
	xvalrg = *extxvalrg;
	xpixrg.start = xah.getPix( xvalrg.start );
	xpixrg.stop = xah.getPix( xvalrg.stop );
	xpixrg.sort();
	xvalrg.start = xah.getVal(xpixrg.start);
	xvalrg.stop = xah.getVal(xpixrg.stop);
    }

    uiPoint from(xpixrg.start,ypixrg.start), to(xpixrg.stop,ypixrg.stop);
    if ( lp.ax == 0 )
    {
	const int ypix = yah.getPix( lp.a0 );
	if ( !ypixrg.includes( ypix ) ) return;
	from.x = xpixrg.start; to.x = xpixrg.stop;
	from.y = to.y = ypix;
    }
    else
    {
	const float xx0 = xvalrg.start; const float yx0 = lp.getValue( xx0 );
 	const float xx1 = xvalrg.stop; const float yx1 = lp.getValue( xx1 );
	const float yy0 = yvalrg.start; const float xy0 = lp.getXValue( yy0 );
 	const float yy1 = yvalrg.stop; const float xy1 = lp.getXValue( yy1 );
	const bool yx0ok = yvalrg.includes( yx0 );
	const bool yx1ok = yvalrg.includes( yx1 );
	const bool xy0ok = xvalrg.includes( xy0 );
	const bool xy1ok = xvalrg.includes( xy1 );

	if ( !yx0ok && !yx1ok && !xy0ok && !xy1ok )
	    return;

	if ( yx0ok )
	{
	    from.x = xah.getPix( xx0 ); from.y = yah.getPix( yx0 );
	    if ( yx1ok )
		{ to.x = xah.getPix( xx1 ); to.y = yah.getPix( yx1 ); }
	    else if ( xy0ok )
		{ to.x = xah.getPix( xy0 ); to.y = yah.getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
	else if ( yx1ok )
	{
	    from.x = xah.getPix( xx1 ); from.y = yah.getPix( yx1 );
	    if ( xy0ok )
		{ to.x = xah.getPix( xy0 ); to.y = yah.getPix( yy0 ); }
	    else if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
	else if ( xy0ok )
	{
	    from.x = xah.getPix( xy0 ); from.y = yah.getPix( yy0 );
	    if ( xy1ok )
		{ to.x = xah.getPix( xy1 ); to.y = yah.getPix( yy1 ); }
	    else
		return;
	}
    }

    uiLineItem* uilinetitem = scene.addLine( from, to );
}


void uiAxisHandler::annotAtEnd( const char* txt )
{
    const int edgepix = pixToEdge();
    int xpix, ypix; Alignment al;
    if ( isHor() )
    {
	xpix = devsz_ - pixAfter() - 2;
	ypix = setup_.side_ == uiRect::Top ? edgepix  
	    				   : height_ - edgepix - 2;
	al.hor_ = OD::AlignLeft;
	al.ver_ = setup_.side_ == uiRect::Top ? OD::AlignBottom
	    				      : OD::AlignTop;
    }
    else
    {
	xpix = setup_.side_ == uiRect::Left  ? edgepix + 2 
	    				     : width_ - edgepix - 2;
	ypix = pixBefore() + 5;
	al.hor_ = setup_.side_ == uiRect::Left ? OD::AlignRight
	    				       : OD::AlignLeft;
	al.ver_ = OD::AlignVCenter;
    }


    if ( !annottextitem_ )
	annottextitem_ = scene_->addText( txt );
    else
	annottextitem_->setText( txt );
    annottextitem_->setPos( xpix, ypix );
    annottextitem_->setAlignment( al );
    annottextitem_->setZValue( 3 );

}


void uiAxisHandler::annotPos( int pix, const char* txt, const LineStyle& ls )
{
    const int edgepix = pixToEdge();
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int y0 = istop ? edgepix : height_ - edgepix;
	const int y1 = istop ? y0 - ticsz_ : y0 + ticsz_;
	uiLineItem* annotposlineitm = new uiLineItem();
	annotposlineitm->setLine( pix, y0, pix, y1 );
	annotposlineitm->setZValue( 3 );
	annotposlineitmgrp_->add( annotposlineitm );
	Alignment al( OD::AlignHCenter,
		      istop ? OD::AlignBottom : OD::AlignTop );
	uiTextItem* annotpostxtitem = new uiTextItem();
	annotpostxtitem->setText( txt );
	annotpostxtitem->setZValue( 3 );
	annotpostxtitem->setTextColor( ls.color_ );
	annotpostxtitem->setPos( pix, y1 );
	annotpostxtitem->setAlignment( al );
	annotpostxtitemgrp_->add( annotpostxtitem );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x0 = isleft ? edgepix : width_ - edgepix;
	const int x1 = isleft ? x0 - ticsz_ : x0 + ticsz_;
	uiLineItem* annotposlineitm = new uiLineItem();
	annotposlineitm->setLine( x0, pix, x1, pix );
	annotposlineitm->setZValue( 3 );
	Alignment al( isleft ? OD::AlignRight : OD::AlignLeft,
		      OD::AlignVCenter );
	uiTextItem* annotpostxtitem = new uiTextItem();
	annotpostxtitem->setText( txt );
	annotpostxtitem->setZValue( 3 );
	annotpostxtitem->setPos( x1, pix );
	annotpostxtitem->setAlignment( al );
	annotpostxtitem->setTextColor( ls.color_ );
	annotpostxtitemgrp_->add( annotpostxtitem );
    }
}


void uiAxisHandler::drawGridLine( int pix )
{
    const uiAxisHandler* hndlr = beghndlr_ ? beghndlr_ : endhndlr_;
    int endpix = setup_.border_.get( uiRect::across(setup_.side_) );
    if ( hndlr )
	endpix = setup_.side_ == uiRect::Left || setup_.side_ == uiRect::Bottom
	    	? hndlr->pixAfter() : hndlr->pixBefore();
    const int startpix = pixToEdge();

    uiLineItem* lineitem = new uiLineItem();
    switch ( setup_.side_ )
    {
    case uiRect::Top:
	lineitem->setLine( pix, startpix, pix, height_ - endpix );
	break;
    case uiRect::Bottom:
	lineitem->setLine( pix, endpix, pix, height_ - startpix );
	break;
    case uiRect::Left:
	lineitem->setLine( startpix, pix, width_ - endpix, pix );
	break;
    case uiRect::Right:
	lineitem->setLine( endpix, pix, width_ - startpix, pix );
	break;
    }
    lineitem->setZValue( 3 );
    lineitem->setPenStyle( setup_.style_ );
    gridlineitngrp_->add( lineitem );
}


void uiAxisHandler::drawName() 
{
    uiPoint pt;
    if ( !nameitem_ )
	nameitem_ = scene_->addText( name() );
    else
	nameitem_->setText( name() );
    nameitem_->setZValue( 3 );
    nameitem_->setTextColor( setup_.style_.color_ );
    if ( isHor() )
    {
	const bool istop = setup_.side_ == uiRect::Top;
	const int x = pixBefore() + axsz_ / 2;
	const int y = istop ? 2 : height_- 8;
	const Alignment al( OD::AlignHCenter,
			    istop ? OD::AlignBottom : OD::AlignTop );
	nameitem_->setPos( x, y );
	nameitem_->setAlignment( al );
    }
    else
    {
	const bool isleft = setup_.side_ == uiRect::Left;
	const int x = isleft ? pixBefore() - 3 : width_-3;
	const int y = height_ / 2;
	const Alignment al( isleft ? OD::AlignRight : OD::AlignLeft,
			    OD::AlignVCenter );
	nameitem_->setAlignment( al );
	nameitem_->setPos( x, y ); 
	if ( !ynmtxtvertical_ )
	    nameitem_->rotate( 90 );
	ynmtxtvertical_ = true;
    }
}
