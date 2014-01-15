/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiflatviewcontrol.h"
#include "flatviewzoommgr.h"
#include "mouseevent.h"
#include "uiflatviewer.h"
#include "uiflatviewpropdlg.h"
#include "uirgbarraycanvas.h"
#include "uigraphicsscene.h"
#include "uiworld2ui.h"
#include "bufstringset.h"
#include "datapackbase.h"
#include "uiobjdisposer.h"


uiFlatViewControl::uiFlatViewControl( uiFlatViewer& vwr, uiParent* p, 
				bool rub, bool withhanddrag )
    : uiGroup(p ? p : vwr.attachObj()->parent(),"Flat viewer control")
    , zoommgr_(*new FlatView::ZoomMgr)
    , haverubber_(rub)
    , withhanddrag_(withhanddrag)
    , propdlg_(0)
    , infoChanged(this)
    , viewerAdded(this)
    , zoomChanged(this)
{
    setBorder( 0 );
    addViewer( vwr );
    if ( vwr.attachObj()->parent() )
	mAttachCB( vwr.attachObj()->parent()->postFinalise(),
			uiFlatViewControl::onFinalise );
    mAttachCB( viewerAdded, uiFlatViewControl::vwrAdded );
}


uiFlatViewControl::~uiFlatViewControl()
{
    detachAllNotifiers();
    delete &zoommgr_;
    delete propdlg_;
    propdlg_ = 0;
}


void uiFlatViewControl::addViewer( uiFlatViewer& vwr )
{
    vwrs_ += &vwr;
    vwr.control_ = this;
    zoommgr_.setNrViewers( vwrs_.size() );
    mAttachCB( vwr.dataChanged, uiFlatViewControl::dataChangeCB );

    if ( haverubber_ )
	mAttachCB(vwr.rgbCanvas().rubberBandUsed,uiFlatViewControl::rubBandCB);

    MouseEventHandler& mevh = mouseEventHandler( vwrs_.size()-1, true );
    mAttachCB( mevh.movement, uiFlatViewControl::mouseMoveCB );
    mAttachCB( mevh.buttonReleased, uiFlatViewControl::usrClickCB );
    mAttachCB( vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
		uiFlatViewControl::keyPressCB );

    viewerAdded.trigger();
}


void uiFlatViewControl::removeViewer( uiFlatViewer& vwr )
{
    vwrs_ -= &vwr;
    zoommgr_.setNrViewers( !vwrs_.size() ? 1 : vwrs_.size() );
    mDetachCB( vwr.dataChanged, uiFlatViewControl::dataChangeCB );

    uiGraphicsView& cnvs = vwr.rgbCanvas();
    if ( haverubber_ )
	mDetachCB( cnvs.rubberBandUsed, uiFlatViewControl::rubBandCB );

    MouseEventHandler& mevh = cnvs.scene().getMouseEventHandler();
    mDetachCB( mevh.movement, uiFlatViewControl::mouseMoveCB );
    mDetachCB( mevh.buttonReleased, uiFlatViewControl::usrClickCB );
    mDetachCB( vwr.rgbCanvas().getKeyboardEventHandler().keyPressed,
		uiFlatViewControl::keyPressCB );
}


TypeSet<uiWorldRect> uiFlatViewControl::getBoundingBoxes() const
{
    TypeSet<uiWorldRect> wrs;

    for ( int ivwr=0; ivwr<vwrs_.size(); ivwr++ )
	wrs += vwrs_[ivwr]->boundingBox();

    return wrs;
}


void uiFlatViewControl::onFinalise( CallBacker* )
{
    const bool canreuse = zoommgr_.current().width() > 10
			 && canReUseZoomSettings( vwrs_[0]->curView().centre(),
						  zoommgr_.current() );
    if ( !canreuse )
    {
	zoommgr_.init( getBoundingBoxes() );
    }

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	if ( !canreuse )
	    vwrs_[idx]->setViewToBoundingBox();
	else
    	    vwrs_[idx]->setView( vwrs_[idx]->curView() );
    }

    finalPrepare();
}


void uiFlatViewControl::dataChangeCB( CallBacker* cb )
{
    zoommgr_.reInit( getBoundingBoxes() );
    zoomChanged.trigger();
}


bool uiFlatViewControl::havePan(
	Geom::Point2D<double> oldcentre, Geom::Point2D<double> newcentre,
	Geom::Size2D<double> sz )
{
    const Geom::Point2D<double> eps( sz.width()*1e-6, sz.height()*1e-6 );
    return !mIsZero(oldcentre.x-newcentre.x,eps.x)
	|| !mIsZero(oldcentre.y-newcentre.y,eps.y);
}


