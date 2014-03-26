/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          January 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiprobdenfuncman.h"

#include "uiprobdenfuncgen.h"
#include "uieditpdf.h"

#include "uilistbox.h"
#include "uitextedit.h"
#include "uiioobjsel.h"
#include "uiioobjmanip.h"
#include "uimsg.h"

#include "bufstring.h"
#include "sampledprobdenfunc.h"
#include "simpnumer.h"
#include "probdenfunctr.h"

static const int cPrefWidth = 75;
static const float cMaxProbVal = 100.0f;

uiProbDenFuncMan::uiProbDenFuncMan( uiParent* p )
    : uiObjFileMan(p,uiDialog::Setup("Manage Probability Density Functions",
				     mNoDlgTitle,
				     "112.1.0").nrstatusflds(1),
	           ProbDenFuncTranslatorGroup::ioContext())
{
    createDefaultUI();
    selgrp_->getListField()->doubleClicked.notify(
				mCB(this,uiProbDenFuncMan,browsePush) );

    uiIOObjManipGroup* manipgrp = selgrp_->getManipGroup();
    manipgrp->addButton( "browseprdf",
			 "Browse/edit this Probability Density Function",
			 mCB(this,uiProbDenFuncMan,browsePush) );
    manipgrp->addButton( "genprdf",
			 "Generate Probability Density Function",
			 mCB(this,uiProbDenFuncMan,genPush) );

    selgrp_->setPrefWidthInChar( mCast(float,cPrefWidth) );
    infofld_->setPrefWidthInChar( cPrefWidth );
    selChg( this );
}



uiProbDenFuncMan::~uiProbDenFuncMan()
{
}


#define mGetPDF(pdf) \
    PtrMan<ProbDenFunc> pdf = ProbDenFuncTranslator::read( *curioobj_ )


void uiProbDenFuncMan::browsePush( CallBacker* )
{
    if ( !curioobj_ ) return;
    mGetPDF(pdf);
    if ( !pdf ) return;

    uiEditProbDenFunc dlg( this, *pdf, true );
    if ( dlg.go() && dlg.isChanged() )
    {
	const int choice = uiMSG().question( "PDF changed. Save?", sYes(),
						"&As new ...", sNo() );
	if ( choice < 0 ) return;

	PtrMan<IOObj> saveioobj = curioobj_->clone();
	if ( choice == 0 )
	{
	    CtxtIOObj ctio( ctxt_ );
	    ctio.ctxt.forread = false;
	    uiIOObjSelDlg seldlg( this, ctio, "Save As" );
	    if ( !seldlg.go() || !seldlg.ioObj() ) return;
	    saveioobj = seldlg.ioObj()->clone();
	}

	BufferString emsg;
	if ( !ProbDenFuncTranslator::write(*pdf,*saveioobj,&emsg) )
	    uiMSG().error( emsg );
	else
	    selgrp_->fullUpdate( saveioobj->key() );
    }
}


void uiProbDenFuncMan::genPush( CallBacker* )
{
    uiProbDenFuncGen dlg( this );
    if ( dlg.go() )
	selgrp_->fullUpdate( dlg.newObjKey() );
}


void uiProbDenFuncMan::mkFileInfo()
{
    if ( !curioobj_ ) { setInfo( "" ); return; }

    BufferString txt;
    txt += getFileInfo();

    mGetPDF(pdf);
    if ( pdf )
    {
	IOPar par;
	pdf->fillPar( par );
	txt += BufferString( "Type: ", pdf->getTypeStr() );
	for ( int idx=0; idx<pdf->nrDims(); idx++ )
	{
	    BufferString lbl( "\nDimension ", idx+1, ": " );
	    txt += lbl; txt += pdf->dimName(idx);
	}
    }
    setInfo( txt );
}
