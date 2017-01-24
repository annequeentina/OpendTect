#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
________________________________________________________________________


-*/

#include "earthmodelmod.h"
#include "notify.h"
#include "trckeyzsampling.h"
#include "draw.h"
#include "emposid.h"
#include "dbkey.h"
#include "coord.h"
#include "sharedobject.h"

#include "uistring.h"

class TrcKeyZSampling;
class Executor;
class IOObj;
class IOObjContext;
class TaskRunner;

namespace Geometry { class Element; }

template <class T> class Selector;
template <class T> class Array2D;

namespace EM
{
class EMManager;

/*!
\brief EM object callback data.
*/

mExpClass(EarthModel) EMObjectCallbackData
{
public:

    mDefIntegerIDType(int, CBID);

		EMObjectCallbackData()
		    : attrib( -1 )
		    , flagfor2dviewer( false )
		    , event( EMObjectCallbackData::Undef )
		    , cbid_(CBID::get(curcbid_++))
		{}


		EMObjectCallbackData( const EMObjectCallbackData& data )
		    : pid0( data.pid0 )
		    , pid1( data.pid1 )
		    , attrib( data.attrib )
		    , flagfor2dviewer( data.flagfor2dviewer )
		    , event( data.event )
		    , cbid_(data.cbid_)
		{}


    enum Event { Undef, PositionChange, PosIDChange, PrefColorChange, Removal,
		 AttribChange, SectionChange, NameChange, SelectionChange,
		 LockChange, BurstAlert, LockColorChange, SelectionColorChange,
		 ParentColorChange, LineStyleChange, MarkerStyleChange } event;

    EM::PosID	pid0;
    EM::PosID	pid1;	//Only used in PosIDChange
    int		attrib; //Used only with AttribChange
    bool	flagfor2dviewer; //Used only with BurstAlert for 2DViewer
    CBID	cbID() const { return cbid_; }

protected:

    CBID			cbid_;
    static Threads::Atomic<int>	curcbid_;
};


/*!
\brief Iterator that iterates a number of positions (normally all) on an
EMObject. The object is created by EMObject::createIterator, and the next()
function is called until no more positions can be found.
*/

mExpClass(EarthModel) EMObjectIterator
{
public:
    virtual		~EMObjectIterator() {}
    virtual EM::PosID	next()		= 0;
			/*!<posid.objectID()==-1 when there are no more pids*/
    virtual int		approximateSize() const	{ return maximumSize(); }
    virtual int		maximumSize() const	{ return -1; }
    virtual bool	canGoTo() const		{ return false; }
    virtual EM::PosID	goTo(od_int64)		{ return EM::PosID(); }
};


/*!
\brief Position attribute
*/

mExpClass(EarthModel) PosAttrib
{
public:
			PosAttrib(){}

    enum Type		{ PermanentControlNode, TemporaryControlNode,
			  EdgeControlNode, TerminationNode, SeedNode,
			  IntersectionNode };

    Type		type_;
    TypeSet<PosID>	posids_;
    bool		locked_;
};


/*!
\brief Base class for all EarthModel objects.
*/

#define mImplEMSet(fnnm,typ,memb,emcbtype) \
    void fnnm( typ _set_to_ ) \
    {	EMObjectCallbackData* cbdata = getNewEMCBData(); \
	cbdata->event = emcbtype; \
	setMemberSimple( memb, _set_to_, 0, cbdata->cbID().getI() ); }
#define mImplEMGetSet(pfx,fnnmget,fnnmset,typ,memb,emcbtype) \
    pfx mImplSimpleMonitoredGet(fnnmget,typ,memb) \
    pfx mImplEMSet(fnnmset,const typ&,memb,emcbtype)


typedef EMObjectCallbackData::CBID EMCBID;

mExpClass(EarthModel) EMObject : public SharedObject
{
public:

    enum NodeSourceType		{ None = (int)'0', Manual=(int)'1',
				  Auto=(int)'2' };

    const DBKey&		id() const		{ return storageid_; }
    virtual const char*		getTypeStr() const	= 0;
    virtual uiString		getUserTypeStr() const	= 0;
    const DBKey&		dbKey() const		{ return storageid_; }
    void			setDBKey(const DBKey&);

    mImplEMGetSet(inline,preferredColor,setPreferredColor,Color,preferredcolor_,
		  EMObjectCallbackData::PrefColorChange)
    mImplEMGetSet(inline,selectionColor,setSelectionColor,Color,selectioncolor_,
		  EMObjectCallbackData::SelectionColorChange)
    mImplEMGetSet(inline,preferredLineStyle,setPreferredLineStyle,
		  OD::LineStyle, preferredlinestyle_,
		  EMObjectCallbackData::LineStyleChange)
    mImplEMGetSet(inline,preferredMarkerStyle3D,setPreferredMarkerStyle3D,
		  OD::MarkerStyle3D,preferredmarkerstyle_,
		  EMObjectCallbackData::MarkerStyleChange)

    virtual bool		isOK() const		{ return true; }

    uiString			uiName() const { return toUiString(name()); }
    virtual void		setNewName();

    virtual int			nrSections() const	= 0;
    virtual SectionID		sectionID(int) const	= 0;
    virtual BufferString	sectionName(const SectionID&) const;
    virtual bool		canSetSectionName() const;
    virtual bool		setSectionName(const SectionID&,const char*,
					       bool addtohistory);
    virtual int			sectionIndex(const SectionID&) const;
    virtual bool		removeSection(SectionID,bool hist )
							{ return false; }

    const Geometry::Element*	sectionGeometry(const SectionID&) const;
    Geometry::Element*		sectionGeometry(const SectionID&);

    void			setBurstAlert(bool yn);
    bool			hasBurstAlert() const;

    virtual Coord3		getPos(const EM::PosID&) const;
    virtual Coord3		getPos(const EM::SectionID&,
				       const EM::SubID&) const;
    virtual bool		isDefined(const EM::PosID&) const;
    virtual bool		isDefined(const EM::SectionID&,
					  const EM::SubID&) const;
    bool			setPos(const EM::PosID&,const Coord3&,
				       bool addtohistory,
				       NodeSourceType type=Auto);
    bool			setPos(const EM::SectionID&,const EM::SubID&,
				       const Coord3&,bool addtohistory,
				       NodeSourceType type=Auto);
    virtual bool		unSetPos(const EM::PosID&,bool addtohistory);
    virtual bool		unSetPos(const EM::SectionID&,const EM::SubID&,
					 bool addtohistory);

    virtual void		setNodeSourceType(const TrcKey&,
							NodeSourceType){}
    virtual bool		isNodeSourceType(const PosID&,
				    NodeSourceType) const {return false;}
    virtual bool		isNodeSourceType(const TrcKey&,
				     NodeSourceType)const {return false;}

    virtual void		setNodeLocked(const TrcKey&,bool locked){}
    virtual bool		isNodeLocked(const TrcKey&) const
					    { return false; }
    virtual bool		isNodeLocked(const PosID&)const {return false;}

    virtual void		lockAll() {}
    virtual void		unlockAll(){}
    virtual const Array2D<char>*
				getLockedNodes() const { return 0; }
    virtual void		setLockColor(const Color&) {}
    virtual const Color		getLockColor() const { return Color::Blue(); }
    virtual bool		hasLockedNodes() const {return haslockednodes_;}


    virtual bool		enableGeometryChecks(bool);
    virtual bool		isGeometryChecksEnabled() const;

    virtual bool		isAtEdge(const EM::PosID&) const;


    virtual void		getLinkedPos(const EM::PosID& posid,
					  TypeSet<EM::PosID>&) const
					{ return; }
				/*!< Gives positions on the object that are
				     linked to the posid given
				*/

    virtual EMObjectIterator*	createIterator(const EM::SectionID&,
					       const TrcKeyZSampling* =0) const
				{ return 0; }
				/*!< creates an iterator. If the sectionid is
				     -1, all sections will be traversed. */

    virtual int			nrPosAttribs() const;
    virtual int			posAttrib(int idx) const;
    virtual void		addPosAttrib(int attr);
    virtual void		removePosAttribList(int attr,
						    bool addtohistory=true);
    virtual void		setPosAttrib(const EM::PosID&,
				    int attr,bool yn,bool addtohistory=true);
				//!<Sets/unsets the posattrib depending on yn.
    virtual bool		isPosAttrib(const EM::PosID&,int attr) const;
    virtual const char*		posAttribName(int) const;
    virtual int			addPosAttribName(const char*);
    const TypeSet<PosID>*	getPosAttribList(int attr) const;
    const OD::MarkerStyle3D&	getPosAttrMarkerStyle(int attr);
    void			setPosAttrMarkerStyle(int attr,
						      const OD::MarkerStyle3D&);
    virtual void		lockPosAttrib(int attr,bool yn);
    virtual bool		isPosAttribLocked(int attr) const;
    virtual void		removeSelected(const Selector<Coord3>&,
					       TaskRunner*);
    void			removeListOfSubIDs(const TypeSet<EM::SubID>&,
						   const EM::SectionID&);
    void			removeAllUnSeedPos();
    const TrcKeyZSampling	getRemovedPolySelectedPosBox();
    void			emptyRemovedPolySelectedPosBox();

    CNotifier<EMObject,const EMObjectCallbackData&>	change;

    virtual Executor*		loader()		{ return 0; }
    virtual bool		isLoaded() const	{ return false; }
    virtual Executor*		saver()			{ return 0; }
    virtual bool		isChanged() const	{ return changed_; }
    virtual bool		isEmpty() const;
    virtual void		setChangedFlag()	{ changed_=true; }
    virtual void		resetChangedFlag()	{ changed_=false; }
    bool			isFullyLoaded() const	{ return fullyloaded_; }
    void			setFullyLoaded(bool yn) { fullyloaded_=yn; }

    virtual bool		isLocked() const	{ return locked_; }
    virtual void		lock(bool yn)		{ locked_=yn;}

    bool			isInsideSelRemoval() const
				{ return insideselremoval_; }
    bool			isSelRemoving() const	{ return selremoving_; }

    uiString			errMsg() const;
    void			setErrMsg(const uiString& m) { errmsg_ = m; }

    virtual bool		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    void			saveDisplayPars() const;

    static int			sTerminationNode();
    static int			sSeedNode();
    static int			sIntersectionNode();

    static ChangeType		cPositionChange()	{ return 2; }
    static ChangeType		cPrefColorChange()	{ return 3; }
    static ChangeType		cSelColorChange()	{ return 4; }
    static ChangeType		cPrefLineStyleChange()	{ return 5; }
    static ChangeType		cPrefMarkerStyleChange(){ return 6; }

    virtual const IOObjContext&	getIOObjContext() const = 0;


    const EMObjectCallbackData*	getEMCBData(EMCBID) const;

protected:
				~EMObject();
				EMObject(const char*);
				//!<must be called after creation

    virtual bool		setPosition(const EM::SectionID&,
					    const EM::SubID&,
					    const Coord3&,bool addtohistory,
					    NodeSourceType type=Auto);
    virtual Geometry::Element*	sectionGeometryInternal(const SectionID&);
    virtual void		prepareForDelete();
    void			posIDChangeCB(CallBacker*);
    void			useDisplayPars(const IOPar&);

    EMObjectCallbackData*	getNewEMCBData();

    BufferString		objname_;
    DBKey			storageid_;
    uiString			errmsg_;

    Color&			preferredcolor_;
    Color&			selectioncolor_;
    OD::LineStyle&		preferredlinestyle_;
    OD::MarkerStyle3D&		preferredmarkerstyle_;
    ObjectSet<PosAttrib>	posattribs_;
    TypeSet<int>		attribs_;

    TrcKeyZSampling		removebypolyposbox_;

    bool			changed_;
    bool			fullyloaded_;
    bool			locked_;
    int				burstalertcount_;
    Threads::Lock		setposlock_;

    bool			insideselremoval_;
    bool			selremoving_;
    bool			haslockednodes_;

    ObjectSet<EMObjectCallbackData> emcbdatas_;

    static const char*		nrposattrstr();
    static const char*		posattrprefixstr();
    static const char*		posattrsectionstr();
    static const char*		posattrposidstr();

public:

    mDeprecated const DBKey&	multiID() const		{ return storageid_; }
    mDeprecated void		setMultiID( const DBKey& k ) { setDBKey(k); }
    static Color		sDefaultSelectionColor();
};

} // namespace EM

