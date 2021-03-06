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
#include "stratseisevent.h"
#include "uistring.h"
class uiCheckBox;
class uiGenInput;
class uiLabel;
class uiStratLevelSel;


mExpClass(uiWellAttrib) uiStratSeisEvent : public uiGroup
{ mODTextTranslationClass(uiStratSeisEvent);
public:

    typedef Strat::Level::ID	LevelID;

    mExpClass(uiWellAttrib) Setup
    {
    public:
			Setup( bool wew=false )
			    : withextrwin_(wew)
			    , allowlayerbased_(false)	    {}

	mDefSetupMemb(LevelID,fixedlevelid)
	mDefSetupMemb(bool,withextrwin)
	mDefSetupMemb(bool,allowlayerbased)
    };

			uiStratSeisEvent(uiParent*,const Setup&);

    bool		getFromScreen();
    void		setLevel(const char* lvlnm);
    void		setLevels(const BufferStringSet);
    void		putToScreen();
    BufferString	levelName() const;
    bool		doAllLayers() const;
    bool		hasExtrWin() const;
    bool		hasStep() const;

    Strat::SeisEvent&	event()		{ return ev_; }
			// step may be undefined
    const StepInterval<float> getFullExtrWin() const;

protected:

    Strat::SeisEvent	ev_;
    Setup		setup_;

    uiStratLevelSel*	levelfld_;
    uiGenInput*		evfld_;
    EnumDefImpl<VSEvent::Type>	evtype_;
    uiGenInput*		snapoffsfld_;
    uiGenInput*		extrwinfld_;
    uiCheckBox*		usestepfld_;
    uiGenInput*		extrstepfld_;
    uiLabel*		nosteplbl_;
    uiGenInput*		uptolvlfld_;

    void		evSnapCheck(CallBacker*);
    void		extrWinCB(CallBacker*);
    void		stopAtCB(CallBacker*);
    void		stepSelCB(CallBacker*);

};
