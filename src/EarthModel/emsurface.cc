/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Oct 1999
________________________________________________________________________

-*/

#include "emsurface.h"

#include "trckeyzsampling.h"
#include "emhorizon2d.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emrowcoliterator.h"
#include "emsurfacegeometry.h"
#include "emsurfaceiodata.h"
#include "emsurfaceauxdata.h"
#include "filepath.h"
#include "ioobj.h"
#include "dbman.h"
#include "iopar.h"
#include "posfilter.h"
#include "posinfo2dsurv.h"


static const char* sDbInfo = "DB Info";
static const char* sRange = "Range";
static const char* sValnms = "Value Names";
static const char* sSections = "Patches";


namespace EM
{

void SurfaceIOData::clear()
{
    rg.init(false);
    dbinfo = "";
    valnames.setEmpty();
    sections.setEmpty();
    linenames.setEmpty();
    linesets.setEmpty();
    geomids.setEmpty();
    trcranges.setEmpty();
}


void SurfaceIOData::use( const Surface& surf )
{
    clear();

    mDynamicCastGet( const RowColSurfaceGeometry*, rcsg, &surf.geometry() );
    if ( rcsg )
    {
	StepInterval<int> hrg = rcsg->rowRange();
	rg.start_.inl() = hrg.start; rg.stop_.inl() = hrg.stop;
	rg.step_.inl() = hrg.step;
	hrg = rcsg->colRange();
	rg.start_.crl() = hrg.start; rg.stop_.crl() = hrg.stop;
	rg.step_.crl() = hrg.step;
    }

    for ( int idx=0; idx<surf.nrSections(); idx++ )
	sections += new BufferString( surf.sectionName( surf.sectionID(idx) ) );

    mDynamicCastGet(const Horizon3D*,horizon,&surf);
    if ( horizon )
    {
	for ( int idx=0; idx<horizon->auxdata.nrAuxData(); idx++ )
	    valnames += new BufferString( horizon->auxdata.auxDataName(idx) );
    }

    linenames.setEmpty();
    linesets.setEmpty();
    geomids.setEmpty();
    trcranges.setEmpty();
    mDynamicCastGet(const Horizon2D*,horizon2d,&surf);
    if ( horizon2d )
    {
	const Horizon2DGeometry& emgeom = horizon2d->geometry();
	for ( int idx=0; idx<emgeom.nrLines(); idx++ )
	{
	    const Pos::GeomID geomid = emgeom.geomID( idx );
	    linenames.add( emgeom.lineName(idx) );
	    geomids += geomid;
	    trcranges += emgeom.colRange( geomid );
	}
    }
}


void SurfaceIOData::fillPar( IOPar& iopar ) const
{
    iopar.set( sDbInfo, dbinfo );

    IOPar bidpar;
    rg.fillPar( bidpar );
    iopar.mergeComp( bidpar, sRange );

    IOPar valnmspar;
    valnames.fillPar( valnmspar );
    iopar.mergeComp( valnmspar, sValnms );

    IOPar sectionpar;
    sections.fillPar( sectionpar );
    iopar.mergeComp( sectionpar, sSections );

    int nrlines = geomids.size();
    if ( !nrlines ) nrlines = linenames.size();
    iopar.set( Horizon2DGeometry::sKeyNrLines(), nrlines );
    for ( int idx=0; idx<nrlines; idx++ )
    {
	if ( geomids.validIdx(idx)
		&& geomids[idx] != Survey::GeometryManager::cUndefGeomID() )
	    iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomids[idx] );
	else if ( linesets.validIdx(idx) && linenames.validIdx(idx) )
	{
	    Pos::GeomID geomid = Survey::GM().getGeomID( linesets.get(idx),
							 linenames.get(idx) );
	    if ( geomid == Survey::GeometryManager::cUndefGeomID() )
		continue;

	    iopar.set( IOPar::compKey(sKey::GeomID(),idx), geomid );
	}

	BufferString key = IOPar::compKey( "Line", idx );
	iopar.set( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		   trcranges[idx] );
    }
}


