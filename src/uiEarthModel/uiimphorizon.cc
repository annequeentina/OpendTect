/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Nanne Hemstra
 Date:		June 2002
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiimphorizon.h"

#include "uiarray2dinterpol.h"
#include "uicombobox.h"
#include "uicompoundparsel.h"
#include "uibutton.h"
#include "uicolor.h"
#include "uitaskrunner.h"
#include "uifileinput.h"
#include "uigeninputdlg.h"
#include "uiioobjsel.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uipossubsel.h"
#include "uiscaler.h"
#include "uiseparator.h"
#include "uistratlvlsel.h"
#include "uistrings.h"
#include "uitblimpexpdatasel.h"
#include "uitoolbutton.h"

#include "arrayndimpl.h"
#include "array2dinterpolimpl.h"
#include "binidvalset.h"
#include "ctxtioobj.h"
#include "emhorizon3d.h"
#include "emhorizonascio.h"
#include "emmanager.h"
#include "emsurfacetr.h"
#include "emsurfaceauxdata.h"
#include "file.h"
#include "filepath.h"
#include "horizonscanner.h"
#include "ioman.h"
#include "ioobj.h"
#include "keystrs.h"
#include "oddirs.h"
#include "pickset.h"
#include "randcolor.h"
#include "strmprov.h"
#include "surfaceinfo.h"
#include "survinfo.h"
#include "tabledef.h"
#include "od_helpids.h"

#include <math.h>

static const char* sZVals = "Z values";

static BufferString sImportFromPath;


void uiImportHorizon::initClass()
{ sImportFromPath = GetDataDir(); }


