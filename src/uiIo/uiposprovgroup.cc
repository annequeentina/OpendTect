/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2008
________________________________________________________________________

-*/


#include "uiposprovgroupstd.h"
#include "uigeninput.h"
#include "uifilesel.h"
#include "uipicksetsel.h"
#include "uiselsurvranges.h"
#include "uimsg.h"
#include "trckeyzsampling.h"
#include "ioobjctxt.h"
#include "file.h"
#include "ioobj.h"
#include "iopar.h"
#include "keystrs.h"
#include "oddirs.h"
#include "survinfo.h"
#include "uistrings.h"

mImplFactory2Param(uiPosProvGroup,uiParent*,const uiPosProvGroup::Setup&,
		   uiPosProvGroup::factory);


uiPosProvGroup::uiPosProvGroup( uiParent* p, const uiPosProvGroup::Setup& su )
    : uiPosFiltGroup(p,su)
{
}


uiRangePosProvGroup::uiRangePosProvGroup( uiParent* p,
					  const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , hrgfld_(0)
    , nrrgfld_(0)
    , zrgfld_(0)
    , setup_(su)
{
    uiObject* attobj = 0;
    if ( su.is2d_ )
    {
	nrrgfld_ =
	    new uiSelNrRange( this, uiSelNrRange::Gen, su.withstep_ );
	nrrgfld_->setRange( su.tkzs_.hsamp_.crlRange() );
	attobj = nrrgfld_->attachObj();
    }
    else
    {
	hrgfld_ = new uiSelHRange( this, su.tkzs_.hsamp_, su.withstep_ );
	attobj = hrgfld_->attachObj();
    }

    if ( setup_.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, su.tkzs_.zsamp_, su.withstep_,
				   0, su.zdomkey_ );
	if ( attobj )
	    zrgfld_->attach( alignedBelow, attobj );
	attobj = zrgfld_->attachObj();
    }

    if ( attobj ) setHAlignObj( attobj );
}


void uiRangePosProvGroup::usePar( const IOPar& iop )
{
    TrcKeyZSampling cs; getTrcKeyZSampling( cs );
    cs.usePar( iop );

    if ( hrgfld_ )
	hrgfld_->setSampling( cs.hsamp_ );
    if ( zrgfld_ )
	zrgfld_->setRange( cs.zsamp_ );
    if ( nrrgfld_ )
    {
	StepInterval<int> trcrg = cs.hsamp_.crlRange();
	iop.get( IOPar::compKey(sKey::TrcRange(),0), trcrg );
	nrrgfld_->setRange( trcrg );
	if ( zrgfld_ )
	{
	    StepInterval<float> zrg = cs.zsamp_;
	    iop.get( IOPar::compKey(sKey::ZRange(),0), zrg );
	    zrgfld_->setRange( zrg );
	}
    }
}


bool uiRangePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Range() );
    TrcKeyZSampling cs; getTrcKeyZSampling( cs );

    if ( setup_.is2d_ )
    {
	iop.set( IOPar::compKey(sKey::TrcRange(),0), cs.hsamp_.crlRange() );
	if ( setup_.withz_ )
	    iop.set( IOPar::compKey(sKey::ZRange(),0), cs.zsamp_ );
	return true;
    }

    cs.fillPar( iop );
    return true;
}


void uiRangePosProvGroup::getSummary( BufferString& txt ) const
{
    TrcKeyZSampling cs; getTrcKeyZSampling( cs );
    txt += setup_.withz_ ? "Sub-volume" : "Sub-area";
}


static void getExtrDefTrcKeyZSampling( TrcKeyZSampling& cs )
{
    int nrsamps = cs.zsamp_.nrSteps() + 1;
    if ( nrsamps > 2000 ) cs.zsamp_.step *= 1000;
    else if ( nrsamps > 200 ) cs.zsamp_.step *= 100;
    else if ( nrsamps > 20 ) cs.zsamp_.step *= 10;
    else if ( nrsamps > 10 ) cs.zsamp_.step *= 5;
    nrsamps = cs.zsamp_.nrSteps() + 1;

    const int nrextr = mCast( int, cs.hsamp_.totalNr() * nrsamps );
    int blocks = nrextr / 50000;
    float fstepfac = (float) ( Math::Sqrt( (double)blocks ) );
    int stepfac = mNINT32(fstepfac);
    cs.hsamp_.step_.inl() *= stepfac;
    cs.hsamp_.step_.crl() *= stepfac;
}


