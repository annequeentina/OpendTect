/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : April 2005
-*/


#include "prestackgather.h"

#include "genericnumer.h"
#include "flatposdata.h"
#include "ioobj.h"
#include "ioobjtags.h"
#include "iopar.h"
#include "dbman.h"
#include "ptrman.h"
#include "samplfunc.h"
#include "seisbuf.h"
#include "seistrc.h"
#include "seispsioprov.h"
#include "seispsread.h"
#include "seistrctr.h"
#include "seistrcprop.h"
#include "survinfo.h"
#include "survgeom.h"
#include "unitofmeasure.h"
#include "keystrs.h"

const char* Gather::sDataPackCategory()		{ return "Pre-Stack Gather"; }
const char* Gather::sKeyIsAngleGather()		{ return "Angle Gather"; }
const char* Gather::sKeyIsCorr()		{ return "Is Corrected"; }
const char* Gather::sKeyZisTime()		{ return "Z Is Time"; }
const char* Gather::sKeyPostStackDataID()	{ return "Post Stack Data"; }
const char* Gather::sKeyStaticsID()		{ return "Statics"; }

Gather::Gather()
    : FlatDataPack( sDataPackCategory(), new Array2DImpl<float>(1,1) )
    , offsetisangle_( false )
    , iscorr_( false )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{
}


Gather::Gather( const Gather& oth )
    : FlatDataPack( oth )
{
    copyClassData( oth );
}


Gather::~Gather()
{
    sendDelNotif();
}


mImplMonitorableAssignment( Gather, FlatDataPack )


void Gather::copyClassData( const Gather& oth )
{
    offsetisangle_ = oth.offsetisangle_;
    iscorr_ = oth.iscorr_;
    trckey_ = oth.trckey_;
    coord_ = oth.coord_;
    zit_ = oth.zit_;
    azimuths_ = oth.azimuths_;
    velocityid_ = oth.velocityid_;
    storageid_ = oth.storageid_;
    staticsid_ = oth.staticsid_;
}


Monitorable::ChangeType Gather::compareClassData( const Gather& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	offsetisangle_ == oth.offsetisangle_ &&
	iscorr_ == oth.iscorr_ &&
	trckey_ == oth.trckey_ &&
	coord_ == oth.coord_ &&
	zit_ == oth.zit_ &&
	azimuths_ == oth.azimuths_ &&
	velocityid_ == oth.velocityid_ &&
	storageid_ == oth.storageid_ &&
	staticsid_ == oth.staticsid_
	    );
}


Gather::Gather( const FlatPosData& fposdata )
    : FlatDataPack( sDataPackCategory(),
        new Array2DImpl<float>(fposdata.nrPts(true),fposdata.nrPts(false)) )
    , offsetisangle_( false )
    , iscorr_( false )
    , coord_( 0, 0 )
    , zit_( SI().zIsTime() )
{
    posdata_ = fposdata;
}


