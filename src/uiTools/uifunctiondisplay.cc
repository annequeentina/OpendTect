/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Mar 2008
 RCS:           $Id: uifunctiondisplay.cc,v 1.19 2008-10-27 11:12:56 cvssatyaki Exp $
________________________________________________________________________

-*/

#include "uifunctiondisplay.h"
#include "uiaxishandler.h"
#include "uigraphicsscene.h"
#include "uigraphicsitemimpl.h"
#include "mouseevent.h"
#include "linear.h"

static const int cBoundarySz = 10;

uiFunctionDisplay::uiFunctionDisplay( uiParent* p,
				      const uiFunctionDisplay::Setup& su )
    : uiGraphicsView(p,"Function display viewer", su.handdrag_ )
    , setup_(su)
    , xax_(0)
    , yax_(0)
    , xmarkval_(mUdf(float))
    , ymarkval_(mUdf(float))
    , selpt_(0)
    , ypolyitem_(0)
    , y2polyitem_(0)
    , markeritems_(0)
    , pointSelected(this)
    , pointChanged(this)
    , mousedown_(this)
{
    setPrefWidth( setup_.canvaswidth_ );
    setPrefHeight( setup_.canvasheight_ );
    setStretch( 2, 2 );
    gatherInfo();
    //preDraw.notify( mCB(this,uiFunctionDisplay,gatherInfo) );
    uiAxisHandler::Setup asu( uiRect::Bottom, setup_.canvaswidth_,
	    		      setup_.canvasheight_ );
    asu.noannot( !setup_.annotx_ );
    asu.border_ = setup_.border_;
    xax_ = new uiAxisHandler( &scene(), asu );
    asu.side( uiRect::Left ).noannot( !setup_.annoty_ );
    yax_ = new uiAxisHandler( &scene(), asu );
    xax_->setBegin( yax_ ); yax_->setBegin( xax_ );
    asu.side( uiRect::Right ).noannot( !setup_.annoty_ );
    y2ax_ = new uiAxisHandler( &scene(), asu );

    if ( setup_.editable_ )
    {
	getMouseEventHandler().buttonPressed.notify(
				mCB(this,uiFunctionDisplay,mousePress) );
	getMouseEventHandler().buttonReleased.notify(
				mCB(this,uiFunctionDisplay,mouseRelease) );
	getMouseEventHandler().movement.notify(
				mCB(this,uiFunctionDisplay,mouseMove) );
	getMouseEventHandler().doubleClick.notify(
				mCB(this,uiFunctionDisplay,mouseDClick) );
    }
    reSize.notify( mCB(this,uiFunctionDisplay,reSized) );
    setScrollBar( false );
    draw();
}


uiFunctionDisplay::~uiFunctionDisplay()
{
    delete xax_;
    delete yax_;
    delete y2ax_;
    if ( ypolyitem_ ) scene().removeItem( ypolyitem_ );
    if ( y2polyitem_ ) scene().removeItem( y2polyitem_ );
    if ( markeritems_ ) scene().removeItem( markeritems_ );
}

void uiFunctionDisplay::reSized( CallBacker* )
{
    draw();
}


void uiFunctionDisplay::setVals( const float* xvals, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz < 2 ) return;

    for ( int idx=0; idx<sz; idx++ )
	{ xvals_ += xvals[idx]; yvals_ += yvals[idx]; }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setVals( const Interval<float>& xrg, const float* yvals,
				 int sz )
{
    xvals_.erase(); yvals_.erase();
    if ( sz < 2 ) return;

    const float dx = (xrg.stop-xrg.start) / (sz-1);
    for ( int idx=0; idx<sz; idx++ )
	{ xvals_ += xrg.start + idx * dx; yvals_ += yvals[idx]; }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setY2Vals( const float* xvals, const float* yvals,
				   int sz )
{
    y2xvals_.erase(); y2yvals_.erase();
    if ( sz < 2 ) return;

    for ( int idx=0; idx<sz; idx++ )
    {
	y2xvals_ += xvals[idx];
	y2yvals_ += yvals[idx];
    }

    gatherInfo(); draw();
}


void uiFunctionDisplay::setMarkValue( float val, bool is_x )
{
    (is_x ? xmarkval_ : ymarkval_) = val;
}


#define mSetRange( axis, rg ) \
    axlyo.setDataRange( rg ); rg.step = axlyo.sd.step; \
    if ( !mIsEqual(rg.start,axlyo.sd.start,axlyo.sd.step*1e-6) ) \
	axlyo.sd.start += axlyo.sd.step; \
    axis->setRange( rg, &axlyo.sd.start );


void uiFunctionDisplay::gatherInfo()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();
    if ( havey2 )
	{ xax_->setEnd( y2ax_ ); y2ax_->setBegin( xax_ ); }

    StepInterval<float> xrg, yrg;
    getRanges( xvals_, yvals_, setup_.xrg_, setup_.yrg_, xrg, yrg );

    AxisLayout axlyo;
    mSetRange( xax_, xrg );
    mSetRange( yax_, yrg );

    if ( havey2 )
    {
	getRanges( y2xvals_, y2yvals_, setup_.xrg_, setup_.y2rg_, xrg, yrg );
	mSetRange( y2ax_, yrg );
    }
}