void uiRangePosProvGroup::setExtractionDefaults()
{
    TrcKeyZSampling cs( true ); getExtrDefTrcKeyZSampling( cs );
    if ( hrgfld_ )
	hrgfld_->setSampling( cs.hsamp_ );
    if ( nrrgfld_ )
    {
	StepInterval<int> rg( nrrgfld_->getRange() );
	rg.step = 10;
	nrrgfld_->setRange( rg );
    }
    zrgfld_->setRange( cs.zsamp_ );
}


void uiRangePosProvGroup::getTrcKeyZSampling( TrcKeyZSampling& cs ) const
{
    cs = SI().sampling( true );
    if ( hrgfld_ )
	cs.hsamp_ = hrgfld_->getSampling();
    if ( nrrgfld_ )
	cs.hsamp_.set( StepInterval<int>(0,mUdf(int),1), nrrgfld_->getRange() );
    if ( zrgfld_ )
	cs.zsamp_ = zrgfld_->getRange();
}


void uiRangePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Range() );
}


uiPolyPosProvGroup::uiPolyPosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
    , zrgfld_(0)
    , stepfld_(0)
{
    polyfld_ = new uiPickSetIOObjSel( this, true,
				      uiPickSetIOObjSel::PolygonOnly );

    uiGroup* attachobj = polyfld_;
    if ( su.withstep_ )
    {
	stepfld_ = new uiSelSteps( this, false );
	stepfld_->attach( alignedBelow, polyfld_ );
	attachobj = stepfld_;
    }

    if ( su.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, true, false, 0, su.zdomkey_ );
	zrgfld_->attach( alignedBelow, attachobj );
    }

    setHAlignObj( polyfld_ );
}



#define mErrRet(s) { uiMSG().error(s); return false; }
#define mGetPolyKey(k) IOPar::compKey(sKey::Polygon(),k)


void uiPolyPosProvGroup::usePar( const IOPar& iop )
{
    polyfld_->usePar( iop, sKey::Polygon() );
    if ( stepfld_ )
    {
	BinID stps( SI().sampling(true).hsamp_.step_ );
	iop.get( mGetPolyKey(sKey::StepInl()), stps.inl() );
	iop.get( mGetPolyKey(sKey::StepCrl()), stps.crl() );
	stepfld_->setSteps( stps );
    }
    if ( zrgfld_ )
    {
	StepInterval<float> zrg( SI().zRange(true) );
	iop.get( mGetPolyKey(sKey::ZRange()), zrg );
	zrgfld_->setRange( zrg );
    }
}


bool uiPolyPosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Polygon() );
    if ( !polyfld_->commitInput() || !polyfld_->fillPar(iop,sKey::Polygon()) )
	mErrRet(uiStrings::phrSelect(uiStrings::sPolygon()))

    const BinID stps(
	stepfld_ ? stepfld_->getSteps() : SI().sampling(true).hsamp_.step_ );
    iop.set( mGetPolyKey(sKey::StepInl()), stps.inl() );
    iop.set( mGetPolyKey(sKey::StepCrl()), stps.crl() );
    iop.set( mGetPolyKey(sKey::ZRange()),
	zrgfld_ ? zrgfld_->getRange() : SI().zRange(true) );
    return true;
}


void uiPolyPosProvGroup::getSummary( BufferString& txt ) const
{
    txt.set( "Within polygon" );
    const IOObj* ioobj = polyfld_->ioobj( true );
    if ( ioobj )
	txt.add( " '" ).add( ioobj->name() ).add( "." );
}


