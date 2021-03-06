 /*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:		Feb 2007
________________________________________________________________________

-*/

#include "bitmapmgr.h"

#include "arrayndimpl.h"
#include "array2dbitmapimpl.h"
#include "flatposdata.h"
#include "flatview.h"


BitMapMgr::BitMapMgr()
    : datapack_(0)
    , appearance_(*new FlatView::Appearance)
    , bmp_(0)
    , pos_(0)
    , data_(0)
    , gen_(0)
    , wva_(false)
    , sz_(mUdf(int),mUdf(int))
    , wr_(mUdf(double),mUdf(double),mUdf(double),mUdf(double))
{
}


BitMapMgr::~BitMapMgr()
{
    delete &appearance_;
    clearAll();
}


void BitMapMgr::init( const FlatDataPack* fdp, const FlatView::Appearance& app,
		      bool wva )
{
    clearAll();

    datapack_ = fdp;

    appearance_ = app;
    wva_ = wva;
    setup();
}


void BitMapMgr::clearAll()
{
    Threads::Locker locker( lock_ );

    deleteAndZeroPtr( bmp_ );
    deleteAndZeroPtr( pos_ );
    deleteAndZeroPtr( data_ );
    deleteAndZeroPtr( gen_ );

    datapack_ = 0;
}


void BitMapMgr::setup()
{
    Threads::Locker locker( lock_ );

    if ( !datapack_ ) return;

    Monitorable::AccessLocker updlckr( *datapack_ );
    const FlatPosData& pd = datapack_->posData();
    const Array2D<float>& arr = datapack_->data();
    if ( pd.nrPts(true) < arr.info().getSize(0) )
	return;

    pos_ = new A2DBitMapPosSetup( arr.info(), pd.getPositions(true) );
    pos_->setDim1Positions( mCast(float,pd.range(false).start),
			    mCast(float,pd.range(false).stop) );
    data_ = new A2DBitMapInpData( arr );

    if ( !wva_ )
    {
	VDA2DBitMapGenerator* gen = new VDA2DBitMapGenerator( *data_, *pos_ );
	gen->linearInterpolate( appearance_.ddpars_.vd_.lininterp_ );
	gen_ = gen;
    }
    else
    {
	const FlatView::DataDispPars::WVA& wvapars = appearance_.ddpars_.wva_;
	WVAA2DBitMapGenerator* wvagen
			= new WVAA2DBitMapGenerator( *data_, *pos_ );
	wvagen->wvapars().drawwiggles_ = wvapars.wigg_.isVisible();
	wvagen->wvapars().drawrefline_ = wvapars.refline_.isVisible();
	wvagen->wvapars().filllow_ = wvapars.lowfill_.isVisible();
	wvagen->wvapars().fillhigh_ = wvapars.highfill_.isVisible();
	wvagen->wvapars().overlap_ = wvapars.overlap_;
	wvagen->wvapars().reflinevalue_ = wvapars.reflinevalue_;
	wvagen->wvapars().x1reversed_ = appearance_.annot_.x1_.reversed_;
	gen_ = wvagen;
    }

    const FlatView::DataDispPars::Common* pars = &appearance_.ddpars_.wva_;
    if ( !wva_ ) pars = &appearance_.ddpars_.vd_;

    gen_->pars().clipratio_ = pars->mapper_->setup().clipRate();
    gen_->pars().nointerpol_ = pars->blocky_;
    gen_->pars().scale_ = pars->mapper_->getRange();
    gen_->pars().autoscale_ = false;
}


Geom::Point2D<int> BitMapMgr::dataOffs(
			const Geom::PosRectangle<double>& inpwr,
			const Geom::Size2D<int>& inpsz ) const
{
    Threads::Locker locker( lock_ );

    Geom::Point2D<int> ret( mUdf(int), mUdf(int) );
    if ( mIsUdf(wr_.left()) ) return ret;

    // First see if we have different zooms:
    const Geom::Size2D<double> wrsz = wr_.size();
    const double xratio = wrsz.width() / sz_.width();
    const double yratio = wrsz.height() / sz_.height();
    const Geom::Size2D<double> inpwrsz = inpwr.size();
    const double inpxratio = inpwrsz.width() / inpsz.width();
    const double inpyratio = inpwrsz.height() / inpsz.height();
    if ( !mIsZero(xratio-inpxratio,mDefEps)
      || !mIsZero(yratio-inpyratio,mDefEps) )
	return ret;

    // Now check whether we have a pan outside buffered area:
    const bool xrev = wr_.right() < wr_.left();
    const bool yrev = wr_.top() < wr_.bottom();
    const double xoffs = (xrev ? inpwr.right() - wr_.right()
			       : inpwr.left() - wr_.left()) / xratio;
    const double yoffs = (yrev ? inpwr.top() - wr_.top()
			       : inpwr.bottom() - wr_.bottom()) / yratio;
    if ( xoffs <= -0.5 || yoffs <= -0.5 )
	return ret;
    const double maxxoffs = sz_.width() - inpsz.width() + .5;
    const double maxyoffs = sz_.height() - inpsz.height() + .5;
    if ( xoffs >= maxxoffs || yoffs >= maxyoffs )
	return ret;

    // No, we're cool. Return nearest integers
    ret.x_ = mNINT32(xoffs); ret.y_ = mNINT32(yoffs);
    return ret;
}


bool BitMapMgr::generate( const Geom::PosRectangle<double>& wr,
			  const Geom::Size2D<int>& sz,
			  const Geom::Size2D<int>& availsz )
{
    Threads::Locker locker( lock_ );
    if ( !gen_ )
    {
	setup();
	if ( !gen_ )
	    return true;
    }

    if ( !datapack_ )
	{ pErrMsg("Sanity check"); return true; }

    Monitorable::AccessLocker updlckr( *datapack_ );
    const FlatPosData& pd = datapack_->posData();
    pos_->setDimRange( 0, Interval<float>(
				mCast(float,wr.left()-pd.offset(true)),
				mCast(float,wr.right()-pd.offset(true))) );
    pos_->setDimRange( 1,
	Interval<float>(mCast(float,wr.bottom()),mCast(float,wr.top())) );

    bmp_ = new A2DBitMapImpl( sz.width(), sz.height() );
    if ( !bmp_ || !bmp_->isOK() || !bmp_->getData() )
    {
	delete bmp_; bmp_ = 0;
	updlckr.unlockNow();
	return false;
    }

    wr_ = wr; sz_ = sz;
    A2DBitMapGenerator::initBitMap( *bmp_ );
    gen_->setBitMap( *bmp_ );
    gen_->setPixSizes( availsz.width(), availsz.height() );

    if ( &datapack_->data() != &data_->data() )
    {
	pErrMsg("Sanity check failed");
	return false;
    }

    gen_->fill();
    return true;
}