bool uiFlatViewControl::haveZoom( Geom::Size2D<double> oldsz,
				  Geom::Size2D<double> newsz )
{
    const Geom::Point2D<double> eps( oldsz.width()*1e-6, oldsz.height()*1e-6 );
    return !mIsZero(oldsz.width()-newsz.width(),eps.x)
	|| !mIsZero(oldsz.height()-newsz.height(),eps.y);
}


uiWorldRect uiFlatViewControl::getNewWorldRect( Geom::Point2D<double>& centre,
						Geom::Size2D<double>& sz,
						const uiWorldRect& cv,
						const uiWorldRect& bb ) const
{
    const bool havepan = havePan( cv.centre(), centre, cv.size() );
    const bool havezoom = haveZoom( cv.size(), sz );
    if ( !havepan && !havezoom ) return cv;

    uiWorldRect wr( havepan && havezoom ? getZoomAndPanRect(centre,sz,bb)
	    				: getZoomOrPanRect(centre,sz,bb) );
    centre = wr.centre(); sz = wr.size();

    if ( cv.left() > cv.right() ) wr.swapHor();
    if ( cv.bottom() > cv.top() ) wr.swapVer();
    return wr;
}


uiTabStackDlg* uiFlatViewControl::propDialog()
{ return propdlg_; }


void uiFlatViewControl::setNewView( Geom::Point2D<double>& centre,
				    Geom::Size2D<double>& sz )
{
    uiWorldRect br = vwrs_[0]->boundingBox();
    br.sortCorners();
    const uiWorldRect wr = getNewWorldRect( centre,sz, vwrs_[0]->curView(),br );
    vwrs_[0]->setView( wr );
    zoommgr_.add( sz );

    zoomChanged.trigger();
}


uiWorldRect uiFlatViewControl::getZoomAndPanRect( Geom::Point2D<double> centre,
						Geom::Size2D<double> sz,
       						const uiWorldRect& bbox	)
{
    //TODO we should have a different policy for requests outside
    return getZoomOrPanRect( centre, sz, bbox );
}


uiWorldRect uiFlatViewControl::getZoomOrPanRect( Geom::Point2D<double> centre,
						 Geom::Size2D<double> sz,
						 const uiWorldRect& inputbb )
{
    uiWorldRect bb( inputbb );
    if ( bb.left() > bb.right() ) bb.swapHor();
    if ( bb.bottom() > bb.top() ) bb.swapVer();

    if ( sz.width() > bb.width() )
	sz.setWidth( bb.width() );
    if ( sz.height() > bb.height() )
	sz.setHeight( bb.height() );
    const double hwdth = sz.width() * .5;
    const double hhght = sz.height() * .5;

    if ( centre.x - hwdth < bb.left() )		centre.x = bb.left() + hwdth;
    if ( centre.y - hhght < bb.bottom() )	centre.y = bb.bottom() + hhght;
    if ( centre.x + hwdth > bb.right() )	centre.x = bb.right() - hwdth;
    if ( centre.y + hhght > bb.top() )		centre.y = bb.top() - hhght;

    return uiWorldRect( centre.x - hwdth, centre.y + hhght,
			centre.x + hwdth, centre.y - hhght );
}


void uiFlatViewControl::flip( bool hor )
{
    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	FlatView::Annotation::AxisData& ad
			    = hor ? vwrs_[idx]->appearance().annot_.x1_
				  : vwrs_[idx]->appearance().annot_.x2_;
	ad.reversed_ = !ad.reversed_;
	vwrs_[idx]->handleChange( FlatView::Viewer::Annot );
    }
}


void uiFlatViewControl::rubBandCB( CallBacker* cb )
{
    //TODO handle when zoom is disabled
    const uiRect* selarea = vwrs_[0]->rgbCanvas().getSelectedArea();
    if ( !selarea || (selarea->topLeft() == selarea->bottomRight()) ||
	 (selarea->width()<5 && selarea->height()<5) )
	return;

    uiWorld2Ui w2u;
    vwrs_[0]->getWorld2Ui(w2u);
    uiWorldRect wr = w2u.transform(*selarea);
    Geom::Point2D<double> centre = wr.centre();
    Geom::Size2D<double> newsz = wr.size();

    setNewView( centre, newsz );
}


