/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2007
________________________________________________________________________

-*/

#include "uiflatauxdatadisplay.h"

#include "uigraphicsitemimpl.h"
#include "uimain.h"
#include "uiflatviewer.h"

namespace FlatView
{


AuxData* uiAuxDataDisplay::clone() const
{ return new uiAuxDataDisplay( *this ); }


uiAuxDataDisplay::~uiAuxDataDisplay()
{
    if ( viewer_ ) viewer_->viewChanged.remove(
	    mCB(this,uiAuxDataDisplay,updateTransformCB) );
}


#define mImplConstructor( arg ) \
    : AuxData( arg ) \
    , display_( 0 ) \
    , polygonitem_( 0 ) \
    , polylineitem_( 0 ) \
    , nameitem_( 0 ) \
    , viewer_( 0 )


uiAuxDataDisplay::uiAuxDataDisplay( const char* nm )
    mImplConstructor( nm )
{}


uiAuxDataDisplay::uiAuxDataDisplay( const uiAuxDataDisplay& a )
    mImplConstructor( a )
{}


uiGraphicsItemGroup* uiAuxDataDisplay::getDisplay()
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Attempt to update GUI in non ui-thread");
	return 0;
    }

    if ( !display_ )
	display_ = new uiGraphicsItemGroup( true );

    return display_;
}


void uiAuxDataDisplay::removeDisplay()
{
    display_ = 0;
    removeItems();
}


void uiAuxDataDisplay::removeItems()
{
    polygonitem_ = 0; polylineitem_ = 0; nameitem_ = 0;
    markeritems_.erase();
}


void uiAuxDataDisplay::updateCB( CallBacker* cb )
{
    if ( !isMainThreadCurrent() )
    {
	pErrMsg("Attempt to update GUI in non ui-thread");
	return;
    }

    if ( !getDisplay() ) return;

    if ( !enabled_ || poly_.isEmpty() )
    {
	display_->removeAll( true );
	removeItems();
	return;
    }

    display_->setVisible( turnon_ );
    if ( !display_->isVisible() )
	return;

    display_->setZValue( zvalue_ );

    if ( x1rg_ || x2rg_ || fitnameinview_ )
    {
	viewer_->viewChanged.notifyIfNotNotified(
	    mCB(this,uiAuxDataDisplay,updateTransformCB) );

	updateTransformCB(0);
    }

    const bool drawfill = close_ && fillcolor_.isVisible();
    if ( (linestyle_.isVisible() || drawfill) && poly_.size()>1 )
    {
	uiGraphicsItem* item = 0;

	if ( close_ )
	{
	    if ( !polygonitem_ )
	    {
		polygonitem_ = new uiPolygonItem( poly_,fillcolor_.isVisible());
		display_->add( polygonitem_ );
	    }
	    else
	    {
		polygonitem_->setPolygon( poly_ );
	    }

	    polygonitem_->setFillColor( fillcolor_, true );
	    polygonitem_->setFillPattern( fillpattern_ );
	    item = polygonitem_;

	    if ( polylineitem_ )
	    {
		display_->remove( polylineitem_, true );
		polylineitem_ = 0;
	    }
	}
	else
	{
	    if ( !polylineitem_ )
	    {
		polylineitem_ = new uiPolyLineItem( poly_ );
		display_->add( polylineitem_ );
	    }
	    else
	    {
		if ( needsupdatelines_ )
		    polylineitem_->setPolyLine( poly_ );
	    }

	    item = polylineitem_;

	    if ( polygonitem_ )
	    {
		display_->remove( polygonitem_, true );
		polygonitem_ = 0;
	    }
	}

	item->setPenStyle( linestyle_, true );
	//item->setCursor( cursor_ );
    }
    else
    {
	if ( polygonitem_ )
	{
	    display_->remove( polygonitem_, true );
	    polygonitem_ = 0;
	}

	if ( polylineitem_ )
	{
	    display_->remove( polylineitem_, true );
	    polylineitem_ = 0;
	}
    }

    const int nrmarkerstyles = markerstyles_.size();
    while ( markeritems_.size() > poly_.size() )
	display_->remove( markeritems_.pop(), true );

    for ( int idx=0; idx<poly_.size(); idx++ )
    {
	const int styleidx = mMIN(idx,nrmarkerstyles-1);
	if ( styleidx < 0 )
	{
	    if ( idx < markeritems_.size() )
		markeritems_[idx]->setVisible( false );
	    continue;
	}

	const OD::MarkerStyle2D& style = markerstyles_[styleidx];
	if ( !style.isVisible() )
	    continue;

	uiMarkerItem* item;
	if ( idx>=markeritems_.size() )
	{
	    item = new uiMarkerItem( style );
	    markeritems_ += item;
	    display_->add( item );
	}
	else
	    item = markeritems_[idx];

	item->setMarkerStyle( style );
	item->setRotation( style.rotation_ );
	item->setPenColor( style.color_, true );
	item->setFillColor( style.color_ );
	item->setPos( poly_[idx] );
	item->setVisible( true );
	//item->setCursor( cursor_ );
    }

    if ( !name_.isEmpty() && !mIsUdf(namepos_) )
    {
	int listpos = namepos_;
	if ( listpos < 0 ) listpos=0;
	if ( listpos > poly_.size() ) listpos = poly_.size()-1;

	if ( !nameitem_ )
	{
	    nameitem_ = new uiTextItem;
	    display_->add( nameitem_ );
	}
	nameitem_->setText( toUiString(name_) );
	nameitem_->setAlignment( namealignment_ );

	nameitem_->setTextColor( linestyle_.color_ );
	if ( poly_.size() > listpos )
	    nameitem_->setPos( poly_[listpos] );
    }
    else if ( nameitem_ )
    {
	display_->remove( nameitem_, true );
	nameitem_ = 0;
    }
}


void uiAuxDataDisplay::updateTransformCB( CallBacker* cb )
{
    //The aux-data is sitting in the viewer's world space.
    //If we have own axises, we need to set the transform
    //so the local space transforms to the worlds.

    double xpos = 0, ypos = 0, xscale = 1, yscale = 1;
    const uiWorldRect& curview = viewer_->curView();

    if ( fitnameinview_ && nameitem_ )
    {
	/*xscale = curview.width()/x1rg_->width();
	xpos = curview.left()-xscale*x1rg_->start;*/
	int listpos = namepos_;
	if ( listpos < 0 ) listpos=0;
	if ( listpos > poly_.size() ) listpos = poly_.size()-1;
	FlatView::Point modnamepos = poly_[listpos];
	modnamepos = curview.moveInside( modnamepos );
	nameitem_->setPos( modnamepos );
    }

    if ( x1rg_ || x2rg_ )
    {
	if ( x1rg_ )
	{
	    xscale = curview.width()/x1rg_->width();
	    xpos = curview.left()-xscale*x1rg_->start;
	}
	if( x2rg_ )
	{
	    yscale = curview.height()/x2rg_->width();
	    ypos = curview.top()-yscale*x2rg_->start;
	}
	display_->setPos( (float) xpos, (float) ypos );
	display_->setScale( (float) xscale, (float) yscale );
    }
}


} //namespace
