/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : Bert
 * DATE     : April 2016
-*/


#include "saveable.h"
#include "saveablemanager.h"
#include "monitorchangerecorder.h"
#include "autosaver.h"
#include "dbman.h"
#include "ioobj.h"
#include "ctxtioobj.h"


Saveable::Saveable( const SharedObject& obj )
    : object_(&obj)
    , objectalive_(true)
    , lastsavedirtycount_(0)
{
    attachCBToObj();
    mTriggerInstanceCreatedNotifier();
}


Saveable::Saveable( const Saveable& oth )
    : object_(oth.object_)
    , lastsavedirtycount_(0)
{
    *this = oth;
    mTriggerInstanceCreatedNotifier();
}


Saveable::~Saveable()
{
    sendDelNotif();
    detachAllNotifiers();
}


mImplMonitorableAssignment(Saveable,Monitorable)


void Saveable::copyClassData( const Saveable& oth )
{
    detachCBFromObj();
    object_ = oth.object_;
    objectalive_ = oth.objectalive_;
    storekey_ = oth.storekey_;
    ioobjpars_ = oth.ioobjpars_;
    lastsavedirtycount_ = oth.lastsavedirtycount_;
    attachCBToObj();
}


Monitorable::ChangeType Saveable::compareClassData( const Saveable& oth ) const
{
    mDeliverYesNoMonitorableCompare(
	    object_ == oth.object_
	&& storekey_ == oth.storekey_
	&& ioobjpars_ == oth.ioobjpars_ );
}


void Saveable::setObject( const SharedObject& obj )
{
    mLock4Read();
    if ( object_ == &obj )
	return;

    mLock2Write();
    detachCBFromObj();
    object_ = &obj;
    objectalive_ = true;
    attachCBToObj();
    mSendEntireObjChgNotif();
}


void Saveable::attachCBToObj()
{
    if ( objectalive_ )
	mAttachCB( object_->objectToBeDeleted(), Saveable::objDelCB );
}


void Saveable::detachCBFromObj()
{
    if ( objectalive_ )
	mDetachCB( const_cast<SharedObject&>(*object_).objectToBeDeleted(),
		   Saveable::objDelCB );
}


const SharedObject* Saveable::object() const
{
    mLock4Read();
    return object_;
}


Monitorable::DirtyCountType Saveable::curDirtyCount() const
{
    mLock4Read();
    return objectalive_ ? object_->dirtyCount() : lastsavedirtycount_.load();
}


void Saveable::objDelCB( CallBacker* )
{
    objectalive_ = false;
}


uiRetVal Saveable::save() const
{
    mLock4Read();
    if ( !objectalive_ )
	{ pErrMsg("save already deleted object"); return uiRetVal::OK(); }

    uiRetVal uirv;
    PtrMan<IOObj> ioobj = DBM().get( storekey_ );
    if ( !ioobj )
    {
	if ( storekey_.isValid() )
	    uirv = tr("Cannot find database entry for: %1").arg(storekey_);
	else
	    uirv = tr("Cannot save object without database key");
    }
    else
    {
	if ( !ioobj->pars().includes(ioobjpars_) )
	{
	    ioobj->pars().merge( ioobjpars_ );
	    DBM().setEntry( *ioobj );
	    ioobj = DBM().get( storekey_ );
	}

	uirv = store( *ioobj );

	if ( !uirv.isOK() )
	    mSendChgNotif( cSaveFailedChangeType(), storekey_.toInt64() );
	else
	{
	    setJustSaved();
	    mSendChgNotif( cSaveSucceededChangeType(), storekey_.toInt64() );
	}
    }

    return uirv;
}


bool Saveable::needsSave() const
{
    return !objectalive_ ? false : lastsavedirtycount_ != object_->dirtyCount();
}


void Saveable::setJustSaved() const
{
    if ( objectalive_ )
	lastsavedirtycount_ = object_->dirtyCount();
}


bool Saveable::isSave( const IOObj& ioobj ) const
{
    return storekey_ == ioobj.key();
}


uiRetVal Saveable::store( const IOObj& ioobj ) const
{
    if ( !objectalive_ )
	{ pErrMsg("store already deleted object"); return uiRetVal::OK(); }
    return doStore( ioobj );
}