bool Gather::readFrom( const DBKey& mid, const TrcKey& tk, int comp,
		       uiString* errmsg )
{
    PtrMan<IOObj> ioobj = DBM().get( mid );
    if ( !ioobj )
    {
	if ( errmsg ) (*errmsg) = tr("No valid gather selected.");
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    return readFrom( *ioobj, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const TrcKey& tk, int comp,
		       uiString* errmsg )
{
    PtrMan<SeisPSReader> rdr = SPSIOPF().getReader( ioobj, tk );
    if ( !rdr )
    {
	if ( errmsg )
	    (*errmsg) = tr("This Prestack data store cannot be handled.");
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    return readFrom( ioobj, *rdr, tk, comp, errmsg );
}



bool Gather::readFrom( const DBKey& mid, const BinID& bid, int comp,
		       uiString* errmsg )
{
    TrcKey tk( bid );
    return readFrom( mid, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const BinID& bid, int comp,
		       uiString* errmsg )
{
    TrcKey tk( bid );
    return readFrom( ioobj, tk, comp, errmsg );
}


bool Gather::readFrom( const DBKey& mid, const int trcnr,
		       const char* linename, int comp, uiString* errmsg )
{
    Pos::GeomID geomid = Survey::GM().getGeomID( linename );
    if ( mIsUdfGeomID(geomid) )
	return false;

    TrcKey tk( geomid, trcnr );
    return readFrom( mid, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, const int tracenr,
		       const char* linename, int comp, uiString* errmsg )
{
    Pos::GeomID geomid = Survey::GM().getGeomID( linename );
    if ( mIsUdfGeomID(geomid) )
	return false;

    TrcKey tk( geomid, tracenr );
    return readFrom( ioobj, tk, comp, errmsg );
}


bool Gather::readFrom( const IOObj& ioobj, SeisPSReader& rdr, const TrcKey& tk,
		       int comp, uiString* errmsg )
{
    PtrMan<SeisTrcBuf> tbuf = new SeisTrcBuf( true );
    if ( !rdr.getGather(tk,*tbuf) )
    {
	if ( errmsg ) (*errmsg) = rdr.errMsg();
	delete arr2d_; arr2d_ = 0;
	return false;
    }
    if ( !setFromTrcBuf( *tbuf, comp, true ) )
       return false;

    ioobj.pars().getYN(sKeyZisTime(),zit_);

    velocityid_.setInvalid();
    GetVelocityVolumeTag( ioobj, velocityid_ );
    staticsid_.setInvalid();
    ioobj.pars().get( sKeyStaticsID(), staticsid_ );

    offsetisangle_ = false;
    ioobj.pars().getYN(sKeyIsAngleGather(), offsetisangle_ );

    iscorr_ = false;
    if ( !ioobj.pars().getYN(sKeyIsCorr(), iscorr_ ) )
	ioobj.pars().getYN( "Is NMO Corrected", iscorr_ );

    trckey_ = tk;
    setName( ioobj.name() );

    storageid_ = ioobj.key();

    return true;
}


bool Gather::readFrom( const IOObj& ioobj, SeisPSReader& rdr, const BinID& bid,
		       int comp, uiString* errmsg )
{
    TrcKey tk( bid );
    return readFrom( ioobj, rdr, tk, comp, errmsg );
}


bool Gather::setFromTrcBuf( SeisTrcBuf& tbuf, int comp, bool snapzrgtosi )
{
    tbuf.sort( true, SeisTrcInfo::Offset );

    bool isset = false;
    StepInterval<float> zrg;
    Coord crd( coord_ );
    for ( int idx=tbuf.size()-1; idx>-1; idx-- )
    {
	const SeisTrc* trc = tbuf.get( idx );
	if ( mIsUdf( trc->info().offset_ ) )
	    delete tbuf.remove( idx );

	const int trcsz = trc->size();
	if ( !isset )
	{
	    isset = true;
	    zrg.setFrom( trc->info().sampling_.interval( trcsz ) );
	    crd = trc->info().coord_;
	}
	else
	{
	    zrg.start = mMIN( trc->info().sampling_.start, zrg.start );
	    zrg.stop = mMAX( trc->info().sampling_.atIndex(trcsz-1), zrg.stop );
	    zrg.step = mMIN( trc->info().sampling_.step, zrg.step );
	}
    }

    if ( !isset )
    {
	delete arr2d_; arr2d_ = 0;
	return false;
    }

    if ( snapzrgtosi )
    {
	SI().snapZ(zrg.start);
	SI().snapZ(zrg.stop);
    }

    zrg_ = zrg;
    int nrsamples = zrg.nrSteps()+1;
    if ( zrg.step>0 && (zrg.stop-zrg.atIndex(nrsamples-1))>fabs(zrg.step*0.5) )
	nrsamples++;

    const Array2DInfoImpl newinfo( tbuf.size(), nrsamples );
    if ( !arr2d_ || !arr2d_->setInfo( newinfo ) )
    {
	delete arr2d_;
	arr2d_ = new Array2DImpl<float>( newinfo );
	if ( !arr2d_ || !arr2d_->isOK() )
	{
	    delete arr2d_; arr2d_ = 0;
	    return false;
	}
    }

    azimuths_.setSize( tbuf.size(), mUdf(float) );

    for ( int trcidx=tbuf.size()-1; trcidx>=0; trcidx-- )
    {
	const SeisTrc* trc = tbuf.get( trcidx );
	for ( int idx=0; idx<nrsamples; idx++ )
	{
	    const float val = trc->getValue( zrg.atIndex( idx ), comp );
	    arr2d_->set( trcidx, idx, val );
	}

	azimuths_[trcidx] = trc->info().azimuth_;
    }

    double offset;
    float* offsets = tbuf.getHdrVals(SeisTrcInfo::Offset, offset);
    if ( !offsets )
	return false;
    posData().setX1Pos( offsets, tbuf.size(), offset );
    StepInterval<double> pzrg(zrg.start, zrg.stop, zrg.step);
    posData().setRange( false, pzrg );

    zit_ = SI().zIsTime();
    coord_ = crd;
    const BinID binid = SI().transform( coord_ );
    trckey_ = TrcKey( binid );

    return true;
}


const char* Gather::dimName( bool dim0 ) const
{
    return dim0 ? sKey::Offset() :
			(SI().zIsTime() ? sKey::Time() : sKey::Depth());
}


void Gather::getAuxInfo( int idim0, int idim1, IOPar& par ) const
{
    par.set( "X", coord_.x_ );
    par.set( "Y", coord_.y_ );
    float z = (float) posData().position( false, idim1 );
    if ( zit_ ) z *= 1000;
    par.set( "Z", z );
    par.set( sKey::Offset(), getOffset(idim0) );
    if ( azimuths_.validIdx(idim0) )
	par.set( sKey::Azimuth(), getAzimuth(idim0) );
    if ( !is3D() )
	par.set( sKey::TraceNr(), trckey_.trcNr() );
    else
	par.set( sKey::Position(), trckey_.position().toString() );
}


float Gather::getOffset( int idx ) const
{ return (float) posData().position( true, idx ); }


float Gather::getAzimuth( int idx ) const
{
    return azimuths_[idx];
}


OffsetAzimuth Gather::getOffsetAzimuth( int idx ) const
{
    return OffsetAzimuth( (float) posData().position( true, idx ),
			  azimuths_[idx] );
}


bool Gather::getVelocityID(const DBKey& stor, DBKey& vid )
{
    PtrMan<IOObj> ioobj = DBM().get( stor );
    return ioobj && GetVelocityVolumeTag( *ioobj, vid );
}

#define mIfNonZero \
const float val = data().get( offset, idz ); \
if ( val && !mIsUdf(val) )

void Gather::detectInnerMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = 0;
    for ( int offset=0; offset<nroffsets; offset++ )
    {
	for ( int idz=nrz-1; idz>=muteend; idz-- )
	{
	    mIfNonZero
		muteend = idz+1;
	}

	res[offset] = muteend-taperlen;
    }
}


void Gather::detectOuterMutes( int* res, int taperlen ) const
{
    const int nroffsets = size( !offsetDim() );
    const int nrz = size( offsetDim() );

    int muteend = nrz-1;
    for ( int offset=nroffsets-1; offset>=0; offset-- )
    {
	for ( int idz=0; idz<=muteend; idz++ )
	{
	    mIfNonZero
		muteend = idz-1;
	}

	res[offset] = muteend + taperlen;
    }
}



GatherSetDataPack::GatherSetDataPack( const char* categry,
				      const ObjectSet<Gather>& gathers )
    : DataPack( categry )
    , gathers_( gathers )
{
    for ( int gidx=0; gidx<gathers_.size(); gidx++ )
    {
	DPM(DataPackMgr::FlatID()).add( gathers_[gidx] );
	gathers_[gidx]->ref();
    }
}


GatherSetDataPack::GatherSetDataPack( const GatherSetDataPack& oth )
    : DataPack( oth )
{
    copyClassData( oth );
}


GatherSetDataPack::~GatherSetDataPack()
{
    sendDelNotif();
    deepUnRef( gathers_ );
}


mImplMonitorableAssignment( GatherSetDataPack, DataPack )


void GatherSetDataPack::copyClassData( const GatherSetDataPack& oth )
{
    deepUnRef( gathers_ );
    for ( int idx=0; idx<oth.gathers_.size(); idx++ )
	gathers_ += oth.gathers_[idx]->clone();
}


Monitorable::ChangeType GatherSetDataPack::compareClassData(
					const GatherSetDataPack& oth ) const
{
    if ( gathers_.size() != oth.gathers_.size() )
	return cEntireObjectChange();

    for ( int idx=0; idx<gathers_.size(); idx++ )
	if ( *gathers_[idx] != *oth.gathers_[idx] )
	    return cEntireObjectChange();

    return cNoChange();
}


const Gather* GatherSetDataPack::getGather( const BinID& bid ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	if ( gathers_[idx]->getBinID() == bid )
	    return gathers_[idx];
    }

    return 0;
}


void GatherSetDataPack::addGather( Gather* gather )
{
    DPM(DataPackMgr::FlatID()).add( gather );
    gathers_ += gather;
}


float GatherSetDataPack::gtNrKBytes() const
{
    float totalnrkbytes = 0.0f;
    for ( int idx=0; idx<gathers_.size(); idx++ )
	totalnrkbytes += gathers_[idx]->nrKBytes();

    return totalnrkbytes;
}


void GatherSetDataPack::fill( Array2D<float>& inp, int offsetidx ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
    {
	const Array2D<float>& data = gathers_[idx]->data();
	for ( int idz=0; idz<data.info().getSize(0); idz++ )
	    inp.set( idx, idz, data.get( offsetidx, idz ) );
    }
}


void GatherSetDataPack::fill( SeisTrcBuf& inp, int offsetidx ) const
{
    for ( int idx=0; idx<gathers_.size(); idx++ )
	inp.add( gtTrace(idx,offsetidx) );
}


void GatherSetDataPack::fill( SeisTrcBuf& inp, Interval<float> stackrg ) const
{
    const int gathersz = gathers_.size();
    TypeSet<int> offidxs;
    for ( int idx=0; idx<gathersz; idx++ )
    {
	const Gather& gather = *gathers_[idx];
	const int offsz = gather.size( true );

	for ( int idoff=0; idoff<offsz; idoff++ )
	{
	    if ( stackrg.includes( gather.getOffset( idoff ), true ) )
		offidxs.addIfNew( idoff );
	}
    }
    if ( offidxs.isEmpty() )
	return;

    for ( int idx=0; idx<offidxs.size(); idx++ )
    {
	if ( inp.isEmpty() )
	    fill( inp, offidxs[idx] );
	else
	{
	    SeisTrcBuf buf(false); fill( buf, offidxs[idx] );
	    for ( int idgather=0; idgather<gathersz; idgather++ )
	    {
		SeisTrcPropChg stckr( *inp.get( idgather ) );
		stckr.stack( *buf.get( idgather ) );
	    }
	}
    }
}


void GatherSetDataPack::fillGatherBuf( SeisTrcBuf& seisbuf,const BinID& bid)
{
    const Gather* gather = 0; int gatheridx = -1;
    for ( int idx=0; idx<gathers_.size(); idx++ )
	if ( gathers_[idx]->getBinID() == bid )
	    { gather = gathers_[idx]; gatheridx = idx; break; }
    if ( !gather ) return;

    for ( int offsetidx=0; offsetidx<gather->nrOffsets(); offsetidx++ )
	seisbuf.add( getTrace(gatheridx,offsetidx) );
}


const SeisTrc* GatherSetDataPack::getTrace(
		const BinID& bid, int offsetidx ) const
{
    int gatheridx = -1;
    for ( int idx=0; idx<gathers_.size(); idx++ )
	if ( gathers_[idx]->getBinID() == bid )
	    { gatheridx = idx; break; }

    return gtTrace( gatheridx, offsetidx );
}


const SeisTrc* GatherSetDataPack::getTrace(int gatheridx,int offsetidx) const
{
    return gtTrace( gatheridx, offsetidx );
}


SeisTrc* GatherSetDataPack::getTrace( int gatheridx, int offsetidx )
{
    return gtTrace( gatheridx, offsetidx );
}


SeisTrc* GatherSetDataPack::gtTrace( int gatheridx, int offsetidx ) const
{
    if ( !gathers_.validIdx(gatheridx) ||
	    offsetidx>=gathers_[gatheridx]->nrOffsets() )
	return 0;

    const Gather* gather = gathers_[gatheridx];
    const int gathersz = gather->size( false );
    SeisTrc* trc = new SeisTrc( gathersz );
    trc->info().trckey_ = gather->getTrcKey();
    trc->info().coord_ = trc->info().trckey_.getCoord();
    const SamplingData<double>& sd = gather->posData().range( false );
    trc->info().sampling_.set( mCast(float,sd.start), mCast(float,sd.step));
    const Array2D<float>& data = gather->data();
    for ( int idz=0; idz<gathersz; idz++ )
	trc->set( idz, data.get( offsetidx, idz ), 0 );

    return trc;
}
