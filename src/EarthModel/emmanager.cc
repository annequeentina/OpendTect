/*+
 * COPYRIGHT: (C) de Groot-Bril Earth Sciences B.V.
 * AUTHOR   : K. Tingdahl
 * DATE     : Apr 2002
-*/

static const char* rcsID = "$Id: emmanager.cc,v 1.11 2003-04-22 11:01:52 kristofer Exp $";

#include "emmanager.h"

#include "ctxtioobj.h"
#include "emfaulttransl.h"
#include "emhistory.h"
#include "emhorizontransl.h"
#include "emobject.h"
#include "emwelltransl.h"
#include "executor.h"
#include "iodir.h"
#include "ioman.h"
#include "ptrman.h"
#include "uidobjset.h"


EarthModel::EMManager& EarthModel::EMM()
{
    static PtrMan<EMManager> emm = 0;

    if ( !emm ) emm = new EarthModel::EMManager;
    return *emm;
}

EarthModel::EMManager::EMManager()
    : history_( *new EarthModel::History(*this) )
{
    init();
}


EarthModel::EMManager::~EMManager()
{
    deepErase( objects );
    delete &history_;
}


const EarthModel::History& EarthModel::EMManager::history() const
{ return history_; }


EarthModel::History& EarthModel::EMManager::history()
{ return history_; }


void EarthModel::EMManager::init()
{ } 


MultiID EarthModel::EMManager::add( EarthModel::EMManager::Type type,
				const char* name )
{
    IOM().to( MultiID(IOObjContext::getStdDirData(IOObjContext::Mdl)->id) );

    if ( type==EarthModel::EMManager::Hor )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EarthModelHorizonTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();

	if ( !ctio->ioobj ) return -1;
	MultiID key = ctio->ioobj->key();

	Horizon* hor = new Horizon( *this, key );
	PtrMan<Executor> exec = hor->saver();
	exec->execute();
	objects += hor;

	return key;
    }


    if ( type==EarthModel::EMManager::Fault )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EarthModelFaultTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();

	if ( !ctio->ioobj ) return -1;
	MultiID key = ctio->ioobj->key();

	EarthModel::Fault* fault = new EarthModel::Fault( *this, key );
	PtrMan<Executor> exec = fault->saver();
	exec->execute();
	objects += fault;

	return key;
    }


    if ( type==EarthModel::EMManager::Well )
    {
	CtxtIOObj* ctio =
	    new CtxtIOObj(EarthModelWellTranslator::ioContext());
	ctio->ctxt.forread = false;
	ctio->ioobj = 0;
	ctio->setName( name );
	ctio->fillObj();
	if ( !ctio->ioobj ) return -1;

	MultiID key = ctio->ioobj->key();

	EarthModel::Well* well = new EarthModel::Well( *this, key );
	PtrMan<Executor> exec = well->saver();
	exec->execute();

	objects += well;

	return key;
    }

    return -1;
}


EarthModel::EMObject* EarthModel::EMManager::getObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EarthModel::EMObject* EarthModel::EMManager::getObject( const MultiID& id ) const
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id()==id )
	    return objects[idx];
    }

    return 0;
}


const EarthModel::EMObject* EarthModel::EMManager::getEMObject( int idx ) const
{
    if ( objects.size() )
	return objects[idx];

    return 0;
}


EarthModel::EMObject* EarthModel::EMManager::getEMObject( int idx )
{
    if ( objects.size() )
	return objects[idx];

    return 0;
}


void EarthModel::EMManager::removeObject( const MultiID& id )
{
    for ( int idx=0; idx<objects.size(); idx++ )
    {
	if ( objects[idx]->id() == id )
	{
	    objects.remove( idx );
	    return;
	}
    }
}


Executor* EarthModel::EMManager::load( const MultiID& id )
{
    EMObject* obj = getObject( id );
    if ( obj ) return obj->loader();
    
    PtrMan<IOObj> ioobj = IOM().get( id );
    if ( !ioobj ) return 0;

    const char* grpname = ioobj->group();
    if ( !strcmp( grpname, EarthModelWellTranslator::keyword ))
    {
	EarthModel::Well* well = new EarthModel::Well( *this, id );
	objects += well;
	return well->loader();
    }

    if ( !strcmp( grpname, EarthModelHorizonTranslator::keyword ))
    {
	EarthModel::Horizon* hor = new EarthModel::Horizon( *this, id );
	objects += hor;
	return hor->loader();
    }


    if ( !strcmp( grpname, EarthModelFaultTranslator::keyword ))
    {
	EarthModel::Fault* fault = new EarthModel::Fault( *this, id );
	objects += fault;
	return fault->loader();
    }

    return 0;
}


bool EarthModel::EMManager::isLoaded(const MultiID& id ) const
{
    const EMObject* obj = getObject( id );
    return obj ? obj->isLoaded() : false;
}