SaveableManager::SaveableManager( const IOObjContext& ctxt, bool withautosave,
				  bool tempobjsonly )
    : ctxt_(*new IOObjContext(ctxt))
    , autosaveable_(withautosave)
    , tempobjsonly_(tempobjsonly)
    , ObjAdded(this)
    , ObjOrphaned(this)
    , UnsavedObjLastCall(this)
    , ShowRequested(this)
    , HideRequested(this)
    , VanishRequested(this)
{
    chgrecs_.allowNull( true );
    mAttachCB( DBM().surveyToBeChanged, SaveableManager::survChgCB );
    mAttachCB( DBM().applicationClosing, SaveableManager::appExitCB );
    mAttachCB( DBM().entryToBeRemoved, SaveableManager::dbmEntryRemovedCB );
}


SaveableManager::~SaveableManager()
{
    sendDelNotif();
    detachAllNotifiers();
    setEmpty();
    delete const_cast<IOObjContext*>( &ctxt_ );
}


void SaveableManager::setEmpty()
{
    deepErase( savers_ );
    deepErase( chgrecs_ );
}


void SaveableManager::setJustSaved( const ObjID& id ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
	savers_[idx]->setJustSaved();
}


uiRetVal SaveableManager::doSave( const ObjID& id ) const
{
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
    {
	PtrMan<IOObj> ioobj = getIOObj( id );
	if ( ioobj && ioobj->isTmp() )
	    return uiRetVal::OK();

	return savers_[idx]->save();
    }

    return uiRetVal::OK();
}


uiRetVal SaveableManager::save( const SharedObject& obj ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( obj );
    return idx<0 ? uiRetVal::OK() : doSave( savers_[idx]->key() );
}


uiRetVal SaveableManager::save( const ObjID& id ) const
{
    mLock4Read();
    return doSave( id );
}


uiRetVal SaveableManager::saveAs( const ObjID& id, const ObjID& newid ) const
{
    mLock4Read();

    const IdxType idx = gtIdx( id );
    if ( idx < 0 )
	{ pErrMsg("Save-As not loaded ID"); return uiRetVal::OK(); }

    Saveable& svr = *const_cast<Saveable*>( savers_[idx] );
    svr.setKey( newid );
    uiRetVal uirv = doSave( newid );
    if ( uirv.isError() )
	{ svr.setKey( id ); return uirv; } // rollback

    return uiRetVal::OK();
}


bool SaveableManager::needsSave( const ObjID& id ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( id );
    return idx < 0 ? false : savers_[idx]->needsSave();
}


bool SaveableManager::needsSave( const SharedObject& obj ) const
{
    mLock4Read();
    const IdxType idx = gtIdx( obj );
    return idx < 0 ? false : savers_[idx]->needsSave();
}


SaveableManager::ObjID SaveableManager::getIDByName( const char* nm ) const
{
    if ( !nm || !*nm )
	return ObjID::getInvalid();

    mLock4Read();

    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	const Saveable& saver = *savers_[idx];
	const SharedObject* obj = saver.object();
	if ( obj && obj->name() == nm )
	    return saver.key();
    }

    PtrMan<IOObj> ioobj = getIOObjByName( nm );
    if ( ioobj )
	return ioobj->key();

    return ObjID::getInvalid();
}


SaveableManager::ObjID SaveableManager::getID( const SharedObject& obj ) const
{
    mLock4Read();

    const IdxType idxof = gtIdx( obj );
    return idxof < 0 ? ObjID::getInvalid() : savers_[idxof]->key();
}


IOPar SaveableManager::getIOObjPars( const ObjID& id ) const
{
    if ( id.isInvalid() )
	return IOPar();

    mLock4Read();
    const IdxType idx = gtIdx( id );
    if ( idx >= 0 )
	return savers_[idx]->ioObjPars();
    mUnlockAllAccess();

    PtrMan<IOObj> ioobj = getIOObj( id );
    return ioobj ? ioobj->pars() : IOPar();
}


IOObj* SaveableManager::getIOObj( const ObjID& id ) const
{
    return DBM().get( id );
}


IOObj* SaveableManager::getIOObjByName( const char* nm ) const
{
    return DBM().getByName( ctxt_, nm );
}


BufferString SaveableManager::nameOf( const ObjID& id ) const
{
    return DBM().nameOf( id );
}


bool SaveableManager::nameExists( const char* nm ) const
{
    IOObj* ioobj = getIOObjByName( nm );
    delete ioobj;
    return ioobj;
}


const Saveable* SaveableManager::saverFor( const ObjID& id ) const
{
    const int idx = gtIdx( id );
    return idx < 0 ? 0 : savers_[idx];
}