void uiPolyPosProvGroup::setExtractionDefaults()
{
    TrcKeyZSampling cs( true ); getExtrDefTrcKeyZSampling( cs );
    if ( stepfld_ ) stepfld_->setSteps( cs.hsamp_.step_ );
    if ( zrgfld_ ) zrgfld_->setRange( cs.zsamp_ );
}


bool uiPolyPosProvGroup::getID( DBKey& ky ) const
{
    const IOObj* ioobj = polyfld_->ioobj( true );
    if ( !ioobj )
	return false;
    ky = ioobj->key();
    return true;
}


void uiPolyPosProvGroup::getZRange( StepInterval<float>& zrg ) const
{
    zrg = zrgfld_ ? zrgfld_->getRange() : SI().zRange(true);
}


void uiPolyPosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Polygon() );
}


uiTablePosProvGroup::uiTablePosProvGroup( uiParent* p,
					const uiPosProvGroup::Setup& su )
    : uiPosProvGroup(p,su)
{
    const CallBack selcb( mCB(this,uiTablePosProvGroup,selChg) );

    selfld_ = new uiGenInput(this, tr("Data from"),
		    BoolInpSpec(true,uiStrings::sPickSet(),
		    uiStrings::phrJoinStrings(uiStrings::sTable(),
					      uiStrings::sFile())));
    selfld_->valuechanged.notify( selcb );
    psfld_ = new uiPickSetIOObjSel( this, true );
    psfld_->attach( alignedBelow, selfld_ );
    uiFileSel::Setup fisu( OD::GeneralContent );
    tffld_ = new uiFileSel( this, uiStrings::sFileName(), fisu );
    tffld_->attach( alignedBelow, selfld_ );

    setHAlignObj( selfld_ );
    postFinalise().notify( selcb );
}


void uiTablePosProvGroup::selChg( CallBacker* )
{
    const bool isps = selfld_->getBoolValue();
    psfld_->display( isps );
    tffld_->display( !isps );
}

#define mGetTableKey(k) IOPar::compKey(sKey::Table(),k)

void uiTablePosProvGroup::usePar( const IOPar& iop )
{
    const char* idres = iop.find( mGetTableKey("ID") );
    const char* fnmres = iop.find( mGetTableKey(sKey::FileName()) );
    const bool isfnm = fnmres && *fnmres;
    selfld_->setValue( !isfnm );
    if ( idres )
	psfld_->setInput( DBKey::getFromString(idres) );
    if ( fnmres )
	tffld_->setFileName( fnmres );
}


bool uiTablePosProvGroup::fillPar( IOPar& iop ) const
{
    iop.set( sKey::Type(), sKey::Table() );
    if ( selfld_->getBoolValue() )
    {
	if ( !psfld_->fillPar(iop,sKey::Table()) )
	    mErrRet(uiStrings::phrSelect(uiStrings::sPickSet()))
	iop.removeWithKey( mGetTableKey(sKey::FileName()) );
    }
    else
    {
	const BufferString fnm = tffld_->fileName();
	if ( fnm.isEmpty() )
	    mErrRet(tr("Provide the table file name"))
	else if ( File::isEmpty(fnm.buf()) )
	    mErrRet(tr("Select an existing/readable file"))
	iop.set( mGetTableKey(sKey::FileName()), fnm );
	iop.removeWithKey( mGetTableKey("ID") );
    }
    return true;
}


void uiTablePosProvGroup::getSummary( BufferString& txt ) const
{
    txt += "In table";
}


bool uiTablePosProvGroup::getID( DBKey& ky ) const
{
    if ( !selfld_->getBoolValue() )
	return false;
    const IOObj* ioobj = psfld_->ioobj();
    if ( !ioobj )
	return false;
    ky = ioobj->key();
    return true;
}


bool uiTablePosProvGroup::getFileName( BufferString& fnm ) const
{
    if ( selfld_->getBoolValue() )
	return false;
    fnm = tffld_->fileName();
    return true;
}


void uiTablePosProvGroup::initClass()
{
    uiPosProvGroup::factory().addCreator( create, sKey::Table() );
}
