/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Aug 2016
-*/


#include "coordsystem.h"

#include "iopar.h"
#include "notify.h"
#include "separstr.h"
#include "keystrs.h"

static const BufferString coordsysfactorynm_(
		IOPar::compKey(sKey::CoordSys(),"System name") );
static const BufferString coordsysusrnm_(
		IOPar::compKey(sKey::CoordSys(),"Description") );
const char* Coords::CoordSystem::sKeyFactoryName() { return coordsysfactorynm_;}
const char* Coords::CoordSystem::sKeyUiName() { return coordsysusrnm_; }

static const double cAvgEarthRadius = 6367450;
static const double latdist = cAvgEarthRadius*mDeg2RadD;

mImplFactory( Coords::CoordSystem, Coords::CoordSystem::factory );

using namespace Coords;

static Threads::Lock systemreposlock;
static ManagedObjectSet<IOPar> systemrepos;


static void reloadRepository(CallBacker*)
{
    Threads::Locker lock( systemreposlock );
    systemrepos.erase();

    //Todo
}


bool CoordSystem::operator==( const CoordSystem& oth ) const
{
    IOPar myiop; fillPar( myiop );
    IOPar othiop; oth.fillPar( othiop );
    return myiop == othiop;
}


void CoordSystem::initRepository( NotifierAccess* na )
{
    reloadRepository( 0 );

    if ( na ) na->notify( mSCB( reloadRepository ) );
    //Don't do remove, as we assume we will be destroyed
    //after NotifierAccess's last call
}


void CoordSystem::getSystemNames( bool orthogonalonly, bool projectiononly,
				uiStringSet& strings, ObjectSet<IOPar>& pars )
{
    deepErase( pars );
    strings.setEmpty();

    //Add all factory entries
    const BufferStringSet factorynames = factory().getNames();
    const uiStringSet factoryuinames = factory().getUserNames();

    for ( int idx=0; idx<factorynames.size(); idx++ )
    {
	mDeclareAndTryAlloc( PtrMan<IOPar>, systempar, IOPar );
	if ( !systempar ) //out of memory
	    continue;

	systempar->set( sKeyFactoryName(), factorynames.get(idx) );
	systempar->set( sKeyUiName(), factoryuinames[idx] );

	if ( orthogonalonly || projectiononly )
	{
	    RefMan<CoordSystem> system = createSystem( *systempar );
	    if ( !system || ( orthogonalonly && !system->isOrthogonal() )
		    || ( projectiononly && !system->isProjection() ) )
		continue;
	}

	pars += systempar.release();
	strings += factoryuinames[idx];
    }

    //Add all repository entries
    Threads::Locker lock( systemreposlock );

    for ( int idx=0; idx<systemrepos.size(); idx++ )
    {
	PtrMan<IOPar> systempar = new IOPar( *systemrepos[idx] );
	RefMan<CoordSystem> system = createSystem( *systempar );
	if ( !system )
	    continue;

	if ( orthogonalonly && !system->isOrthogonal() )
	    continue;

	uiString uiname;
	BufferString factoryname;
	if ( !systempar->get( sKeyUiName(), uiname ) ||
	     !systempar->get( sKeyFactoryName(), factoryname ) )
	    continue;

	pars += systempar.release();
	strings += uiname;
    }
}


RefMan<CoordSystem> CoordSystem::createSystem( const IOPar& par )
{
    BufferString factorykey;
    if ( !par.get( sKeyFactoryName(), factorykey ) )
	return 0;

    RefMan<CoordSystem> res = factory().create( factorykey );
    if ( !res )
	return 0;

    if ( !res->usePar( par ) )
	return 0;

    return res;
}


Coord CoordSystem::convert( const Coord& in, const CoordSystem& from,
			       const CoordSystem& to )
{
    const LatLong geomwgs84( LatLong::transform(in,true,&from) );

    return LatLong::transform( geomwgs84, true, &to );
}


Coord CoordSystem::convertFrom( const Coord& in,
				   const CoordSystem& from ) const
{ return convert( in, from, *this ); }


bool CoordSystem::usePar( const IOPar& par )
{
    BufferString nm;
    if ( !par.get(sKeyFactoryName(),nm) || nm != factoryKeyword() )
	return false;

    PtrMan<IOPar> subpar = par.subselect( sKey::CoordSys() );
    if ( !subpar || subpar->isEmpty() )
	{ pErrMsg("Huh"); return false; }

    return doUsePar( *subpar );
}


void CoordSystem::fillPar( IOPar& par ) const
{
    par.removeSubSelection( sKey::CoordSys() );

    par.set( sKeyFactoryName(), factoryKeyword() );

    IOPar subpar;
    doFillPar( subpar );
    par.mergeComp( subpar, sKey::CoordSys() );
}


uiString CoordSystem::toUiString( const Coord& crd ) const
{
    return ::toUiString( toString(crd,false) );
}