void uiFlatViewControl::doPropertiesDialog( int vieweridx )
{
    //TODO what if more than one viewer? Also functions below
    uiFlatViewer& vwr = *vwrs_[vieweridx];
    BufferStringSet annots;
    const int selannot = vwr.getAnnotChoices( annots ); 

    if ( !propdlg_ || &propdlg_->viewer()!=&vwr )
    {
	delete propdlg_;
    	propdlg_ = new uiFlatViewPropDlg( 0, vwr,
				mCB(this,uiFlatViewControl,applyProperties),
				annots.size() ? &annots : 0, selannot );
    	mAttachCB( propdlg_->windowClosed, uiFlatViewControl::propDlgClosed );
    }

    propdlg_->show();
    propdlg_->raise();
}


void uiFlatViewControl::propDlgClosed( CallBacker* )
{
    if ( propdlg_->uiResult() == 1 )
    {
	applyProperties(0);
	if ( propdlg_->saveButtonChecked() )
	    saveProperties( propdlg_->viewer() );
    }
}


void uiFlatViewControl::applyProperties( CallBacker* cb )
{
    if ( !propdlg_ ) return;

    mDynamicCastGet( uiFlatViewer*, vwr, &propdlg_->viewer() );
    if ( !vwr ) return;

    const int selannot = propdlg_->selectedAnnot();
    vwr->setAnnotChoice( selannot );
    vwr->handleChange( FlatView::Viewer::Annot |
	    	       FlatView::Viewer::DisplayPars );
    vwr->dispPropChanged.trigger();
}


void uiFlatViewControl::saveProperties( FlatView::Viewer& vwr )
{
    const FlatDataPack* fdp = vwr.pack( true );
    if ( !fdp ) fdp = vwr.pack( false );

    BufferString cat( "General" );

    if ( fdp )
    {
	cat = fdp->category();
	if ( category_.isEmpty() )
	    category_ = cat;
    }

    vwr.storeDefaults( cat );
}


MouseEventHandler& uiFlatViewControl::mouseEventHandler( int idx, bool ofscene )
{
    uiGraphicsView& canvas = vwrs_[idx]->rgbCanvas();
    return ofscene ? canvas.scene().getMouseEventHandler()
		   : canvas.getNavigationMouseEventHandler();
}


int uiFlatViewControl::getViewerIdx( const MouseEventHandler* meh,bool ofscene )
{
    if ( !meh ) return -1;

    for ( int idx=0; idx<vwrs_.size(); idx++ )
    {
	const MouseEventHandler* imeh = &mouseEventHandler( idx, ofscene );
	if ( imeh==meh && imeh->hasEvent() ) return idx;
    }

    return -1;
}


uiRect uiFlatViewControl::getViewRect( const uiFlatViewer* vwr )
{ return vwr->getViewRect(); }


void uiFlatViewControl::addSizesToZoomMgr()
{
    TypeSet<FlatView::ZoomMgr::Size> sizes;
    
    for ( int idx=0; idx<vwrs_.size(); idx++ )
	sizes += vwrs_[idx]->curView().size();
    
    zoommgr_.add( sizes );
}


void uiFlatViewControl::mouseMoveCB( CallBacker* cb )
{
    mDynamicCastGet( const MouseEventHandler*, meh, cb );
    if ( !meh || !meh->hasEvent() ) return;

    const int idx = getViewerIdx( meh, true );
    if ( idx<0 ) return;
    uiWorld2Ui w2u;
    vwrs_[idx]->getWorld2Ui(w2u);
    const uiWorldPoint wp = w2u.transform( meh->event().pos() );
    vwrs_[idx]->getAuxInfo( wp, infopars_ );
    CBCapsule<IOPar> caps( infopars_, this );
    infoChanged.trigger( &caps );
}


void uiFlatViewControl::usrClickCB( CallBacker* cb )
{
    mDynamicCastGet(MouseEventHandler*,meh,cb);
    if ( !meh || !meh->hasEvent() || meh->isHandled() )
	return;

    meh->setHandled( handleUserClick(getViewerIdx(meh,true)) );
}


bool uiFlatViewControl::canReUseZoomSettings( Geom::Point2D<double> centre,
					      Geom::Size2D<double> sz ) const
{
    //TODO: allow user to decide to reuse or not with a specific parameter
    const uiWorldRect bb( vwrs_[0]->boundingBox() );
    if ( sz.width() > bb.width() || sz.height() > bb.height() )
	return false;
    
    const double hwdth = sz.width() * .5;
    const double hhght = sz.height() * .5;

    Geom::Point2D<double> topleft( centre.x - hwdth, centre.y - hhght );
    Geom::Point2D<double> botright( centre.x + hwdth, centre.y + hhght );
    if ( ! ( bb.contains( topleft, 1e-6 ) && bb.contains( botright, 1e-6 ) ) )
	return false;

    return true;
}