uiImportHorizon::uiImportHorizon( uiParent* p, bool isgeom )
    : uiDialog(p,uiDialog::Setup(uiStrings::sEmptyString(),mNoDlgTitle,
				 mODHelpKey(mImportHorAttribHelpID) )
				 .modal(false))
    , ctio_(*mMkCtxtIOObj(EMHorizon3D))
    , isgeom_(isgeom)
    , filludffld_(0)
    , interpol_(0)
    , colbut_(0)
    , stratlvlfld_(0)
    , displayfld_(0)
    , fd_(*EM::Horizon3DAscIO::getDesc())
    , scanner_(0)
    , importReady(this)
{
    setCaption( isgeom ? tr("Import Horizon") : tr("Import Horizon Data") );
    setOkCancelText( uiStrings::sImport(), uiStrings::sClose() );
    setDeleteOnClose( false );
    ctio_.ctxt.forread = !isgeom_;

    BufferString fltr( "Text (*.txt *.dat);;XY/IC (*.*xy* *.*ic* *.*ix*)" );
    inpfld_ = new uiFileInput( this, "Input ASCII File",
		uiFileInput::Setup(uiFileDialog::Gen)
		.withexamine(true).forread(true).filter(fltr)
		.defseldir(sImportFromPath) );
    inpfld_->setSelectMode( uiFileDialog::ExistingFiles );
    inpfld_->valuechanged.notify( mCB(this,uiImportHorizon,inputChgd) );

    OD::ChoiceMode mode =
	isgeom ? OD::ChooseZeroOrMore : OD::ChooseAtLeastOne;
    uiLabeledListBox* attrllb =
	new uiLabeledListBox( this, tr("Attribute(s) to import"), mode,
			      uiLabeledListBox::LeftTop );
    attrllb->attach( alignedBelow, inpfld_ );
    attrlistfld_ = attrllb->box();
    attrlistfld_->setNrLines( 6 );
    attrlistfld_->itemChosen.notify( mCB(this,uiImportHorizon,inputChgd) );

    uiToolButton* addbut = new uiToolButton( this, "addnew", tr("Add new"),
				mCB(this,uiImportHorizon,addAttribCB) );
    addbut->attach( rightTo, attrllb );
    uiToolButton* rmbut = new uiToolButton( this, "stop",
					    uiStrings::sRemove(true),
				mCB(this,uiImportHorizon,rmAttribCB) );
    rmbut->attach( alignedBelow, addbut );
    uiToolButton* clearbut = new uiToolButton( this, "clear", tr("Clear list"),
				mCB(this,uiImportHorizon,clearListCB) );
    clearbut->attach( alignedBelow, rmbut );

    uiSeparator* sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, attrllb );

    dataselfld_ = new uiTableImpDataSel( this, fd_,
		  mODHelpKey(mTableImpDataSel3DSurfacesHelpID) );
    dataselfld_->attach( alignedBelow, attrllb );
    dataselfld_->attach( ensureBelow, sep );
    dataselfld_->descChanged.notify( mCB(this,uiImportHorizon,descChg) );

    scanbut_ = new uiPushButton( this, tr("Scan Input File"),
				 mCB(this,uiImportHorizon,scanPush), true );
    scanbut_->attach( alignedBelow, dataselfld_);

    sep = new uiSeparator( this, "H sep" );
    sep->attach( stretchedBelow, scanbut_ );

    subselfld_ = new uiPosSubSel( this, uiPosSubSel::Setup(false,false) );
    subselfld_->attach( alignedBelow, attrllb );
    subselfld_->attach( ensureBelow, sep );
    subselfld_->setSensitive( false );

    outputfld_ = new uiIOObjSel( this, ctio_ );
    outputfld_->setLabelText( isgeom_ ? tr("Output Horizon")
				      : tr("Add to Horizon") );

    if ( !isgeom_ )
    {
	fd_.setName( EM::Horizon3DAscIO::sKeyAttribFormatStr() );
	outputfld_->attach( alignedBelow, subselfld_ );
    }
    else
    {
	setHelpKey(mODHelpKey(mImportHorizonHelpID) );
	filludffld_ = new uiGenInput( this, tr("Fill undefined parts"),
				      BoolInpSpec(true) );
	filludffld_->valuechanged.notify(mCB(this,uiImportHorizon,fillUdfSel));
	filludffld_->setValue(false);
	filludffld_->setSensitive( false );
	filludffld_->attach( alignedBelow, subselfld_ );
	interpolparbut_ = new uiPushButton( this, uiStrings::sSettings(true),
	       mCB(this,uiImportHorizon,interpolSettingsCB), false );
	interpolparbut_->attach( rightOf, filludffld_ );

	outputfld_->attach( alignedBelow, filludffld_ );

	stratlvlfld_ = new uiStratLevelSel( this, true );
	stratlvlfld_->attach( alignedBelow, outputfld_ );
	stratlvlfld_->selChange.notify( mCB(this,uiImportHorizon,stratLvlChg) );

	colbut_ = new uiColorInput( this,
				    uiColorInput::Setup(getRandStdDrawColor())
				    .lbltxt(tr("Base color")) );
	colbut_->attach( alignedBelow, stratlvlfld_ );

	displayfld_ = new uiCheckBox( this, tr("Display after import") );
	displayfld_->attach( alignedBelow, colbut_ );

	fillUdfSel(0);
    }

    postFinalise().notify( mCB(this,uiImportHorizon,inputChgd) );
}


uiImportHorizon::~uiImportHorizon()
{
    delete ctio_.ioobj; delete &ctio_;
    delete interpol_;
}


void uiImportHorizon::descChg( CallBacker* cb )
{
    if ( scanner_ ) delete scanner_;
    scanner_ = 0;
}


void uiImportHorizon::interpolSettingsCB( CallBacker* )
{
    uiSingleGroupDlg dlg( this, uiDialog::Setup(tr("Interpolation settings"),
			  (const char*) 0, mNoHelpKey ) );

    uiArray2DInterpolSel* arr2dinterpfld =
	new uiArray2DInterpolSel( &dlg, true, true, false, interpol_ );
    arr2dinterpfld->setDistanceUnit( SI().xyInFeet() ? tr("[ft]") : tr("[m]") );
    dlg.setGroup( arr2dinterpfld );

    if ( dlg.go() )
    {
	delete interpol_;
	interpol_ = arr2dinterpfld->getResult();
    }
}


