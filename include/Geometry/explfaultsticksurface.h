#ifndef explfaultsticksurface_h
#define explfaultsticksurface_h

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        J.C. Glas
 Date:          October 2007
 RCS:           $Id: explfaultsticksurface.h,v 1.15 2010-07-27 08:56:57 cvsjaap Exp $
________________________________________________________________________

-*/

#include "indexedshape.h"
#include "position.h"
#include "rowcol.h"
#include "datapack.h"

class DataPointSet;

namespace Geometry
{

class FaultStickSurface;
class ExplFaultStickTexturePositionExtracter;

/*!A triangulated representation of a faultsticksurface */


mClass ExplFaultStickSurface: public Geometry::IndexedShape,
			     public CallBacker
{
public:
			ExplFaultStickSurface(FaultStickSurface*,float zscale);
    			~ExplFaultStickSurface();

    bool		needsUpdate() const 		{ return needsupdate_; }

    void		setSurface(FaultStickSurface*);
    FaultStickSurface*	getSurface()			{ return surface_; }
    const FaultStickSurface* getSurface() const		{ return surface_; }

    void		setZScale( float );

    void		display(bool sticks,bool panels);
    bool		areSticksDisplayed() const    { return displaysticks_; }
    bool		arePanelsDisplayed() const    { return displaypanels_; }

    bool		createsNormals() const 		{ return true; }
    bool		createsTextureCoords() const 	{ return true; }

    void		setMaximumTextureSize(int);
    void		setTexturePowerOfTwo(bool yn);
    void		setTextureSampling(const BinIDValue&);
    const RowCol&	getTextureSize() const;
    void		needUpdateTexture(bool yn);
    bool		needsUpdateTexture() const;
    void		setRightHandedNormals(bool yn);


    bool		getTexturePositions(DataPointSet&,
	    				    TaskRunner*);
    const BinIDValue	getBinIDValue() { return texturesampling_; }

    static const char*  sKeyTextureI() { return "Fault texture i column"; }
    static const char*  sKeyTextureJ() { return "Fault texture j column"; }

protected:
    friend		class ExplFaultStickSurfaceUpdater;    
    friend		class ExplFaultStickTexturePositionExtracter;    

    void		removeAll();
    void		insertAll();
    bool		update(bool forceall,TaskRunner*);
    
    void		addToGeometries(IndexedGeometry*);
    void		removeFromGeometries(const IndexedGeometry*);

    void		emptyStick(int stickidx);
    void		fillStick(int stickidx);
    void		removeStick(int stickidx);
    void		insertStick(int stickidx);

    void		emptyPanel(int panelidx);
    void		fillPanel(int panelidx);
    void		removePanel(int panelidx);
    void		insertPanel(int panelidx);

    void		surfaceChange(CallBacker*);
    void		surfaceMovement(CallBacker*);

    void		updateTextureCoords();
    bool		updateTextureSize();
    int			textureColSz(const int panelidx);
    int			sampleSize(const Coord3& p0,const Coord3& p1);
    int			point2LineSampleSz(const Coord3& point,
	    				   const Coord3& linept0,
					   const Coord3& linept1);
    Coord3		getCoord(int stickidx,int texturerow) const;
    float		getAvgDistance(int stickidx,
	    			const TypeSet<int>& shift,int extra) const;
    void		shiftStick(int stickidx,int nrunits);
    void		updateStickShifting();

    bool		displaysticks_;
    bool		displaypanels_;

    FaultStickSurface*	surface_;
    Coord3		scalefacs_;

    bool					needsupdate_;
    bool					needsupdatetexture_;

    ObjectSet<IndexedGeometry>			sticks_;
    ObjectSet<IndexedGeometry>			paneltriangles_;
    ObjectSet<IndexedGeometry>			panellines_;

    BoolTypeSet					stickhidden_;
    TypeSet<int>				texturecolcoords_;
    ObjectSet< TypeSet<int> >			textureknotcoords_;	
    int						maximumtexturesize_;
    RowCol					texturesize_;
    bool					texturepot_;
    BinIDValue					texturesampling_;
};

};

#endif