void SurfaceIOData::usePar( const IOPar& iopar )
{
    iopar.get( sDbInfo, dbinfo );

    IOPar* bidpar = iopar.subselect(sRange);
    if ( bidpar ) rg.usePar( *bidpar );

    IOPar* valnmspar = iopar.subselect(sValnms);
    if ( valnmspar ) valnames.usePar( *valnmspar );

    IOPar* sectionpar = iopar.subselect(sSections);
    if ( sectionpar ) sections.usePar( *sectionpar );

    if ( iopar.find(Horizon2DGeometry::sKeyNrLines()) )
    {
	int nrlines = 0;
	iopar.get( Horizon2DGeometry::sKeyNrLines(), nrlines );
	for ( int idx=0; idx<nrlines; idx++ )
	{
	    BufferString key = IOPar::compKey( "Line", idx );
	    Pos::GeomID geomid = Survey::GeometryManager::cUndefGeomID();
	    if ( !iopar.get(IOPar::compKey(sKey::GeomID(),idx),geomid) )
	    {
		BufferString idstr;
		if ( iopar.get(IOPar::compKey(key,Horizon2DGeometry::sKeyID()),
			       idstr) )
		{
		    PosInfo::Line2DKey l2dkey; l2dkey.fromString( idstr );
		    if ( S2DPOS().curLineSetID() != l2dkey.lsID() )
			S2DPOS().setCurLineSet( l2dkey.lsID() );
		    geomid = Survey::GM().getGeomID(
				S2DPOS().getLineSet(l2dkey.lsID()),
				S2DPOS().getLineName(l2dkey.lineID()) );
		}
	    }

	    if ( geomid == Survey::GeometryManager::cUndefGeomID() )
		continue;

	    geomids += geomid;
	    linenames.add( Survey::GM().getName(geomid) );
	    StepInterval<int> trcrange;
	    iopar.get( IOPar::compKey(key,Horizon2DGeometry::sKeyTrcRg()),
		       trcrange );
	    trcranges += trcrange;
	}
    }
}


void SurfaceIODataSelection::setDefault()
{
    rg = sd.rg;
    seltrcranges = sd.trcranges;
    selvalues.setEmpty(); selsections.setEmpty();
    for ( int idx=0; idx<sd.valnames.size(); idx++ )
	selvalues += idx;
    for ( int idx=0; idx<sd.sections.size(); idx++ )
	selsections += idx;
}


Surface::Surface( EMManager& man)
    : EMObject( man )
{
}


Surface::~Surface()
{}

void Surface::removeAll() {}

int Surface::nrSections() const
{ return geometry().nrSections(); }

SectionID Surface::sectionID( int idx ) const
{ return geometry().sectionID(idx); }

BufferString Surface::sectionName( const SectionID& sid ) const
{ return geometry().sectionName(sid); }

bool Surface::canSetSectionName() const { return true; }

bool Surface::setSectionName( const SectionID& sid, const char* nm, bool hist )
{ return geometry().setSectionName(sid,nm,hist); }

bool Surface::removeSection( SectionID sid, bool hist )
{ return geometry().removeSection( sid, hist ); }

bool Surface::isAtEdge( const PosID& posid ) const
{ return geometry().isAtEdge(posid); }

bool Surface::isLoaded() const
{ return geometry().isLoaded(); }

Executor* Surface::saver() { return geometry().saver(); }

Executor* Surface::loader() { return geometry().loader(); }

Geometry::Element* Surface::sectionGeometryInternal( const SectionID& sid )
{ return geometry().sectionGeometry(sid); }

EMObjectIterator* Surface::createIterator( const SectionID& sid,
					   const TrcKeyZSampling* cs ) const
{ return geometry().createIterator( sid, cs ); }

bool Surface::enableGeometryChecks( bool nv )
{ return geometry().enableChecks( nv ); }

bool Surface::isGeometryChecksEnabled() const
{ return geometry().isChecksEnabled(); }

const SurfaceGeometry& Surface::geometry() const
{ return const_cast<Surface*>(this)->geometry(); }


void Surface::apply( const Pos::Filter& pf )
{
    PtrMan<EM::EMObjectIterator>iterator = createIterator( -1 );
    while ( true )
    {
	const EM::PosID pid = iterator->next();
	if ( pid.objectID()==-1 )
	    break;

	const Coord3 pos = getPos( pid );
	if ( !pf.includes(pos.getXY(),(float)pos.z_) )
	   unSetPos( pid, false );
    }
}


BufferString Surface::getParFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    File::Path fp( ioobj.mainFileName() );
    fp.setExtension( sParFileExtension() );
    return fp.fullPath();
}


BufferString Surface::getSetupFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    File::Path fp( ioobj.mainFileName() );
    fp.setExtension( "ts" );
    return fp.fullPath();
}


BufferString Surface::getParentChildFileName( const IOObj& ioobj )
{
    if ( !EM::canOverwrite(ioobj.key()) )
	return BufferString::empty();

    File::Path fp( ioobj.mainFileName() );
    fp.setExtension( "pcd" );
    return fp.fullPath();
}


bool Surface::usePar( const IOPar& par )
{
    return EMObject::usePar(par) && geometry().usePar(par);
}


void Surface::fillPar( IOPar& par ) const
{
    EMObject::fillPar( par );
    geometry().fillPar( par );
}

} // namespace EM
