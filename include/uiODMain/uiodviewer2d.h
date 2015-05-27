#ifndef uiodviewer2d_h
#define uiodviewer2d_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        A.H. Bril
 Date:          Dec 2003
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiodmainmod.h"
#include "trckeyzsampling.h"
#include "datapack.h"
#include "emposid.h"
#include "uigeom.h"
#include "uistring.h"

class uiFlatViewAuxDataEditor;
class uiFlatViewStdControl;
class uiFlatViewWin;
class uiMainWin;
class uiODMain;
class uiODVw2DTreeTop;
class uiParent;
class uiSlicePos2DView;
class uiToolBar;
class uiTreeFactorySet;
class MouseCursorExchange;
class Vw2DDataManager;
class ZAxisTransform;

namespace Attrib { class SelSpec; }
namespace FlatView { class AuxData; }

/*!
\brief A 2D Viewer.
*/

mExpClass(uiODMain) uiODViewer2D : public CallBacker
{ mODTextTranslationClass(uiODViewer2D);
public:
				uiODViewer2D(uiODMain&,int visid);
				~uiODViewer2D();

    mDeclInstanceCreatedNotifierAccess(uiODViewer2D);

    virtual void		setUpView(DataPack::ID,bool wva);
    void			setSelSpec(const Attrib::SelSpec*,bool wva);
    void			setMouseCursorExchange(MouseCursorExchange*);

    uiParent*			viewerParent();
    uiFlatViewWin*		viewwin()		{ return viewwin_; }
    const uiFlatViewWin*	viewwin() const		{ return viewwin_; }
    Vw2DDataManager*		dataMgr()		{ return datamgr_; }
    const Vw2DDataManager*	dataMgr() const		{ return datamgr_; }

    uiODVw2DTreeTop*		treeTop()		{ return treetp_; }

    const uiTreeFactorySet*	uiTreeItemFactorySet() const { return tifs_; }

    const ObjectSet<uiFlatViewAuxDataEditor>&	dataEditor()
				{ return auxdataeditors_; }

    Attrib::SelSpec&		selSpec( bool wva )
				{ return wva ? wvaselspec_ : vdselspec_; }
    const Attrib::SelSpec&	selSpec( bool wva ) const
				{ return wva ? wvaselspec_ : vdselspec_; }
    DataPack::ID		getDataPackID(bool wva) const;
				/*!<Returns DataPack::ID of specified display if
				it has a valid one. Returns DataPack::ID of
				other display if both have same Attrib::SelSpec.
				Else, returns uiODViewer2D::createDataPack.*/
    DataPack::ID		createDataPack(bool wva) const
				{ return createDataPack(selSpec(wva)); }
    DataPack::ID		createDataPack(const Attrib::SelSpec&) const;
				/*!< Creates RegularFlatDataPack by getting
				TrcKeyZSampling from slicepos_. Uses the
				existing TrcKeyZSampling, if there is no
				slicepos_. Also transforms data if the 2D Viewer
				hasZAxisTransform(). */
    DataPack::ID		createFlatDataPack(DataPack::ID,int comp) const;
				/*!< Creates a FlatDataPack from SeisDataPack.
				Either a transformed or a non-transformed
				datapack can be passed. The returned datapack
				will always be in transformed domain if the
				viewer hasZAxisTransform(). */
    bool			useStoredDispPars(bool wva);

    ZAxisTransform*		getZAxisTransform() const
				{ return datatransform_; }
    bool			setZAxisTransform(ZAxisTransform*);
    bool			hasZAxisTransform() const
				{ return datatransform_; }
    void			setTrcKeyZSampling(const TrcKeyZSampling&);
    const TrcKeyZSampling&	getTrcKeyZSampling() const
				{ return tkzs_; }
    Pos::GeomID			geomID() const;

    void			setInitialCentre( const uiWorldPoint& wp )
				{ initialcentre_ = wp; }
    void			setInitialX1PosPerCM( float val )
				{ initialx1pospercm_ = val; }
    void			setInitialX2PosPerCM( float val )
				{ initialx2pospercm_ = val; }
    void			setUpAux();

    const uiFlatViewStdControl* viewControl() const
				{ return viewstdcontrol_; }
    uiFlatViewStdControl*	viewControl()
				{ return viewstdcontrol_; }
    uiSlicePos2DView*		slicePos()
				{ return slicepos_; }
    uiODMain&			appl_;

    int				id_; /*!<Unique identifier */
    int				visid_; /*!<ID from 3D visualization */
    BufferString		basetxt_;

    virtual void		usePar(const IOPar&);
    virtual void		fillPar(IOPar&) const;
    virtual void		setWinTitle(bool fromcs=false);
				/*!<\param fromcs if true, TrcKeyZSampling
				will be used to set window title.*/

    static const char*		sKeyVDSelSpec()  { return "VD SelSpec"; }
    static const char*		sKeyWVASelSpec() { return "WVA SelSpec"; }
    static const char*		sKeyPos()	 { return "Position"; }

    Notifier<uiODViewer2D>	viewWinAvailable;
    Notifier<uiODViewer2D>	viewWinClosed;
    Notifier<uiODViewer2D>	dataChanged;

protected:

    uiSlicePos2DView*				slicepos_;
    uiFlatViewStdControl*			viewstdcontrol_;
    ObjectSet<uiFlatViewAuxDataEditor>		auxdataeditors_;

    Attrib::SelSpec&		wvaselspec_;
    Attrib::SelSpec&		vdselspec_;

    Vw2DDataManager*		datamgr_;
    uiTreeFactorySet*		tifs_;
    uiODVw2DTreeTop*		treetp_;
    uiFlatViewWin*		viewwin_;
    MouseCursorExchange*	mousecursorexchange_;
    FlatView::AuxData*		marker_;
    ZAxisTransform*		datatransform_;
    TrcKeyZSampling		tkzs_;

    uiWorldPoint		initialcentre_;
    float			initialx1pospercm_;
    float			initialx2pospercm_;

    int				polyseltbid_;
    bool			ispolyselect_;

    DataPack::ID		createDataPackForTransformedZSlice(
						const Attrib::SelSpec&) const;

    virtual void		createViewWin(bool isvert,bool needslicepos);
    virtual void		createTree(uiMainWin*);
    virtual void		createPolygonSelBut(uiToolBar*);
    void			createViewWinEditors();
    void			setDataPack(DataPack::ID,bool wva,bool isnew);
    virtual void		setPos(const TrcKeyZSampling&);
    void			adjustOthrDisp(bool wva,bool isnew);
    void			removeAvailablePacks();
    void			rebuildTree();

    void			winCloseCB(CallBacker*);
    void			winPoppedUpCB(CallBacker*);
    void			posChg(CallBacker*);
    void			selectionMode(CallBacker*);
    void			handleToolClick(CallBacker*);
    void			removeSelected(CallBacker*);
    void			mouseCursorCB(CallBacker*);
    void			mouseMoveCB(CallBacker*);
};

#endif