#define mDefineEMObjFuncs( clss ) \
public: \
				clss(const char* nm); \
    static void			initClass(); \
    static EMObject*		create(EM::EMManager&); \
    static clss*		create(const char* nm); \
    static FixedString		typeStr(); \
    const char*			getTypeStr() const; \
    void			setNewName(); \
protected: \
				~clss()

#define mImplementEMObjFuncs( clss, typenm ) \
void clss::initClass() \
{ \
    EMOF().addCreator( create, typeStr() ); \
} \
 \
 \
EMObject* clss::create( EM::EMManager& emm ) \
{ \
    EMObject* obj = new clss(""); \
    if ( !obj ) return 0; \
    obj->ref();         \
    emm.addObject( obj ); \
    obj->unRefNoDelete(); \
    return obj; \
} \
\
clss* clss::create( const char* nm ) \
{ \
    EMObject* emobj = EMM().createObject( typeStr(), nm ); \
    mDynamicCastGet(clss*,newobj,emobj); \
    return newobj; \
} \
\
FixedString clss::typeStr() { return typenm; } \
const char* clss::getTypeStr() const { return typeStr(); } \
void clss::setNewName() \
{\
    static int objnr = 1; \
    BufferString nm( "<New ", typenm, " " ); \
    nm.add( objnr++ ).add( ">" ); \
    setName( nm ); \
}


#define mSendEMCBNotifPosID( typ, pid ) \
    setChangedFlag(); \
    EMObjectCallbackData* cbdata = getNewEMCBData(); \
    cbdata->event = typ; \
    cbdata->pid0 = pid; \
    mSendChgNotif( cPositionChange(), cbdata->cbID().getI() );

#define mSendEMCBNotif( typ ) \
    mSendEMCBNotifPosID( typ, EM::PosID() );