BufferString CoordSystem::toString( const Coord& crd, bool withsystem ) const
{
    BufferString res;
    const char* space = " ";
    if ( withsystem )
	res.add( factoryKeyword() ).add( space );

    res.add( ::toString(crd.x_) ).add( space );
    res.add( ::toString(crd.y_));

    return res;
}


Coord CoordSystem::fromString( const char* str ) const
{
    const SeparString sepstr( str, ' ' );
    const int nrparts = sepstr.size();

    if ( nrparts==3 ) //With coord system name first
    {
	const BufferString system = sepstr[0];
	if ( system != factoryKeyword() )
	    return Coord::udf();
    }
    else if ( nrparts<2 )
	return Coord::udf();

    const BufferString xstr = sepstr[nrparts-2];
    const BufferString ystr = sepstr[nrparts-1];

    return Coord( toDouble(xstr,mUdf(double)),
		  toDouble(ystr,mUdf(double)) );
}


UnlocatedXY::UnlocatedXY()
{
}


CoordSystem* UnlocatedXY::clone() const
{
    UnlocatedXY* cp = new UnlocatedXY;
    cp->isfeet_ = isfeet_;
    return cp;
}


LatLong UnlocatedXY::toGeographic( const Coord&, bool ) const
{
    return LatLong::udf();
}


Coord UnlocatedXY::fromGeographic( const LatLong&, bool ) const
{
    return Coord::udf();
}


static const char* sKeyIsFeet = "XY in Feet";

bool UnlocatedXY::doUsePar( const IOPar& par )
{
    par.getYN( sKeyIsFeet, isfeet_ );
    return true;
}


void UnlocatedXY::doFillPar( IOPar& par ) const
{
    par.setYN( sKeyIsFeet, isfeet_ );
}


AnchorBasedXY::AnchorBasedXY()
    : lngdist_(mUdf(float))
{
}


AnchorBasedXY::AnchorBasedXY( const LatLong& l, const Coord& c )
{
    setLatLongEstimate( l, c );
}


CoordSystem* AnchorBasedXY::clone() const
{
    AnchorBasedXY* cp = new AnchorBasedXY( reflatlng_, refcoord_ );
    cp->isfeet_ = isfeet_;
    return cp;
}


bool AnchorBasedXY::geographicTransformOK() const
{ return !mIsUdf(lngdist_); }

BufferString AnchorBasedXY::summary() const
{
    BufferString ret( "Anchor: LL:" );
    ret.add( reflatlng_.toString() ).add( refcoord_.toPrettyString() );
    return ret;
}


void AnchorBasedXY::setLatLongEstimate( const LatLong& ll, const Coord& c )
{
    refcoord_ = c; reflatlng_ = ll;
    lngdist_ = mDeg2RadD * cos( ll.lat_ * mDeg2RadD ) * cAvgEarthRadius;
}


LatLong AnchorBasedXY::toGeographic( const Coord& c, bool ) const
{
    if ( !geographicTransformOK() ) return reflatlng_;

    const double scalefac = isfeet_ ? mFromFeetFactorD : 1;

    Coord coorddist( (c.x_ - refcoord_.x_) * scalefac,
		    (c.y_ - refcoord_.y_) * scalefac );
    LatLong ll( reflatlng_.lat_ + coorddist.y_ / latdist,
	       reflatlng_.lng_ + coorddist.x_ / lngdist_ );

    if ( ll.lat_ > 90 )		ll.lat_ = 180 - ll.lat_;
    else if ( ll.lat_ < -90 )	ll.lat_ = -180 - ll.lat_;
    if ( ll.lng_ < -180 )	ll.lng_ = ll.lng_ + 360;
    else if ( ll.lng_ > 180 )	ll.lng_ = ll.lng_ - 360;

    return ll;
}


Coord AnchorBasedXY::fromGeographic( const LatLong& ll, bool ) const
{
    if ( !geographicTransformOK() ) return Coord::udf();

    const double scalefac = isfeet_ ? mFromFeetFactorD : 1;

    const LatLong latlongdist( ll.lat_ - reflatlng_.lat_,
			      ll.lng_ - reflatlng_.lng_ );
    return Coord( refcoord_.x_ + latlongdist.lng_ * lngdist_ / scalefac,
		 refcoord_.y_ + latlongdist.lat_ * latdist / scalefac );
}


static const char* sKeyRefLatLong = "Reference Lat/Long";
static const char* sKeyRefCoord = "Reference Coordinate";

bool AnchorBasedXY::doUsePar( const IOPar& par )
{
    Coord crd;
    LatLong latlong;
    par.getYN( sKeyIsFeet, isfeet_ );

    if ( par.get(sKeyRefLatLong,latlong.lat_,latlong.lng_) &&
	 par.get(sKeyRefCoord,crd) )
	setLatLongEstimate( latlong, crd );

    return true;
}


void AnchorBasedXY::doFillPar( IOPar& par ) const
{
    par.setYN( sKeyIsFeet, isfeet_ );
    par.set( sKeyRefLatLong, reflatlng_.lat_, reflatlng_.lng_ );
    par.set( sKeyRefCoord, refcoord_ );
}
