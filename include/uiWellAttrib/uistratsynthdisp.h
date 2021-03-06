#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Nov 2010
________________________________________________________________________

-*/

#include "uiwellattribmod.h"
#include "uigroup.h"
#include "uiflatviewslicepos.h"
#include "uistring.h"
#include "stratsynthlevelset.h"
#include "uistratsimplelaymoddisp.h"

class TimeDepthModel;
class SeisTrcBuf;
class StratSynth;
class SyntheticData;
class PropertyRef;
class PropertyRefSelection;
class TaskRunner;
class Wavelet;
class uiButton;
class uiComboBox;
class uiFlatViewer;
class uiMultiFlatViewControl;
class uiPushButton;
class uiSynthGenDlg;
class uiWaveletIOObjSel;
class uiSynthSlicePos;
class uiToolButton;
class uiToolButtonSetup;
namespace Strat { class LayerModel; class LayerModelProvider; }
namespace FlatView { class AuxData; }
namespace PreStackView { class uiSyntheticViewer2DMainWin; }


mExpClass(uiWellAttrib) uiStratSynthDisp : public uiGroup
{ mODTextTranslationClass(uiStratSynthDisp);
public:
    typedef TypeSet<float> LVLZVals;
    typedef TypeSet< LVLZVals > LVLZValsSet;

			uiStratSynthDisp(uiParent*,
					 const Strat::LayerModelProvider&);
			~uiStratSynthDisp();

    const Strat::LayerModel& layerModel() const;
    const char*		levelName(const int idx=0) const;
    DBKey		waveletID() const;
    const Wavelet*	getWavelet() const;
    inline const StratSynth& curSS() const
			{ return *(!useed_ ? stratsynth_ : edstratsynth_); }
    inline StratSynth&	curSS()
			{ return *(!useed_ ? stratsynth_ : edstratsynth_); }
    inline const StratSynth& altSS() const
			{ return *(useed_ ? stratsynth_ : edstratsynth_); }
    const StratSynth&	normalSS() const	{ return *stratsynth_; }
    const StratSynth&	editSS() const		{ return *edstratsynth_; }

    const ObjectSet<SyntheticData>& getSynthetics() const;
    RefMan<SyntheticData>	getCurrentSyntheticData(bool wva=true) const;
    RefMan<SyntheticData>	getSyntheticData(const char* nm);
    const PropertyRefSelection&	modelPropertyRefs() const;

    const ObjectSet<const TimeDepthModel>* d2TModels() const;

    void		setFlattened(bool flattened,bool trigger=true);
    void		setDispMrkrs(const BufferStringSet,
				const uiStratLayerModelDisp::LVLZValsSet&);
    void		setSelectedTrace(int);
    void		setDispEach(int);
    void		setZDataRange(const Interval<double>&,bool indpt);
    void		setDisplayZSkip(float zskip,bool withmodchg);

    const uiWorldRect&	curView(bool indepth) const;
    void		setZoomView(const uiWorldRect&);

    uiFlatViewer*	viewer()		{ return vwr_; }

    Notifier<uiStratSynthDisp>	wvltChanged;
    Notifier<uiStratSynthDisp>	viewChanged;
    Notifier<uiStratSynthDisp>	layerPropSelNeeded;
    Notifier<uiStratSynthDisp>	modSelChanged;
    Notifier<uiStratSynthDisp>	synthsChanged;
    Notifier<uiStratSynthDisp>	dispParsChanged;

    void		addTool(const uiToolButtonSetup&);
    void		addViewerToControl(uiFlatViewer&);

    void		modelChanged();
    bool		haveUserScaleWavelet();
    void		displaySynthetic(ConstRefMan<SyntheticData>);
    void		reDisplayPostStackSynthetic(bool wva=true);
    void		cleanSynthetics();
    float		centralTrcShift() const;
    void		setCurrentSynthetic(bool wva);
    void		setSnapLevelSensitive(bool);
    bool		prepareElasticModel();

    uiMultiFlatViewControl* control()	{ return control_; }

    void		fillPar(IOPar&) const;
    void		fillPar(IOPar&,bool) const;
    bool		usePar(const IOPar&);
    void		makeInfoMsg(uiString& msg,IOPar&);

    void		showFRResults();
    void		setBrineFilled( bool yn ) { isbrinefilled_ = yn; }
    void		setAutoUpdate( bool yn )  { autoupdate_ = yn; }
    void		setForceUpdate( bool yn ) { forceupdate_ = yn; }
    bool		doForceUpdate() const	  { return forceupdate_; }
    void		setUseEdited( bool yn )	  { useed_ = yn; }
    bool		isEditUsed() const	  { return useed_; }
    void		setDiffData();
    void		resetRelativeViewRect();
    void		updateRelativeViewRect();
    void		setRelativeViewRect(const uiWorldRect& relwr);
    const uiWorldRect&	getRelativeViewRect() const	{ return relzoomwr_; }
    void		setSavedViewRect();
    void		setFlattenLvl(const Strat::Level& lvl)
			{ flattenlvl_ = lvl; }
    const Strat::Level& getFlattenLvl() { return flattenlvl_; }

protected:

    int					longestaimdl_;
    StratSynth*				stratsynth_;
    StratSynth*				edstratsynth_;
    const Strat::LayerModelProvider&	lmp_;
    uiWorldRect				relzoomwr_;
    mutable uiWorldRect			savedzoomwr_;
    int					selectedtrace_;
    int					dispeach_;
    float				dispskipz_;
    bool				dispflattened_;
    bool				isbrinefilled_;
    bool				autoupdate_;
    bool				forceupdate_;
    bool				useed_;

    const ObjectSet<const TimeDepthModel>* d2tmodels_;
    RefMan<SyntheticData>		currentwvasynthetic_;
    RefMan<SyntheticData>		currentvdsynthetic_;

    uiMultiFlatViewControl*		control_;
    FlatView::AuxData*			selectedtraceaux_;
    ObjectSet<FlatView::AuxData>	levelaux_;

    uiGroup*				topgrp_;
    uiGroup*				datagrp_;
    uiGroup*				prestackgrp_;
    uiWaveletIOObjSel*			wvltfld_;
    uiFlatViewer*			vwr_;
    uiPushButton*			scalebut_;
    uiButton*				lasttool_;
    uiToolButton*			prestackbut_;
    uiComboBox*				wvadatalist_;
    uiComboBox*				vddatalist_;
    uiComboBox*				levelsnapselfld_;
    uiSynthGenDlg*			synthgendlg_;
    uiSynthSlicePos*			offsetposfld_;
    PtrMan<TaskRunner>			taskrunner_;

    PreStackView::uiSyntheticViewer2DMainWin*	prestackwin_;

    void		showInfoMsg(bool foralt);
    void		handleFlattenChange();
    void		setCurrentWavelet();
    void		fillPar(IOPar&,const StratSynth*) const;
    void		doModelChange();
    const SeisTrcBuf&	curTrcBuf() const;
    void		getCurD2TModel(ConstRefMan<SyntheticData>,
				    ObjectSet<const TimeDepthModel>&,
				    float offset = 0.0f) const;
    void		reSampleTraces(ConstRefMan<SyntheticData>,
				       SeisTrcBuf&) const;
    void		updateFields();
    void		updateSynthetic(const char* nm,bool wva);
    void		updateSyntheticList(bool wva);
    void		copySyntheticDispPars();
    inline StratSynth&	altSS()
			{ return *(useed_ ? stratsynth_ : edstratsynth_); }

    void		drawLevel();
    void		displayFRText();
    void		displayPreStackSynthetic(ConstRefMan<SyntheticData>);
    void		displayPostStackSynthetic(ConstRefMan<SyntheticData>,
						  bool wva=true);
    void		setPreStackMapper();
    void		setAbsoluteViewRect(const uiWorldRect& abswr);
    void		getAbsoluteViewRect(uiWorldRect& abswr) const;

    void		addEditSynth(CallBacker*);
    void		exportSynth(CallBacker*);
    void		wvDataSetSel(CallBacker*);
    void		vdDataSetSel(CallBacker*);
    void		levelSnapChanged(CallBacker*);
    void		layerPropsPush(CallBacker*);
    void		offsetChged(CallBacker*);
    void		scalePush(CallBacker*);
    void		genNewSynthetic(CallBacker*);
    void		viewPreStackPush(CallBacker*);
    void		wvltChg(CallBacker*);
    void		viewChg(CallBacker*);
    void		parsChangedCB(CallBacker*);
    void		syntheticRemoved(CallBacker*);
    void		syntheticDisabled(CallBacker*);
    void		syntheticChanged(CallBacker*);
    void		selPreStackDataCB(CallBacker*);
    void		preStackWinClosedCB(CallBacker*);
    Strat::Level	flattenlvl_;
};


mExpClass(uiWellAttrib) uiSynthSlicePos : public uiGroup
{ mODTextTranslationClass(uiSynthSlicePos);
public:
			uiSynthSlicePos(uiParent*,const uiString& lbltxt);

    Notifier<uiSynthSlicePos>	positionChg;
    void		setLimitSampling(StepInterval<float>);
    int			getValue() const;
    void		setValue(int) const;

protected:
    uiLabel*		label_;
    uiSpinBox*		sliceposbox_;
    uiSpinBox*		slicestepbox_;
    uiToolButton*	prevbut_;
    uiToolButton*	nextbut_;

    void		slicePosChg( CallBacker* );
    void		prevCB(CallBacker*);
    void		nextCB(CallBacker*);

    StepInterval<float>	limitsampling_;
};