void uiImportHorizon::inputChgd( CallBacker* cb )
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
    if ( isgeom_ ) attrnms.insertAt( new BufferString(sZVals), 0 );
    const int nrattrib = attrnms.size();
    const bool keepdef = cb==inpfld_ && fd_.isGood();
    if ( !keepdef )
    {
	EM::Horizon3DAscIO::updateDesc( fd_, attrnms );
	dataselfld_->updateSummary();
    }
    dataselfld_->setSensitive( nrattrib );

    const FixedString fnm = inpfld_->fileName();
    scanbut_->setSensitive( !fnm.isEmpty() && nrattrib );
    if ( !scanner_ )
    {
	subselfld_->setSensitive( false );
	if ( filludffld_ )
	    filludffld_->setSensitive( false );
    }
    else
    {
	delete scanner_;
	scanner_ = 0;
    }

    FilePath fnmfp( fnm );
    sImportFromPath = fnmfp.pathOnly();
    if ( isgeom_ )
	outputfld_->setInputText( fnmfp.baseName() );
}


void uiImportHorizon::addAttribCB( CallBacker* )
{
    uiGenInputDlg dlg( this, "Add Attribute", uiStrings::sName(),
		       new StringInpSpec() );
    if ( !dlg.go() ) return;

    const char* attrnm = dlg.text();
    attrlistfld_->addItem( attrnm );
    attrlistfld_->setChosen( attrlistfld_->size()-1, true );
}


void uiImportHorizon::rmAttribCB( CallBacker* )
{
    if ( attrlistfld_->isEmpty() )
	return;

    int selidx = attrlistfld_->currentItem();
    const bool updatedef = attrlistfld_->isChosen( selidx );

    attrlistfld_->removeItem( selidx );
    selidx--;
    if ( selidx < 0 ) selidx = 0;
    attrlistfld_->setChosen( selidx );

    if ( updatedef )
	inputChgd( 0 );
}


void uiImportHorizon::clearListCB( CallBacker* )
{
    const bool updatedef = attrlistfld_->nrChosen() > 0;
    attrlistfld_->setEmpty();

    if ( updatedef )
	inputChgd( 0 );
}


void uiImportHorizon::scanPush( CallBacker* )
{
    if ( !isgeom_ && !attrlistfld_->nrChosen() )
	{ uiMSG().error(tr("Please select at least one attribute")); return; }
    if ( !dataselfld_->commit() || !doScan() )
	return;

    if ( isgeom_ )
    {
	filludffld_->setSensitive( scanner_->gapsFound(true) ||
				   scanner_->gapsFound(false) );
	fillUdfSel(0);
    }

    subselfld_->setSensitive( true );

    scanner_->launchBrowser();
}


    #define mNotCompatibleRet(ic) \
    const int df = n##ic##lnrg.start - ic##rg.start; \
    if ( df%2 && !(ic##rg.step%2) && !(n##ic##lnrg.step%2) ) \
    { \
	uiString msg = "The horizon is not compatible with survey " \
		       "trace, do you want to continue?"; \
	if ( !uiMSG().askGoOn(msg) ) \
	    return false; \
    }


bool uiImportHorizon::doScan()
{
    BufferStringSet filenms;
    if ( !getFileNames(filenms) ) return false;

    scanner_ = new HorizonScanner( filenms, fd_, isgeom_ );
    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *scanner_ ) )
	return false;

    const StepInterval<int> nilnrg = scanner_->inlRg();
    const StepInterval<int> nclnrg = scanner_->crlRg();
    TrcKeyZSampling cs( true );
    const StepInterval<int> irg = cs.hrg.inlRange();
    const StepInterval<int> crg = cs.hrg.crlRange();
    if ( irg.start>nilnrg.stop || crg.start>nclnrg.stop ||
	 irg.stop<nilnrg.start || crg.stop<nclnrg.start )
	uiMSG().warning( tr("Your horizon is out of the survey range.") );
    else if ( irg.step > 1 )
    {
	mNotCompatibleRet(i);
    }
    else if ( crg.step > 1 )
    {
	mNotCompatibleRet(c);
    }

    if ( nilnrg.step==0 || nclnrg.step==0 )
    {
	uiMSG().error( "Cannot have '0' as a step value" );
	return false;
    }

    cs.hrg.set( nilnrg, nclnrg );
    subselfld_->setInput( cs );
    return true;
}


void uiImportHorizon::fillUdfSel( CallBacker* )
{
    if ( interpolparbut_ )
    {
	interpolparbut_->display( filludffld_->getBoolValue() );
	if ( !interpol_ && filludffld_->getBoolValue() )
	{
	    InverseDistanceArray2DInterpol* templ =
		new InverseDistanceArray2DInterpol;
	    templ->setSearchRadius( 10*(SI().inlDistance()+SI().crlDistance()));
	    templ->setFillType( Array2DInterpol::ConvexHull );
	    interpol_ = templ;
	}
    }
}