bool SaveableManager::canSave( const ObjID& id ) const
{
    return DBM().isPresent( id );
}


uiRetVal SaveableManager::store( const SharedObject& newobj,
				 const IOPar* ioobjpars ) const
{
    const BufferString nm = newobj.name();
    if ( nm.isEmpty() )
	return tr("Please provide a name");

    PtrMan<IOObj> ioobj = getIOObjByName( nm );
    if ( !ioobj )
    {
	CtxtIOObj ctio( ctxt_ );
	ctio.setName( newobj.name() );
	ctio.ctxt_.forread_ = false;
	DBM().getEntry( ctio, tempobjsonly_ );
	ioobj = ctio.ioobj_;
	ctio.ioobj_ = 0;
    }

    return store( newobj, ioobj->key(), ioobjpars );
}


uiRetVal SaveableManager::store( const SharedObject& newobj, const ObjID& id,
				 const IOPar* ioobjpars ) const
{
    if ( id.isInvalid() )
	return store( newobj, ioobjpars );

    mLock4Read();

    const IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	add( newobj, id, mAccessLocker(), ioobjpars, false );
    else
    {
	SaveableManager& self = *const_cast<SaveableManager*>(this);
	Saveable& svr = *self.savers_[idxof];
	if ( svr.object() != &newobj )
	{
	    mLock2Write();
	    svr.setObject( newobj );
	    delete self.chgrecs_.replace( idxof, getChangeRecorder(newobj));
	}
	mUnlockAllAccess();
    }

    return save( id );
}


void SaveableManager::addNew( const SharedObject& obj, const ObjID& id,
				const IOPar* iop, bool justloaded ) const
{
    mLock4Read();
    add( obj, id, mAccessLocker(), iop, justloaded );
}


void SaveableManager::add( const SharedObject& newobj, const ObjID& id,
				AccessLocker& mAccessLocker(),
				const IOPar* ioobjpars, bool justloaded ) const
{
    Saveable* saver = getSaver( newobj );
    saver->setKey( id );
    if ( ioobjpars )
	saver->setIOObjPars( *ioobjpars );
    if ( justloaded )
	saver->setJustSaved();

    SaveableManager& self = *const_cast<SaveableManager*>(this);
    mLock2Write();
    self.savers_ += saver;
    self.chgrecs_ += getChangeRecorder( newobj );
    self.setAuxOnAdd();
    self.addCBsToObj( newobj );
    mUnlockAllAccess();

    self.ObjAdded.trigger( id );
    if ( autosaveable_ )
	OD::AUTOSAVE().add( *saver );
}


void SaveableManager::addCBsToObj( const SharedObject& obj )
{
    mAttachCB( obj.objectToBeDeleted(), SaveableManager::objDelCB );
    mAttachCB( obj.objectChanged(), SaveableManager::objChgCB );
}


bool SaveableManager::isValidID( const ObjID& id ) const
{
    if ( !id.isValid() )
	return false;

    PtrMan<IOObj> ioobj = getIOObj( id );
    if ( !ioobj )
	return false;

    return ctxt_.validIOObj( *ioobj );
}


bool SaveableManager::isLoaded( const char* nm ) const
{
    if ( !nm || !*nm )
	return false;
    mLock4Read();
    return gtIdx( nm ) >= 0;
}


bool SaveableManager::isLoaded( const ObjID& id ) const
{
    if ( id.isInvalid() )
	return false;
    mLock4Read();
    return gtIdx( id ) >= 0;
}


void SaveableManager::getAllLoaded( DBKeySet& kys ) const
{
    mLock4Read();
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
	kys.add( savers_[idx]->key() );
}


SaveableManager::IdxType SaveableManager::size() const
{
    mLock4Read();
    return savers_.size();
}


DBKey SaveableManager::getIDByIndex( IdxType idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->key() : ObjID::getInvalid();
}


IOPar SaveableManager::getIOObjParsByIndex( IdxType idx ) const
{
    mLock4Read();
    return savers_.validIdx(idx) ? savers_[idx]->ioObjPars() : IOPar();
}


SaveableManager::IdxType SaveableManager::gtIdx( const char* nm ) const
{
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	const SharedObject* obj = savers_[idx]->object();
	if ( obj && obj->name() == nm )
	    return idx;
    }
    return -1;
}


SaveableManager::IdxType SaveableManager::gtIdx( const ObjID& id ) const
{
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->key() == id )
	    return idx;
    }
    return -1;
}


