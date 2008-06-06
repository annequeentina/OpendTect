#ifndef visfaultdisplay_h
#define visfaultdisplay_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: visfaultdisplay.h,v 1.10 2008-06-06 16:58:33 cvskris Exp $
________________________________________________________________________


-*/

#include "vismultiattribsurvobj.h"

#include "emposid.h"
#include "ranges.h"


class DataPointSet;

namespace visBase
{
    class GeomIndexedShape;
    class Transformation;
    class PickStyle;
    class ShapeHints;
    class IndexedPolyLine3D;
};

namespace EM { class Fault; }
namespace MPE { class FaultEditor; }
namespace Geometry { class ExplFaultStickSurface; class ExplPlaneIntersection; }


namespace visSurvey
{
class MPEEditor;

/*!\brief 


*/

class FaultDisplay : public MultiTextureSurveyObject
{
public:
    static FaultDisplay*	create()
				mCreateDataObj(FaultDisplay);

    MultiID			getMultiID() const;
    bool			isInlCrl() const	{ return false; }

    virtual int			nrResolutions() const;
    virtual void		setResolution(int);

    SurveyObject::AttribFormat	getAttributeFormat() const
				{ return SurveyObject::RandomPos; }
    void			getRandomPos(DataPointSet&) const;
    void			setRandomPosData(int,const DataPointSet*); 

    bool			hasColor() const		{ return true; }
    Color			getColor() const;
    void			setColor(Color);
    bool			allowMaterialEdit() const	{ return true; }
    NotifierAccess*		materialChange();

    void			useTexture( bool yn, bool trigger );
    bool			usesTexture() const;
    void			setDepthAsAttrib(int);

    void			showManipulator(bool);
    bool			isManipulatorShown() const;

    void			setDisplayTransformation(mVisTrans*);
    mVisTrans*			getDisplayTransformation();
    void			setRightHandSystem(bool);

    void			setSceneEventCatcher(visBase::EventCatcher*);

    void			display(bool sticks,bool panels);
    bool			areSticksDisplayed() const;
    bool			arePanelsDisplayed() const;

    virtual void                fillPar(IOPar&,TypeSet<int>&) const;
    virtual int                 usePar(const IOPar&);

    bool			setEMID(const EM::ObjectID&);
    EM::ObjectID		getEMID() const;

    void			displayIntersections(bool yn);
    bool			areIntersectionsDisplayed() const;

    Notifier<FaultDisplay>	colorchange;

protected:

    virtual			~FaultDisplay();
    void			otherObjectsMoved(
	    			    const ObjectSet<const SurveyObject>&,
				    int whichobj);
    void			setRandomPosDataInternal(int attrib,
	    						 const DataPointSet*,
							 int column); 

    void			updateStickDisplay();
    void			updateSingleColor();
    void			updateManipulator();

    virtual bool		getCacheValue(int attrib,int version,
					      const Coord3&,float&) const;
    virtual void		addCache();
    virtual void		removeCache(int);
    virtual void		swapCache(int,int);
    virtual void		emptyCache(int);
    virtual bool		hasCache(int) const;

    static const char*		sKeyEarthModelID()	{ return "EM ID"; }

    void			mouseCB(CallBacker*);
    void			emChangeCB(CallBacker*);

    void 			updateNearestStickMarker();

    visBase::EventCatcher*		eventcatcher_;
    visBase::Transformation*		displaytransform_;
    visBase::ShapeHints*		shapehints_;

    visBase::GeomIndexedShape*		paneldisplay_;
    Geometry::ExplFaultStickSurface*	explicitpanels_;

    visBase::GeomIndexedShape*		stickdisplay_;
    Geometry::ExplFaultStickSurface*	explicitsticks_;

    visBase::GeomIndexedShape*		intersectiondisplay_;
    Geometry::ExplPlaneIntersection*	explicitintersections_;
    ObjectSet<const SurveyObject>	intersectionobjs_;
    TypeSet<int>			planeids_;

    visBase::PickStyle*			neareststickmarkerpickstyle_;
    visBase::IndexedPolyLine3D*		neareststickmarker_;
    int					neareststick_;

    EM::Fault*				emfault_;
    MPE::FaultEditor*			faulteditor_;
    visSurvey::MPEEditor*		viseditor_;

    Coord3				mousepos_;

    TypeSet<DataPack::ID>		datapackids_;
    bool				showmanipulator_;

    bool				validtexture_;
    Color				nontexturecol_;
    bool				usestexture_;

    bool				displaysticks_;
};

};


#endif