bool uiImportHorizon::doDisplay() const
{
    return displayfld_ && displayfld_->isChecked();
}


MultiID uiImportHorizon::getSelID() const
{
    MultiID mid = ctio_.ioobj ? ctio_.ioobj->key() : -1;
    return mid;
}


void uiImportHorizon::stratLvlChg( CallBacker* )
{
    if ( !stratlvlfld_ ) return;
    const Color col( stratlvlfld_->getColor() );
    if ( col != Color::NoColor() )
	colbut_->setColor( col );
}

#define mErrRet(s) { uiMSG().error(s); return 0; }
#define mErrRetUnRef(s) { horizon->unRef(); mErrRet(s) }
#define mSave(taskrunner) \
    if ( !exec ) \
    { \
	delete exec; \
	horizon->unRef(); \
	return false; \
    } \
    rv = TaskRunner::execute( &taskrunner, *exec ); \
    delete exec;

bool uiImportHorizon::doImport()
{
    BufferStringSet attrnms;
    attrlistfld_->getChosen( attrnms );
    if ( isgeom_ )
	attrnms.insertAt( new BufferString(sZVals), 0 );
    if ( attrnms.isEmpty() )
	mErrRet( tr("No Attributes Selected") );

    EM::Horizon3D* horizon = isgeom_ ? createHor() : loadHor();
    if ( !horizon ) return false;

    if ( !scanner_ && !doScan() )
	return false;

    if ( scanner_->nrPositions() == 0 )
    {
	uiString msg( "No valid positions found\n"
		      "Please re-examine input file and format definition" );
	mErrRetUnRef( msg );
    }

    ManagedObjectSet<BinIDValueSet> sections;
    deepCopy( sections, scanner_->getSections() );

    if ( sections.isEmpty() )
	mErrRetUnRef( tr("Nothing to import") );

    const bool dofill = filludffld_ && filludffld_->getBoolValue();
    if ( dofill )
    {
	if ( !interpol_ )
	    mErrRetUnRef( tr("No interpolation selected") );
	fillUdfs( sections );
    }

    TrcKeySampling hs = subselfld_->envelope().hrg;
    if ( hs.lineRange().step==0 || hs.trcRange().step==0 )
	mErrRetUnRef( tr("Cannot have '0' as a step value") )
    ExecutorGroup importer( "Importing horizon" );
    importer.setNrDoneText( tr("Nr positions done") );
    int startidx = 0;
    if ( isgeom_ )
    {
	importer.add( horizon->importer(sections,hs) );
	attrnms.removeSingle( 0 );
	startidx = 1;
    }

    if ( attrnms.size() )
	importer.add( horizon->auxDataImporter(sections,attrnms,startidx,hs) );

    uiTaskRunner taskrunner( this );
    const bool success = TaskRunner::execute( &taskrunner, importer );
    if ( !success )
	mErrRetUnRef(tr("Cannot import horizon"))

    bool rv;
    if ( isgeom_ )
    {
	Executor* exec = horizon->saver();
	mSave(taskrunner);
    }
    else
    {
	mDynamicCastGet(ExecutorGroup*,exec,horizon->auxdata.auxDataSaver(-1))
	mSave(taskrunner);
    }

    if ( !doDisplay() )
	horizon->unRef();
    else
	horizon->unRefNoDelete();

    return rv;
}


bool uiImportHorizon::acceptOK( CallBacker* )
{
    if ( !checkInpFlds() ) return false;

    if ( !doImport() )
	return false;

    if ( isgeom_ )
    {
	const IOObj* ioobj = outputfld_->ioobj();
	if ( ioobj )
	{
	    ioobj->pars().update( sKey::CrFrom(), inpfld_->fileName() );
	    ioobj->updateCreationPars();
	    IOM().commitChanges( *ioobj );
	}
    }

    uiMSG().message( tr("Horizon successfully imported") );
    if ( doDisplay() )
	importReady.trigger();

    return false;
}