SaveableManager::IdxType SaveableManager::gtIdx( const SharedObject& obj ) const
{
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	if ( savers_[idx]->object() == &obj )
	    return idx;
    }
    return -1;
}


SharedObject* SaveableManager::gtObj( IdxType idx ) const
{
    return !savers_.validIdx(idx) ? 0
	 : const_cast<SharedObject*>( savers_[idx]->object() );
}


void SaveableManager::clearChangeRecords( const ObjID& id )
{
    mLock4Read();
    IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return;
    ChangeRecorder* rec = chgrecs_[ idxof ];
    if ( !rec || rec->isEmpty() )
	return;

    if ( !mLock2Write() )
    {
	idxof = gtIdx( id );
	if ( idxof < 0 )
	    return;
	rec = chgrecs_[ idxof ];
    }

    if ( rec )
	rec->setEmpty();
}


void SaveableManager::getChangeInfo( const ObjID& id, uiString& undotxt,
				      uiString& redotxt ) const
{
    undotxt.setEmpty(); redotxt.setEmpty();
    mLock4Read();
    IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return;
    const ChangeRecorder* rec = chgrecs_[ idxof ];
    if ( !rec || rec->isEmpty() )
	return;

    if ( rec->canApply(ChangeRecorder::Undo) )
	undotxt = rec->usrText( ChangeRecorder::Undo );
    if ( rec->canApply(ChangeRecorder::Redo) )
	redotxt = rec->usrText( ChangeRecorder::Redo );
}


bool SaveableManager::useChangeRecord( const ObjID& id, bool forundo )
{
    mLock4Read();
    IdxType idxof = gtIdx( id );
    if ( idxof < 0 )
	return false;

    ChangeRecorder* rec = chgrecs_[ idxof ];
    mUnlockAllAccess();
    return !rec ? true
	 : rec->apply( forundo ? ChangeRecorder::Undo : ChangeRecorder::Redo );
}


void SaveableManager::displayRequest( const ObjID& objid, DispOpt opt )
{
    switch ( opt )
    {
	case Show:
	    ShowRequested.trigger( objid );
	break;
	case Hide:
	    HideRequested.trigger( objid );
	break;
	case Vanish:
	{
	    if ( needsSave(objid) )
		UnsavedObjLastCall.trigger( objid );
	    VanishRequested.trigger( objid );
	}
	break;
    }
}


void SaveableManager::handleUnsavedLastCall()
{
    mLock4Read();
    for ( IdxType idx=0; idx<savers_.size(); idx++ )
    {
	ConstRefMan<SharedObject> obj = savers_[idx]->object();
	if ( !obj )
	    continue;

	if ( savers_[idx]->lastSavedDirtyCount() != obj->dirtyCount() )
	    UnsavedObjLastCall.trigger( savers_[idx]->key() );
    }
}


void SaveableManager::dbmEntryRemovedCB( CallBacker* cb )
{
    mCBCapsuleUnpack( ObjID, id, cb );
    if ( isLoaded(id) )
	ObjOrphaned.trigger( id );
}


void SaveableManager::survChgCB( CallBacker* )
{
    handleUnsavedLastCall();
    setEmpty();
}


void SaveableManager::appExitCB( CallBacker* )
{
    handleUnsavedLastCall();
}


#define mHandleObjChgCBStart() \
    mDynamicCastGet(SharedObject*,obj,cb) \
    if ( !obj ) \
	{ pErrMsg("CB is not a SharedObject"); return; } \
 \
    mLock4Read(); \
    IdxType idxof = gtIdx( *obj ); \
    if ( idxof < 0 ) \
	{ pErrMsg("idxof < 0"); return; }


void SaveableManager::objDelCB( CallBacker* cb )
{
    mHandleObjChgCBStart();

    if ( !mLock2Write() )
	idxof = gtIdx( *obj );

    if ( idxof >= 0 )
    {
	delete savers_.removeSingle( idxof );
	delete chgrecs_.removeSingle( idxof );
    }
}


void SaveableManager::objChgCB( CallBacker* inpcb )
{
    mGetMonitoredChgDataWithCaller( inpcb, chgdata, cb );
    if ( !chgdata.isEntireObject() )
	return;

    mHandleObjChgCBStart();

    if ( !mLock2Write() )
	idxof = gtIdx( *obj );

    if ( idxof >= 0 )
    {
	ChangeRecorder* rec = chgrecs_[ idxof ];
	if ( rec )
	    rec->setEmpty();
    }
}