void uiFunctionDisplay::getRanges(
	const TypeSet<float>& xvals, const TypeSet<float>& yvals,
	const Interval<float>& setupxrg, const Interval<float>& setupyrg,
	StepInterval<float>& xrg, StepInterval<float>& yrg ) const
{
    for ( int idx=0; idx<xvals.size(); idx++ )
    {
	if ( idx == 0 )
	{
	    xrg.start = xrg.stop = xvals[0];
	    yrg.start = yrg.stop = yvals[0];
	}
	else
	{
	    if ( xvals[idx] < xrg.start ) xrg.start = xvals[idx];
	    if ( yvals[idx] < yrg.start ) yrg.start = yvals[idx];
	    if ( xvals[idx] > xrg.stop ) xrg.stop = xvals[idx];
	    if ( yvals[idx] > yrg.stop ) yrg.stop = yvals[idx];
	}
    }

    if ( !mIsUdf(setupxrg.start) ) xrg.start = setupxrg.start;
    if ( !mIsUdf(setupyrg.start) ) yrg.start = setupyrg.start;
    if ( !mIsUdf(setupxrg.stop) ) xrg.stop = setupxrg.stop;
    if ( !mIsUdf(setupyrg.stop) ) yrg.stop = setupyrg.stop;
}


void uiFunctionDisplay::draw()
{
    if ( yvals_.isEmpty() ) return;
    const bool havey2 = !y2xvals_.isEmpty();

    xax_->setNewDevSize( width(), height() );
    yax_->setNewDevSize( height(), width() );
    if ( havey2 ) y2ax_->setNewDevSize( height(), width() );

    if ( setup_.annotx_ )
	xax_->plotAxis();
    if ( setup_.annoty_ )
    {
	yax_->plotAxis();
	if ( havey2 )
	    y2ax_->plotAxis();
    }

    TypeSet<uiPoint> yptlist, y2ptlist;
    const int nrpts = size();
    const uiPoint closept( xax_->getPix(xax_->range().start),
	    		   yax_->getPix(yax_->range().start) );
    if ( setup_.fillbelow_ )
	yptlist += closept;

    for ( int idx=0; idx<nrpts; idx++ )
    {
	const int xpix = xax_->getPix( xvals_[idx] );
	const uiPoint pt( xpix, yax_->getPix(yvals_[idx]) );
	yptlist += pt;
	if ( setup_.fillbelow_ && idx == nrpts-1 )
	{
	    yptlist += uiPoint( pt.x, closept.y );
	    yptlist += closept;
	}

    }

    if ( havey2 )
    {
	if ( setup_.fillbelowy2_ )
	    y2ptlist += closept;
	Interval<int> xpixintv( xax_->getPix(xax_->range().start),
				xax_->getPix(xax_->range().stop) );
	for ( int idx=0; idx<y2xvals_.size(); idx++ )
	{
	    const int xpix = xax_->getPix( y2xvals_[idx] );
	    if ( xpixintv.includes(xpix) )
		y2ptlist += uiPoint( xpix, y2ax_->getPix(y2yvals_[idx]) );
	}
	if ( setup_.fillbelowy2_ )
	    y2ptlist += uiPoint( xpixintv.stop, closept.y );
    }

    if ( havey2 )
    {
	if ( !y2polyitem_ )
	    y2polyitem_ = scene().addPolygon( y2ptlist, setup_.fillbelowy2_ );
	else
	    y2polyitem_->setPolygon( y2ptlist );
	y2polyitem_->setPenColor( setup_.y2col_ );
	y2polyitem_->setFillColor( setup_.fillbelowy2_ ? setup_.y2col_ :
						        Color::NoColor );
	y2polyitem_->setZValue( 0 );
    }

    if ( !ypolyitem_ )
	ypolyitem_ = scene().addPolygon( yptlist, setup_.fillbelowy2_ );
    else
	ypolyitem_->setPolygon( yptlist );
    ypolyitem_->setPenColor( setup_.ycol_ );
    if ( setup_.fillbelow_ )
	ypolyitem_->setFillColor( setup_.ycol_ );
    ypolyitem_->setZValue( 1 );
    if ( setup_.pointsz_ > 0 )
    {
	if ( !markeritems_ )
	{
	    markeritems_ = new uiGraphicsItemGroup();
	    scene().addItemGrp( markeritems_ );
	}
	else
	    markeritems_->removeAll( true );

	const MarkerStyle2D mst( MarkerStyle2D::Square, setup_.pointsz_,
				 setup_.ycol_ );
	for ( int idx=0; idx<nrpts; idx++ )
	{
	    const int xpix = xax_->getPix( xvals_[idx] );
	    const uiPoint pt( xpix, yax_->getPix(yvals_[idx]) );
	    uiMarkerItem* markeritem = new uiMarkerItem( mst );
	    markeritem->setPos( pt.x, pt.y );
	    markeritems_->add( markeritem );
	}
	markeritems_->setZValue( 1 );
    }

    LineStyle ls;
    if ( !mIsUdf(xmarkval_) )
    {
	ls.color_ = setup_.xmarkcol_;
	xax_->setup().style_ = ls;
	xax_->drawGridLine( xax_->getPix(xmarkval_) );
    }
    if ( !mIsUdf(ymarkval_) )
    {
	ls.color_ = setup_.ymarkcol_;
	yax_->setup().style_ = ls;
	yax_->drawGridLine( yax_->getPix(ymarkval_) );
    }
    setViewArea( 0, 5, width()+10, height()+10 );
}


