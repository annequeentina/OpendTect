#ifndef uiempartserv_h
#define uiempartserv_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        A.H. Bril
 Date:          Sep 2002
 RCS:           $Id: uiempartserv.h,v 1.23 2003-12-18 12:45:15 nanne Exp $
________________________________________________________________________

-*/

#include "uiapplserv.h"
#include "ranges.h"
class BinIDRange;
class BinIDValue;
class BinIDZValue;
class BufferString;
class MultiID;
class uiPopupMenu;
class SurfaceInfo;
class BinID;

namespace EM { class SurfaceIODataSelection; };

/*! \brief Service provider for application level - seismics */

class uiEMPartServer : public uiApplPartServer
{
public:
			uiEMPartServer(uiApplService&);
			~uiEMPartServer();

    const char*		name() const			{ return "EarthModel"; }

			// Services
    bool		importHorizon();
    bool		exportHorizon();

    BufferString	getName(const MultiID&) const;

    bool		selectHorizon(MultiID& id)
    			{ return selectSurface(id,true); }
    bool		selectFault(MultiID& id)
    			{ return selectSurface(id,false); }
    bool		selectSurface(MultiID&,bool);
    bool		loadAuxData(const MultiID&,int);
    bool		loadAuxData(const MultiID&,const char*);
    int			createAuxDataSubMenu(uiPopupMenu&,int,const MultiID&,
	    				     bool);

    bool		importLMKFault();

    bool		selectStickSet(MultiID&);
    bool		createStickSet(MultiID&);

    void		manageSurfaces();
    bool		loadSurface(const MultiID&,
	    			    const EM::SurfaceIODataSelection* s=0);
    void		getSurfaceInfo(ObjectSet<SurfaceInfo>&);
    void		getSurfaceDef(const ObjectSet<MultiID>&,
	    			      TypeSet<BinID>&,
				      TypeSet<Interval<float> >&,
				      const BinIDRange* br=0) const;

    bool		storeObject(const MultiID&);
    void		setDataVal(const MultiID&,
	    			   ObjectSet< TypeSet<BinIDZValue> >&,
				   const char*);
    bool		getDataVal(const MultiID&,
	    			   ObjectSet< TypeSet<BinIDZValue> >&,
				   BufferString&,float&);

    static const int	evDisplayHorizon;

			// Interaction stuff
    const MultiID&	selEMID() const			{ return selemid_; }

protected:

    MultiID&		selemid_;

    bool		ioHorizon(bool);
    
};


#endif