bool uiImportHorizon::getFileNames( BufferStringSet& filenames ) const
{
    if ( !*inpfld_->fileName() )
	mErrRet( tr("Please select input file(s)") )

    inpfld_->getFileNames( filenames );
    for ( int idx=0; idx<filenames.size(); idx++ )
    {
	const char* fnm = filenames[idx]->buf();
	if ( !File::exists(fnm) )
	{
	    uiString errmsg = tr("Cannot find input file:\n%1")
			    .arg(fnm);
	    deepErase( filenames );
	    mErrRet( errmsg );
	}
    }

    return true;
}


bool uiImportHorizon::checkInpFlds()
{
    BufferStringSet filenames;
    if ( !getFileNames(filenames) || !dataselfld_->commit() )
	return false;

    const char* outpnm = outputfld_->getInput();
    if ( !outpnm || !*outpnm )
	mErrRet( tr("Please select output horizon") )

    if ( !outputfld_->commitInput() )
	return false;

    return true;
}


bool uiImportHorizon::fillUdfs( ObjectSet<BinIDValueSet>& sections )
{
    if ( !interpol_ )
	return false;
    TrcKeySampling hs = subselfld_->envelope().hrg;

    const float inldist = SI().inlDistance();
    const float crldist = SI().crlDistance();
    interpol_->setRowStep( inldist*hs.step.inl() );
    interpol_->setColStep( crldist*hs.step.crl());
    uiTaskRunner taskrunner( this );
    Array2DImpl<float> arr( hs.nrInl(), hs.nrCrl() );
    if ( !arr.isOK() )
	return false;

    for ( int idx=0; idx<sections.size(); idx++ )
    {
	arr.setAll( mUdf(float) );
	BinIDValueSet& data = *sections[idx];
	BinID bid;
	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl() = hs.start.inl() + inl*hs.step.inl();
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl() = hs.start.crl() + crl*hs.step.crl();
		BinIDValueSet::SPos pos = data.find( bid );
		if ( pos.j >= 0 )
		{
		    const float* vals = data.getVals( pos );
		    if ( vals )
			arr.set( inl, crl, vals[0] );
		}
	    }
	}

	if ( !interpol_->setArray( arr, &taskrunner ) )
	    return false;

	if ( !TaskRunner::execute( &taskrunner, *interpol_ ) )
	    return false;

	for ( int inl=0; inl<hs.nrInl(); inl++ )
	{
	    bid.inl() = hs.start.inl() + inl*hs.step.inl();
	    for ( int crl=0; crl<hs.nrCrl(); crl++ )
	    {
		bid.crl() = hs.start.crl() + crl*hs.step.crl();
		BinIDValueSet::SPos pos = data.find( bid );
		if ( pos.j >= 0 ) continue;

		TypeSet<float> vals( data.nrVals(), mUdf(float) );
		vals[0] = arr.get( inl, crl );
		data.add( bid, vals.arr() );
	    }
	}
    }

    return true;
}


EM::Horizon3D* uiImportHorizon::createHor() const
{
    const char* horizonnm = outputfld_->getInput();
    EM::EMManager& em = EM::EMM();
    const MultiID mid = getSelID();
    EM::ObjectID objid = em.getObjectID( mid );
    if ( objid < 0 )
	objid = em.createObject( EM::Horizon3D::typeStr(), horizonnm );

    mDynamicCastGet(EM::Horizon3D*,horizon,em.getObject(objid));
    if ( !horizon )
	mErrRet( uiStrings::sCantCreateHor() );

    horizon->change.disable();
    horizon->setMultiID( mid );
    horizon->setStratLevelID( stratlvlfld_->getID() );
    horizon->setPreferredColor( colbut_->color() );
    horizon->ref();
    return horizon;
}


EM::Horizon3D* uiImportHorizon::loadHor()
{
    EM::EMManager& em = EM::EMM();
    EM::EMObject* emobj = em.createTempObject( EM::Horizon3D::typeStr() );
    emobj->setMultiID( ctio_.ioobj->key() );
    Executor* loader = emobj->loader();
    if ( !loader ) mErrRet( uiStrings::sCantLoadHor());

    uiTaskRunner taskrunner( this );
    if ( !TaskRunner::execute( &taskrunner, *loader ) )
	return 0;

    mDynamicCastGet(EM::Horizon3D*,horizon,emobj)
    if ( !horizon ) mErrRet( tr("Error loading horizon"));

    horizon->ref();
    delete loader;
    return horizon;
}

