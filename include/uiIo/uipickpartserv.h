#ifndef uipickpartserv_h
#define uipickpartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Feb 2002
 RCS:           $Id: uipickpartserv.h,v 1.32 2007-08-21 05:35:26 cvsraman Exp $
________________________________________________________________________

-*/

#include "uipicksetmgr.h"
#include "uiapplserv.h"
#include "ranges.h"
#include "multiid.h"
#include "binidvalset.h"
#include "bufstringset.h"
#include "segposinfo.h"

class Color;
class IOObj;
class BinIDRange;
class SurfaceInfo;
class RandLocGenPars;
namespace Pick { class Set; class SetMgr; };


/*! \brief Service provider for application level - seismics */

class uiPickPartServer  : public uiApplPartServer
			, public uiPickSetMgr
{
public:
				uiPickPartServer(uiApplService&);
				~uiPickPartServer();

    const char*			name() const		{ return "Picks"; }

				// Services
    void			managePickSets();
    Pick::Set*			pickSet()		{ return ps_; }
    void			impexpSet(bool import);
    void			fetchHors(bool);
    bool			loadSets();	//!< Load set(s) by user sel
    bool                        createSet(bool);
    void			setMisclassSet(const BinIDValueSet&);
    void			fillZValsFrmHor(Pick::Set*,int);

    static const int		evGetHorInfo2D;
    static const int		evGetHorInfo3D;
    static const int		evGetHorDef;
    static const int            evFillPickSet;
    static const int		evGet2DLineInfo;
    static const int		evGet2DLineDef;


				// Interaction stuff
    BinIDValueSet&			genDef()	{ return gendef_; }
    ObjectSet<PosInfo::Line2DData>&	lineGeoms()	{ return linegeoms_; }

    ObjectSet<SurfaceInfo>& 	horInfos()		{ return hinfos_; }
    const ObjectSet<MultiID>&	selHorIDs() const	{ return selhorids_; }
    const BinIDRange*		selBinIDRange() const	{ return selbr_; }
    MultiID			horID()			{ return horid_; }
    BufferStringSet&		lineSets()		{ return linesets_; }
    TypeSet<BufferStringSet>&	lineNames()		{ return linenms_; }
    TypeSet<MultiID>&		lineSetIds()		{ return setids_; }
    MultiID&			lineSetID()		{ return setid_; }
    BufferStringSet&		selectLines()		{ return selectlines_; }

    uiParent*			parent()
    				{ return appserv().parent(); }

protected:

    BinIDValueSet 			gendef_;
    ObjectSet<PosInfo::Line2DData>	linegeoms_;

    ObjectSet<SurfaceInfo> 	hinfos_;
    ObjectSet<MultiID>		selhorids_;
    BufferStringSet		linesets_;
    TypeSet<BufferStringSet>	linenms_;
    TypeSet<MultiID>		setids_;
    MultiID			setid_;
    BufferStringSet		selectlines_;
    const BinIDRange*		selbr_;
    Pick::Set*			ps_;
    MultiID			horid_;

    bool			mkRandLocs3D(Pick::Set&,const RandLocGenPars&);
    bool                        mkRandLocs2D(Pick::Set&,const RandLocGenPars&);
};


#endif
