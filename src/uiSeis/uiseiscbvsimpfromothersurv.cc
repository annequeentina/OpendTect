/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bruno
 Date:          Oct 2010
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseiscbvsimpfromothersurv.h"

#include "ctxtioobj.h"
#include "seiscbvsimpfromothersurv.h"

#include "uibutton.h"
#include "uigeninput.h"
#include "uispinbox.h"
#include "uimsg.h"
#include "uiseissubsel.h"
#include "uiseissel.h"
#include "uiselobjothersurv.h"
#include "uiseparator.h"
#include "uitaskrunner.h"
#include "od_helpids.h"


static const char* interpols[] = { "Sinc interpolation", "Nearest trace", 0 };

uiSeisImpCBVSFromOtherSurveyDlg::uiSeisImpCBVSFromOtherSurveyDlg( uiParent* p )
    : uiDialog(p,Setup(tr("Import CBVS cube from other survey"),
		       tr("Specify import parameters"),
		       mODHelpKey(mSeisImpCBVSFromOtherSurveyDlgHelpID))
		 .modal(false))
    , import_(0)
{
    setCtrlStyle( RunAndClose );

    finpfld_ = new uiGenInput( this, tr("CBVS file name") );
    finpfld_->setReadOnly();
    CallBack cb = mCB(this,uiSeisImpCBVSFromOtherSurveyDlg,cubeSel);
    uiPushButton* selbut = new uiPushButton( this, m3Dots(uiStrings::sSelect()),
                                             cb, true );
    selbut->attach( rightOf, finpfld_ );

    subselfld_ = new uiSeis3DSubSel( this, Seis::SelSetup( false ) );
    subselfld_->attach( alignedBelow, finpfld_ );

    uiSeparator* sep1 = new uiSeparator( this, "sep" );
    sep1->attach( stretchedBelow, subselfld_ );

    interpfld_ = new uiGenInput( this, tr("Interpolation"),
			    BoolInpSpec( true, interpols[0], interpols[1] ) );
    interpfld_->valuechanged.notify(
		mCB(this,uiSeisImpCBVSFromOtherSurveyDlg,interpSelDone) );
    interpfld_->attach( ensureBelow, sep1 );
    interpfld_->attach( alignedBelow, subselfld_ );

    cellsizefld_ = new uiLabeledSpinBox(this, tr("Lateral stepout (Inl/Crl)"));
    cellsizefld_->attach( alignedBelow, interpfld_ );
    cellsizefld_->box()->setInterval( 2, 12, 2 );
    cellsizefld_->box()->setValue( 8 );

    uiSeparator* sep2 = new uiSeparator( this, "sep" );
    sep2->attach( stretchedBelow, cellsizefld_ );

    outfld_ = new uiSeisSel( this, uiSeisSel::ioContext(Seis::Vol,false),
			     uiSeisSel::Setup(Seis::Vol) );
    outfld_->attach( alignedBelow, cellsizefld_ );
    outfld_->attach( ensureBelow, sep2 );

    interpSelDone(0);
}


void uiSeisImpCBVSFromOtherSurveyDlg::interpSelDone( CallBacker* )
{
    const bool curitm = interpfld_->getBoolValue() ? 0 : 1;
    interpol_ = (SeisImpCBVSFromOtherSurvey::Interpol)(curitm);
    issinc_ = interpol_ == SeisImpCBVSFromOtherSurvey::Sinc;
    cellsizefld_->display( issinc_ );
}


void uiSeisImpCBVSFromOtherSurveyDlg::cubeSel( CallBacker* )
{
    CtxtIOObj inctio( uiSeisSel::ioContext( Seis::Vol, true ) );
    uiSelObjFromOtherSurvey objdlg( this, inctio );
    if ( objdlg.go() && inctio.ioobj )
    {
	if ( import_ ) delete import_;
	import_ = new SeisImpCBVSFromOtherSurvey( *inctio.ioobj );
	BufferString fusrexp; objdlg.getIOObjFullUserExpression( fusrexp );
	if ( import_->prepareRead( fusrexp ) )
	{
	    finpfld_->setText( fusrexp );
	    subselfld_->setInput( import_->cubeSampling() );
	}
	else
	    { delete import_; import_ = 0; }
    }
}


#define mErrRet(msg ) { uiMSG().error( msg ); return false; }
bool uiSeisImpCBVSFromOtherSurveyDlg::acceptOK( CallBacker* )
{
    if ( !import_ )
	mErrRet( tr("No valid input, please select a new input file") )

    const IOObj* outioobj = outfld_->ioobj();
    if ( !outioobj )
	return false;
    int cellsz = issinc_ ? cellsizefld_->box()->getValue() : 0;
    TrcKeyZSampling cs; subselfld_->getSampling( cs );
    import_->setPars( interpol_, cellsz, cs );
    import_->setOutput( const_cast<IOObj&>(*outioobj) );
    uiTaskRunner taskrunner( this );
    return TaskRunner::execute( &taskrunner, *import_ );
}
