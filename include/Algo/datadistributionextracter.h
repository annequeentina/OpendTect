#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Mar 2017
________________________________________________________________________


-*/

#include "datadistribution.h"
#include "arraynd.h"
#include "valseries.h"
#include "paralleltask.h"


/*!\brief Extracts a data distribution from input data:
  * TypeSet or simply ptr + size
  * ArrayND (will try to work with the getData() pointer)
  * ValueSeries (will try to work with the arr() pointer)
*/

template <class vT>
mClass(Algo) DataDistributionExtracter : public ParallelTask
{
public:

    typedef ArrayND<vT>			ArrNDType;
    typedef DataDistribution<vT>	DistribType;
    typedef Interval<vT>		RangeType;

			DataDistributionExtracter( const ArrNDType& arr )
			    : arrnd_(&arr)
			    , vs_(0)
			    , totalsz_(arr.info().getTotalSz())
			    , arr_(arr.getData())	{ init(); }
			DataDistributionExtracter( const vT* arr, od_int64 sz )
			    : arr_(arr)
			    , vs_(0)
			    , totalsz_(sz)
			    , arrnd_(0)			{ init(); }
			DataDistributionExtracter( const ValueSeries<vT>& vs,
						   od_int64 sz )
			    : vs_(&vs)
			    , arr_(vs.arr())
			    , totalsz_(sz)
			    , arrnd_(0)			{ init(); }
			DataDistributionExtracter( const TypeSet<vT>& ts )
			    : DataDistributionExtracter(ts.arr(),ts.size())
							{}
    virtual		~DataDistributionExtracter()	{}

    RangeType		getDataRange() const;
    int			getDefNrBins() const;
    static SamplingData<vT> getSamplingFor(RangeType,int nrbins);

    void		setNrBins( int nr )		{ nrbins_ = nr; }
    void		setBounds( RangeType intv )	{ bounds_ = intv; }

    virtual od_int64	nrIterations() const		{ return totalsz_; }

    RefMan<DistribType>	getDistribution()		{ return distrib_; }
    void		reset()				{ init(); }


protected:

    const ArrayND<vT>*		arrnd_;
    const vT*			arr_;
    const ValueSeries<vT>*	vs_;
    const od_int64		totalsz_;
    RefMan<DistribType>		distrib_;
    int				nrbins_;
    RangeType			bounds_;

    virtual bool		doPrepare(int);
    virtual bool		doWork(od_int64,od_int64,int);

    void			init();
    void			determineBounds();
    static void			includeInRange(RangeType&,vT);
    void			putInBin(vT,TypeSet<vT>&,
					const SamplingData<vT>&,const int);

};


/*!\brief Does some work to limit the range of distribution extraction.

  The strategy is:
  - Create a distrib with a very fine bin size
  - Find 0.25% clipping points which will be the new min and max. Due to the
    fine bin size we can 'zoom in' without loss of detail.
  - Re-sample to the 'normal' bin size.

*/

template <class vT>
mClass(Algo) RangeLimitedDataDistributionExtracter
{
public:

    typedef ArrayND<vT>			ArrNDType;
    typedef DataDistribution<vT>	DistribType;
    typedef Interval<vT>		RangeType;

		RangeLimitedDataDistributionExtracter( const ArrNDType& arr,
						      TaskRunner* tskr=0 )
			    : extracter_(arr)		{ init(tskr); }
		RangeLimitedDataDistributionExtracter( const vT* a, od_int64 sz,
							TaskRunner* tskr=0 )
			    : extracter_(a,sz)		{ init(tskr); }
		RangeLimitedDataDistributionExtracter( const ValueSeries<vT>& v,
					   od_int64 sz, TaskRunner* tskr=0 )
			    : extracter_(v,sz)		{ init(tskr); }
		RangeLimitedDataDistributionExtracter( const TypeSet<vT>& ts,
						       TaskRunner* tskr=0 )
			    : extracter_(ts)		{ init(tskr); }
    virtual	~RangeLimitedDataDistributionExtracter()	{}

    RefMan<DistribType>	getDistribution()		{ return distrib_; }

protected:

    DataDistributionExtracter<vT> extracter_;
    RefMan<DistribType>		distrib_;

    void			init(TaskRunner*);
    bool			deSpike();

};



template <class vT> inline
void DataDistributionExtracter<vT>::init()
{
    distrib_ = new DistribType;
    nrbins_ = mUdf(int);
    bounds_ = RangeType( mUdf(vT), mUdf(vT) );
}


template <class vT> inline
void DataDistributionExtracter<vT>::includeInRange( RangeType& rg, vT val )
{
    if ( mIsUdf(val) )
	return;
    if ( mIsUdf(rg.start) || val < rg.start )
	rg.start = val;
    if ( mIsUdf(rg.stop) || val > rg.stop )
	rg.stop = val;
}


