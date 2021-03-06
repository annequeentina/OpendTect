/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra / Bert Bril
 Date:		Sep 2005 / Nov 2006
________________________________________________________________________

-*/

#include "uichangesurfacedlg.h"

#include "uiarray2dchg.h"
#include "uitaskrunner.h"
#include "uihorsavefieldgrp.h"
#include "uiioobjsel.h"
#include "uiseparator.h"
#include "uimsg.h"
#include "undo.h"

#include "array2dinterpol.h"
#include "array2dfilter.h"
#include "ioobjctxt.h"
#include "emhorizon3d.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "executor.h"
#include "od_helpids.h"


uiChangeHorizonDlg::uiChangeHorizonDlg( uiParent* p, EM::Horizon* hor,
					bool is2d, const uiString& txt )
    : uiDialog (p, Setup(txt,mNoDlgTitle,
			 mODHelpKey(mChangeSurfaceDlgHelpID) ) )
    , horizon_( hor )
    , is2d_( is2d )
    , savefldgrp_( 0 )
    , inputfld_( 0 )
    , parsgrp_( 0 )
    , horReadyForDisplay( this )
{
    setCtrlStyle( RunAndClose );

    if ( horizon_ )
	horizon_->ref();
    else
    {
	IOObjContext ctxt = is2d ? EMHorizon2DTranslatorGroup::ioContext()
				 : EMHorizon3DTranslatorGroup::ioContext();
	ctxt.forread_ = true;
	inputfld_ =
	    new uiIOObjSel( this, ctxt,
			    uiStrings::phrInput(uiStrings::sHorizon(1)) );
    }

    savefldgrp_ = new uiHorSaveFieldGrp( this, horizon_ );
    savefldgrp_->setSaveFieldName( "Save interpolated horizon" );
}


void uiChangeHorizonDlg::attachPars()
{
    if ( !parsgrp_ ) return;

    if ( inputfld_ )
	parsgrp_->attach( alignedBelow, inputfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, parsgrp_ );

    savefldgrp_->attach( alignedBelow, parsgrp_ );
    savefldgrp_->attach( ensureBelow, sep );
}


uiChangeHorizonDlg::~uiChangeHorizonDlg()
{
    if ( horizon_ ) horizon_->unRef();
}


#define mErrRet(msg) { if ( msg ) uiMSG().error( msg ); return false; }


bool uiChangeHorizonDlg::readHorizon()
{
    if ( !inputfld_->ctxtIOObj().ioobj_ )
	return false;

    const DBKey mid = inputfld_->ctxtIOObj().ioobj_->key();
    EM::Horizon* hor = savefldgrp_->readHorizon( mid );
    if ( !hor ) return false;

    if ( horizon_ ) horizon_->unRef();
    horizon_ = hor;
    horizon_->ref();

    return true;
}


bool uiChangeHorizonDlg::doProcessing()
{
    return is2d_ ? doProcessing2D() : doProcessing3D();
}

bool uiChangeHorizonDlg::doProcessing2D()
{ // TODO
    return false;
}


bool uiChangeHorizonDlg::doProcessing3D()
{
    MouseCursorChanger chgr( MouseCursor::Wait );
    bool change = false;
    EM::Horizon* usedhor = savefldgrp_->getNewHorizon() ?
       savefldgrp_->getNewHorizon() : horizon_;
    mDynamicCastGet(EM::Horizon3D*,usedhor3d,usedhor)

    mDynamicCastGet(EM::Horizon3D*,hor3d,horizon_)
    if ( !hor3d )
	return false;

    for ( int idx=0; idx<hor3d->geometry().nrSections(); idx++ )
    {
	const EM::SectionID sid = hor3d->geometry().sectionID( idx );
	if ( !idx && needsFullSurveyArray() )
	    savefldgrp_->setFullSurveyArray( true );

	PtrMan<Array2D<float> > arr = hor3d->createArray2D( sid );
	if ( !arr )
	{
	    uiString msg = tr("Not enough horizon data for section %1")
			 .arg(sid);
	    ErrMsg( msg.getFullString() ); continue;
	}

	PtrMan<Executor> worker = getWorker( *arr,
			hor3d->geometry().rowRange(sid),
			hor3d->geometry().colRange(sid,-1) );
	if ( !worker ) return false;

	uiTaskRunner dlg( this );
	if ( !TaskRunner::execute( &dlg, *worker ) )
	    return false;

	if ( !usedhor3d )
	    return false;
	const EM::SectionID usedsid = usedhor3d->geometry().sectionID( idx );
	if ( !usedhor3d->setArray2D(*arr,usedsid,fillUdfsOnly(),
				    undoText(),false) )
	{
	    uiString msg = tr("Cannot set new data to section %1").arg(usedsid);
	    ErrMsg( msg.getFullString() ); continue;
	}
	else if ( usedhor3d==hor3d )
	{
	    change = true;
	}
    }

    if ( change )
	EM::EMM().undo(usedhor3d->id()).setUserInteractionEnd(
	EM::EMM().undo(usedhor3d->id()).lastEventID());

    return true;
}


bool uiChangeHorizonDlg::acceptOK()
{
    if ( inputfld_ && !inputfld_->commitInput() )
    {
	uiMSG().error( uiStrings::phrSelect(mJoinUiStrs(
				    sInput().toLower(),sHorizon().toLower())) );
	return false;
    }


    if ( !horizon_ && !readHorizon() )
    {
	uiMSG().error(uiStrings::phrCannotRead(
					    uiStrings::sHorizon().toLower()));
	return false;
    }

    if ( !savefldgrp_->acceptOK() )
	return false;

    if ( !doProcessing() )
	return false;

    if ( saveFldGrp()->displayNewHorizon() || !saveFldGrp()->getNewHorizon() )
	horReadyForDisplay.trigger();

    const bool res = savefldgrp_->saveHorizon();
    if ( res )
	uiMSG().message( tr("Horizon sucessfully changed") );

    return false;
}


//---- uiFilterHorizonDlg

uiFilterHorizonDlg::uiFilterHorizonDlg( uiParent* p, EM::Horizon* hor )
    : uiChangeHorizonDlg(p,hor,false,uiStrings::phrJoinStrings(
			 uiStrings::sHorizon(1),tr("filtering")))
{
    Array2DFilterPars filterpars( Stats::Median );
    parsgrp_ = new uiArr2DFilterPars( this, &filterpars );
    attachPars();
}


Executor* uiFilterHorizonDlg::getWorker( Array2D<float>& a2d,
					   const StepInterval<int>& rowrg,
					   const StepInterval<int>& colrg )
{
    Array2DFilterPars pars = ((uiArr2DFilterPars*)parsgrp_)->getInput();
    return new Array2DFilterer<float>( a2d, pars );
}
