/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Mahant Mothey / Bert
 Date:		September 2016 / Mar 2017
________________________________________________________________________

-*/

#include "seisstatscollector.h"

#include "seistrc.h"
#include "statrand.h"
#include "despiker.h"
#include "datadistributiontools.h"
#include "datadistributionextracter.h"

static const int cSampleBufferSize = 1048576;


SeisStatsCollector::SeisStatsCollector( int icomp )
    : vals_(0)
    , selcomp_(icomp)
{
    setEmpty();
}


SeisStatsCollector::~SeisStatsCollector()
{
    delete [] vals_;
}


void SeisStatsCollector::setEmpty()
{
    nrtrcshandled_ = nrvalshandled_ = totalnrsamples_ = 0;
    nrvalscollected_ = 0;
    mSetUdf( valrg_.start ); mSetUdf( valrg_.stop );
    delete [] vals_;
    mTryAlloc( vals_, float[cSampleBufferSize] );
    distrib_ = 0;
}


static void updateStepNr( int tksnr, int tknr, int& stepnr )
{
    const int absdiff = std::abs( tksnr - tknr );
    if ( absdiff < 1 )
	return;
    else if ( mIsUdf(tksnr) || absdiff < stepnr )
	stepnr = absdiff;
}


void SeisStatsCollector::addPosition( const TrcKey& tk,
				      const Interval<float>& zrg )
{
    if ( nrtrcshandled_ < 1 )
    {
	tkzs_.hsamp_ = TrcKeySampling( tk );
	tkzs_.hsamp_.step_.lineNr() = tkzs_.hsamp_.step_.trcNr() = mUdf(int);
	tkzs_.zsamp_.start = zrg.start;
	tkzs_.zsamp_.stop = zrg.stop;
    }
    else
    {
	tkzs_.hsamp_.include( tk );
	tkzs_.zsamp_.include( zrg.start, false );
	tkzs_.zsamp_.include( zrg.stop, false );
	updateStepNr( tkzs_.hsamp_.start_.lineNr(), tk.lineNr(),
		      tkzs_.hsamp_.step_.lineNr() );
	updateStepNr( tkzs_.hsamp_.stop_.lineNr(), tk.lineNr(),
		      tkzs_.hsamp_.step_.lineNr() );
	updateStepNr( tkzs_.hsamp_.start_.trcNr(), tk.trcNr(),
		      tkzs_.hsamp_.step_.trcNr() );
	updateStepNr( tkzs_.hsamp_.stop_.trcNr(), tk.trcNr(),
		      tkzs_.hsamp_.step_.trcNr() );
    }
    nrtrcshandled_++;
}


void SeisStatsCollector::useTrace( const SeisTrc& trc )
{
    if ( !vals_ )
	return;

    if ( nrtrcshandled_ < 1 )
	tkzs_.zsamp_.step = trc.stepPos();
    addPosition( trc.info().trckey_, trc.zRange() );

    const int sz = trc.size();
    for ( int icomp=0; icomp<trc.nrComponents(); icomp++ )
    {
	if ( selcomp_ >= 0 && icomp != selcomp_ )
	    continue;

	for ( int isamp=0; isamp<sz; isamp++ )
	{
	    const float val = trc.get( isamp, icomp );
	    if ( mIsUdf(val) )
		continue;

	    if ( nrvalshandled_ < 1 )
		valrg_.start = valrg_.stop = val;
	    else
		valrg_.include( val, false );

	    nrvalshandled_++;

	    if ( nrvalscollected_ < cSampleBufferSize )
	    {
		vals_[nrvalscollected_] = val;
		nrvalscollected_++;
	    }
	    else
	    {
		const od_int64 replidx
			= Stats::randGen().getIndex( nrvalshandled_ );
		if ( replidx < cSampleBufferSize )
		    vals_[replidx] = val;
	    }
	}

	totalnrsamples_ += sz;
    }
}


bool SeisStatsCollector::finish() const
{
    if ( distrib_ )
	return true;
    else if ( nrvalscollected_ < 1 )
	return false;

    SeisStatsCollector& self = *const_cast<SeisStatsCollector*>( this );

    DeSpiker<float,int> despiker( 10 );
    despiker.deSpike( self.vals_, nrvalscollected_ );

    RangeLimitedDataDistributionExtracter<float> extr( vals_, nrvalscollected_);
    // DataDistributionExtracter<float> extr( vals_, nrvalscollected_);
    self.distrib_ = extr.getDistribution();

    return true;
}


const SeisStatsCollector::DistribType& SeisStatsCollector::distribution() const
{
    return finish() ? *distrib_ : *new DistribType;
}


bool SeisStatsCollector::fillPar( IOPar& iop ) const
{
    if ( !finish() )
	return false;

    tkzs_.fillPar( iop );
    iop.set( "Count.Traces", nrtrcshandled_ );
    iop.set( "Count.Samples", totalnrsamples_ );
    iop.set( "Count.UnDefs", totalnrsamples_-nrvalshandled_ );
    iop.set( "Count.Random Sample Size", nrvalscollected_ );
    if ( !mIsUdf(valrg_.start) )
	iop.set( "Extremes", valrg_ );
    IOPar distribpar;
    DataDistributionInfoExtracter<float>(*distrib_).fillPar( distribpar );
    iop.mergeComp( distribpar, sKey::Distribution() );
    return true;
}


RefMan<SeisStatsCollector::DistribType> SeisStatsCollector::getDistribution(
						const IOPar& iop )
{
    RefMan<DistribType> ret;
    PtrMan<IOPar> distribpar = iop.subselect( sKey::Distribution() );
    if ( distribpar && !distribpar->isEmpty() )
    {
	ret = new DataDistribution<float>;
	DataDistributionChanger<float>(*ret).usePar( *distribpar );
    }
    return ret;
}


Interval<float> SeisStatsCollector::getExtremes( const IOPar& iop )
{
    Interval<float> rg = Interval<float>::udf();
    iop.get( "Extremes", rg );
    return rg;
}


od_int64 SeisStatsCollector::getNrSamples( const IOPar& iop, bool valid )
{
    od_int64 totalnr = 0; od_int64 nrinvalid = 0;
    iop.get( "Count.Samples", totalnr );
    iop.get( "Count.UnDefs", nrinvalid );
    return valid ? totalnr - nrinvalid : nrinvalid;
}


od_int64 SeisStatsCollector::getNrTraces( const IOPar& iop )
{
    od_int64 nrtrcs = 0;
    iop.get( "Count.Traces", nrtrcs );
    return nrtrcs;
}