template <class vT> inline
Interval<vT> DataDistributionExtracter<vT>::getDataRange() const
{
    RangeType ret( mUdf(vT), mUdf(vT) );
    if ( arr_ )
    {
	const vT* stopptr = arr_ + totalsz_;
	for ( const vT* cur = arr_; cur != stopptr; cur++ )
	    includeInRange( ret, *cur );
    }
    else if ( vs_ )
    {
	for ( int idx=0; idx<totalsz_; idx++ )
	    includeInRange( ret, (*vs_)[idx] );
    }
    else
    {
	ArrayNDIter iter( arrnd_->info() );
	while ( iter.next() )
	    includeInRange( ret, arrnd_->getND( iter.getPos() ) );
    }
    return ret;
}


template <class vT> inline
int DataDistributionExtracter<vT>::getDefNrBins() const
{
    int ret = (int)(totalsz_ / 64);
    if ( ret < 8 )
	ret = 8;
    if ( ret > 1024 )
	ret = 1024;
    return ret;
}


template <class vT> inline SamplingData<vT>
DataDistributionExtracter<vT>::getSamplingFor( RangeType rg, int nrbins )
{
    SamplingData<vT> sd;
    sd.step = (rg.stop - rg.start) / nrbins;
    sd.start = rg.start + sd.step * vT(0.5);
    return sd;
}


template <class vT> inline
bool DataDistributionExtracter<vT>::doPrepare( int )
{
    if ( totalsz_ < 1 )
	return false;
    if ( !arrnd_ && !arr_ && !vs_ )
	{ pErrMsg("Duh"); return false; }

    if ( mIsUdf(nrbins_) )
	nrbins_ = getDefNrBins();

    if ( mIsUdf(bounds_.start) || mIsUdf(bounds_.stop) )
	determineBounds();

    if ( bounds_.start == bounds_.stop )
	bounds_.stop = bounds_.start + (vT)1;
    else if ( bounds_.start > bounds_.stop )
	std::swap( bounds_.start, bounds_.stop );

    SamplingData<vT> sd = getSamplingFor( bounds_, nrbins_ );
    distrib_ = new DistribType( sd, nrbins_ );

    return true;
}


template <class vT> inline
void DataDistributionExtracter<vT>::determineBounds()
{
    RangeType rg = getDataRange();

    if ( mIsUdf(bounds_.start) )
	bounds_.start = rg.start;
    if ( mIsUdf(bounds_.stop) )
	bounds_.stop = rg.stop;

    if ( mIsUdf(bounds_.start) || mIsUdf(bounds_.stop) )
	{ bounds_.start = vT(0); bounds_.stop = vT(1); }
}


template <class vT> inline
void DataDistributionExtracter<vT>::putInBin( vT val, TypeSet<vT>& subdistrib,
		    const SamplingData<vT>& sd, const int nrbins )
{
    if ( !mIsUdf(val) )
    {
	const int ibin
		= DataDistribution<vT>::getBinNrFor( val, sd, nrbins );
	subdistrib[ibin]++;
    }
}


template <class vT> inline
bool DataDistributionExtracter<vT>::doWork( od_int64 start, od_int64 stop, int )
{
    TypeSet<vT> subdistrib( distrib_->size(), 0 );
    const SamplingData<vT> sd = distrib_->sampling();
    const int nrbins = distrib_->size();
    if ( arr_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    putInBin( arr_[idx], subdistrib, sd, nrbins );
    }
    else if ( vs_ )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	    putInBin( (*vs_)[idx], subdistrib, sd, nrbins );
    }
    else
    {
	ArrayNDIter iter( arrnd_->info() );
	iter.setGlobalPos( start-1 );
	while ( iter.next() )
	    putInBin( arrnd_->getND(iter.getPos()), subdistrib, sd, nrbins );
    }

    distrib_->add( subdistrib.arr() );
    return true;
}


template <class vT> inline
void RangeLimitedDataDistributionExtracter<vT>::init( TaskRunner* tskr )
{
    const int targetnrbins = extracter_.getDefNrBins();
    extracter_.setNrBins( 32 * targetnrbins );
    extracter_.setBounds( extracter_.getDataRange() );
    if ( !TaskRunner::execute(tskr,extracter_) )
	{ distrib_ = new DistribType; return; }

    RefMan<DistribType> finedistr = extracter_.getDistribution();
    const vT sumvals  = finedistr->sumOfValues();
    const vT cutoffrelpos = vT(0.0025);
    const vT locutoff = cutoffrelpos * sumvals;
    const vT hicutoff = (1-cutoffrelpos) * sumvals;
    const RangeType posrg( finedistr->positionForCumulative( locutoff ),
			   finedistr->positionForCumulative( hicutoff ) );

    const SamplingData<vT> targetsd
		    = extracter_.getSamplingFor( posrg, targetnrbins );
    distrib_ = new DistribType( targetsd, targetnrbins );
    const int finesz = finedistr->size();
    const SamplingData<vT> finesd = finedistr->sampling();
    typename DistribType::SetType distrarr( targetnrbins, vT(0) );
    for ( int idx=0; idx<finesz; idx++ )
    {
	const int binnr = distrib_->getBinNr( finesd.atIndex(idx) );
	distrarr[binnr] += finedistr->get( idx );
    }
    distrib_->set( distrarr.arr() );
}