#define mGetMousePos()  \
    if ( getMouseEventHandler().isHandled() ) \
	return; \
    const MouseEvent& ev = getMouseEventHandler().event(); \
    if ( !(ev.buttonState() & OD::LeftButton ) || \
	  (ev.buttonState() & OD::MidButton ) || \
	  (ev.buttonState() & OD::RightButton ) ) \
        return; \
    const bool isctrl = ev.ctrlStatus(); \
    const bool isoth = ev.shiftStatus() || ev.altStatus(); \
    const bool isnorm = !isctrl && !isoth


bool uiFunctionDisplay::setSelPt()
{
    const MouseEvent& ev = getMouseEventHandler().event();

    int newsel = -1; float mindistsq = 1e30;
    const float xpix = xax_->getRelPos( xax_->getVal(ev.pos().x) );
    const float ypix = yax_->getRelPos( yax_->getVal(ev.pos().y) );
    for ( int idx=0; idx<xvals_.size(); idx++ )
    {
	const float x = xax_->getRelPos( xvals_[idx] );
	const float y = yax_->getRelPos( yvals_[idx] );
	const float distsq = (x-xpix)*(x-xpix) + (y-ypix)*(y-ypix);
	if ( distsq < mindistsq )
	    { newsel = idx; mindistsq = distsq; }
    }
    selpt_ = -1;
    if ( mindistsq > setup_.ptsnaptol_*setup_.ptsnaptol_ ) return false;
    selpt_ = newsel;
    return true;
}


void uiFunctionDisplay::mousePress( CallBacker* )
{
    if ( mousedown_ ) return; mousedown_ = true;
    mGetMousePos();
    if ( isoth || !setSelPt() ) return;

    if ( isnorm )
	pointSelected.trigger();
}


void uiFunctionDisplay::mouseRelease( CallBacker* )
{
    if ( !mousedown_ ) return; mousedown_ = false;
    mGetMousePos();
    if ( !isctrl || selpt_ < 0 || xvals_.size() < 3 ) return;

    xvals_.remove( selpt_ );
    yvals_.remove( selpt_ );

    selpt_ = -1;
    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseMove( CallBacker* )
{
    if ( !mousedown_ ) return;
    mGetMousePos();
    if ( !isnorm || selpt_ < 0 ) return;

    float xval = xax_->getVal( ev.pos().x );
    float yval = yax_->getVal( ev.pos().y );

    if ( selpt_ > 0 && xvals_[selpt_-1] >= xval )
	return;
    if ( selpt_ < xvals_.size() - 1 && xvals_[selpt_+1] <= xval )
	return;

    if ( xval > xax_->range().stop )
	xval = xax_->range().stop;
    else if ( xval < xax_->range().start )
	xval = xax_->range().start;

    if ( yval > yax_->range().stop )
	yval = yax_->range().stop;
    else if ( yval < yax_->range().start )
	yval = yax_->range().start;

    xvals_[selpt_] = xval; yvals_[selpt_] = yval;

    pointChanged.trigger();
    draw();
}


void uiFunctionDisplay::mouseDClick( CallBacker* )
{
    mousedown_ = false;
    mGetMousePos();
    if ( !isnorm ) return;

    const float xval = xax_->getVal(ev.pos().x);
    const float yval = yax_->getVal(ev.pos().y);
    if ( xval > xvals_[xvals_.size()-1] )
    {
	xvals_ += xval; yvals_ += yval;
	selpt_ = xvals_.size()-1;
    }
    else
    {
	for ( int idx=0; idx<xvals_.size(); idx++ )
	{
	    if ( xval > xvals_[idx] )
		continue;

	    if ( xval == xvals_[idx] )
		yvals_[idx] = yval;
	    else
	    {
		xvals_.insert( idx, xval );
		yvals_.insert( idx, yval );
	    }

	    selpt_ = idx;
	    break;
	}
    }

    pointSelected.trigger();
    draw();
}
