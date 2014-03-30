/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Umesh Sinha
 Date:		Nov 2008
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiseislinesel.h"

#include "uibutton.h"
#include "uicombobox.h"
#include "uigeninput.h"
#include "uilistbox.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiselsimple.h"
#include "uiselsurvranges.h"

#include "bufstringset.h"
#include "ctxtioobj.h"
#include "iodir.h"
#include "ioman.h"
#include "keystrs.h"
#include "linekey.h"
#include "seisioobjinfo.h"
#include "seistrc.h"
#include "seistrctr.h"
#include "survinfo.h"
#include "transl.h"


uiSeis2DLineSel::uiSeis2DLineSel( uiParent* p, bool multisel )
    : uiCompoundParSel(p,"Line name")
    , ismultisel_(multisel)
    , selectionChanged(this)
{
    butPush.notify( mCB(this,uiSeis2DLineSel,selPush) );
    Survey::GM().getList( lnms_, geomids_, true );
}


const char* uiSeis2DLineSel::lineName() const
{
    return selidxs_.isEmpty() ? "" : lnms_.get( selidxs_[0] ).buf();
}


Pos::GeomID uiSeis2DLineSel::geomID() const
{
    return selidxs_.isEmpty() ? Survey::GeometryManager::cUndefGeomID()
			      : geomids_[selidxs_[0]];
}


void uiSeis2DLineSel::getSelGeomIDs( TypeSet<Pos::GeomID>& selids ) const
{
    selids.erase();
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	selids += geomids_[selidxs_[idx]];
}


void uiSeis2DLineSel::getSelLineNames( BufferStringSet& selnms ) const
{
    deepErase( selnms );
    for ( int idx=0; idx<selidxs_.size(); idx++ )
	selnms.add( lnms_.get(selidxs_[idx]) );
}


void uiSeis2DLineSel::setSelLineNames( const BufferStringSet& selnms )
{
    selidxs_.erase();
    for ( int idx=0; idx<selnms.size(); idx++ )
    {
	const int index = lnms_.indexOf( selnms.get(idx) );
	if ( index >= 0 )
	    selidxs_ += index;
    }
    
    updateSummary();
}


void uiSeis2DLineSel::setInput( const BufferStringSet& lnms )
{
    clearAll();
    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	Pos::GeomID geomid = Survey::GM().getGeomID( lnms.get(idx) );
	if ( geomid == Survey::GeometryManager::cUndefGeomID() )
	    continue;

	geomids_ += geomid;
	lnms_.add( lnms.get(idx) );
    }
}


void uiSeis2DLineSel::setInput( const MultiID& datasetid )
{
    const SeisIOObjInfo oi( datasetid );
    BufferStringSet lnms; oi.getLineNames( lnms );
    setInput( lnms );
}


BufferString uiSeis2DLineSel::getSummary() const
{
    BufferString ret( "No lines selected" );
    if ( !selidxs_.isEmpty() )
	return ret;

    ret = lnms_.get( selidxs_[0] );
    if ( !ismultisel_ || selidxs_.size()==1 )
	return ret;

    if ( selidxs_.size() == lnms_.size() )
	ret = "All";
    else
	ret = selidxs_.size();
	
    ret += " lines";
    return ret;
}


bool uiSeis2DLineSel::inputOK( bool doerr ) const
{
    if ( selidxs_.isEmpty() )
    {
	if ( doerr )
	    uiMSG().error( "Please select the line" );
	return false;
    }

    return true;
}


void uiSeis2DLineSel::clearAll()
{
    deepErase( lnms_ );
    geomids_.erase();
    clearSelection();
}


void uiSeis2DLineSel::clearSelection()
{
    selidxs_.erase();
    updateSummary();
}

#define mErrRet(s) { uiMSG().error(s); return; }

void uiSeis2DLineSel::selPush( CallBacker* )
{
    if ( lnms_.isEmpty() )
	mErrRet( "No 2D lines available" )

    uiSelectFromList::Setup su( "Select 2D line", lnms_ );
    su.current_ = ismultisel_ || selidxs_.isEmpty() ? 0 : selidxs_[0];
    uiSelectFromList dlg( this, su );
    if ( ismultisel_ && dlg.selFld() )
    {
	dlg.selFld()->setMultiSelect();
	dlg.selFld()->setSelectedItems( selidxs_ );
    }

    if ( !dlg.go() || !dlg.selFld() )
	return;

    selidxs_.erase();
    dlg.selFld()->getSelectedItems( selidxs_ );
    selectionChanged.trigger();
}


void uiSeis2DLineSel::setSelLine( const char* lnm )
{
    selidxs_.erase();
    const int idx = lnms_.indexOf( lnm );
    if ( idx >= 0 )
	selidxs_ += idx;

    updateSummary();
}


void uiSeis2DLineSel::setSelLine( const PosInfo::Line2DKey& l2dky )
{
    l2dky_ = l2dky;
    const char* lnm = S2DPOS().getLineName( l2dky_.lineID() );
    setSelLine( lnm );
}


const PosInfo::Line2DKey& uiSeis2DLineSel::getLine2DKey() const
{ return l2dky_; }


uiSeis2DLineNameSel::uiSeis2DLineNameSel( uiParent* p, bool forread )
    : uiGroup(p,"2D line name sel")
    , forread_(forread)
    , nameChanged(this)
{
    uiLabeledComboBox* lcb = new uiLabeledComboBox( this, "Line name" );
    fld_ = lcb->box();
    fld_->setReadOnly( forread_ );
    if ( !forread_ ) fld_->addItem( "" );
    setHAlignObj( lcb );
    if ( !forread_ )
	postFinalise().notify( mCB(this,uiSeis2DLineNameSel,fillAll) );
    fld_->selectionChanged.notify( mCB(this,uiSeis2DLineNameSel,selChg) );
}


void uiSeis2DLineNameSel::fillAll( CallBacker* )
{
    if ( dsid_.isEmpty() )
	fillWithAll();
}


void uiSeis2DLineNameSel::fillWithAll()
{
    BufferStringSet lnms;
    TypeSet<Pos::GeomID> geomids;
    Survey::GM().getList( lnms, geomids, true );
    fld_->addItems( lnms );
    if ( fld_->size() )
	fld_->setCurrentItem( 0 );
}


void uiSeis2DLineNameSel::addLineNames( const MultiID& ky )
{
    const SeisIOObjInfo oi( ky );
    if ( !oi.isOK() || !oi.is2D() ) return;

    BufferStringSet lnms; oi.getLineNames( lnms );
    nameChanged.disable();
    fld_->addItems( lnms );
    nameChanged.enable();
}


const char* uiSeis2DLineNameSel::getInput() const
{
    return fld_->text();
}


int uiSeis2DLineNameSel::getLineIndex() const
{
    return fld_->currentItem();
}


void uiSeis2DLineNameSel::setInput( const char* nm )
{
    if ( fld_->isPresent(nm) )
	fld_->setCurrentItem( nm );

    if ( !forread_ )
    {
	nameChanged.disable();
	fld_->setCurrentItem( 0 );
	nameChanged.enable();
	fld_->setText( nm );
	nameChanged.trigger();
    }
}


void uiSeis2DLineNameSel::setDataSet( const MultiID& ky )
{
    dsid_ = ky;
    fld_->setEmpty();
    if ( !forread_ ) fld_->addItem( "" );
    addLineNames( ky );
}



uiSeis2DMultiLineSelDlg::uiSeis2DMultiLineSelDlg( uiParent* p, CtxtIOObj& c,
					const uiSeis2DMultiLineSel::Setup& su )
    : uiDialog( p, uiDialog::Setup("Select 2D Lines",mNoDlgTitle,"103.1.13") )
    , setup_(su)
    , ctio_(c)
    , datasetfld_(0)
    , zrgfld_(0)
{
    uiLabeledListBox* llb = new uiLabeledListBox( this, "Line names", true );
    lnmsfld_ = llb->box();
    lnmsfld_->selectionChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,lineSel) );

    if ( setup_.withlinesetsel_ )
    {
	uiSeisSel::Setup sssu(Seis::Line);
	sssu.selattr(setup_.withattr_).allowsetdefault(setup_.filldef_);
	if ( setup_.withattr_ && !setup_.allattribs_ )
	    sssu.selattr( true ).wantSteering(setup_.steering_);

	datasetfld_ = new uiSeisSel( this, ctio_, sssu );
	datasetfld_->selectionDone.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,lineSetSel) );
	llb->attach( alignedBelow, datasetfld_ );
    }

    trcrgfld_ = new uiSelNrRange( this, StepInterval<int>(),
				  setup_.withstep_, "Trace" );
    trcrgfld_->rangeChanged.notify(
		mCB(this,uiSeis2DMultiLineSelDlg,trcRgChanged) );
    trcrgfld_->attach( alignedBelow, llb );

    if ( setup_.withz_ )
    {
	zrgfld_ = new uiSelZRange( this, su.withstep_,
			BufferString("Z Range",SI().getZUnitString()) );
	zrgfld_->setRangeLimits( SI().zRange(false) );
	zrgfld_->attach( alignedBelow, trcrgfld_ );
    }

    postFinalise().notify( mCB(this,uiSeis2DMultiLineSelDlg,finalised) );
}


void uiSeis2DMultiLineSelDlg::finalised( CallBacker* )
{
    if ( !datasetfld_ ) return;

    const IOObj* lsetobj = datasetfld_->ioobj( true );
    if ( !lsetobj )
	datasetfld_->doSel( 0 );
    else if ( lnmsfld_->nrSelected()==0 )
	lineSetSel( 0 );
}


void uiSeis2DMultiLineSelDlg::setLineSet( const MultiID& key, const char* attr )
{
    if ( datasetfld_ )
	datasetfld_->setInput( key );

    lineSetSel( 0 );
}


void uiSeis2DMultiLineSelDlg::setAll( bool yn )
{ lnmsfld_->selectAll( yn ); }

void uiSeis2DMultiLineSelDlg::setSelection( const BufferStringSet& sellines,
				const TypeSet<StepInterval<int> >* rgs	)
{
    if ( rgs && rgs->size() != sellines.size() )
	return;

    lnmsfld_->clearSelection();
    if ( !sellines.isEmpty() )
	lnmsfld_->setCurrentItem( sellines.get(0) );

    for ( int idx=0; idx<sellines.size(); idx++ )
    {
	const int selidx = lnmsfld_->indexOf( sellines.get(idx) );
	if ( selidx < 0 )
	    continue;

	lnmsfld_->setSelected( selidx );
	if ( rgs )
	    trcrgs_[selidx] = (*rgs)[idx];
    }

    lineSel(0);
}

void uiSeis2DMultiLineSelDlg::setZRange( const StepInterval<float>& rg )
{ if ( zrgfld_ ) zrgfld_->setRange( rg ); }

void uiSeis2DMultiLineSelDlg::lineSetSel( CallBacker* )
{
    const IOObj* lsetobj = datasetfld_ ? datasetfld_->ioobj() : ctio_.ioobj;
    if ( !lsetobj )
	return;

    SeisIOObjInfo oinf( lsetobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );
    lnmsfld_->setEmpty();
    maxtrcrgs_.erase();
    trcrgs_.erase();
    zrgs_.erase();


    StepInterval<float> maxzrg( mUdf(float), -mUdf(float), 1 );;

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const char* lnm = lnms.get(idx).buf();
	StepInterval<int> trcrg;
	StepInterval<float> zrg;
	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	oinf.getRanges( geomid, trcrg, zrg );

	if ( trcrg.nrSteps() <= 0 )
	    continue;

	lnmsfld_->addItem( lnm );
	maxtrcrgs_ += trcrg;
	trcrgs_ += trcrg;
	zrgs_ += zrg;
    }

    setZRange( maxzrg );
    lnmsfld_->selectAll();
    lineSel(0);
}


IOObj* uiSeis2DMultiLineSelDlg::getIOObj()
{ return datasetfld_ ? datasetfld_->getIOObj() : ctio_.ioobj->clone(); }

const char* uiSeis2DMultiLineSelDlg::getAttribName() const
{ return datasetfld_ ? datasetfld_->attrNm() : 0; }

void uiSeis2DMultiLineSelDlg::getSelLines( BufferStringSet& sellines ) const
{
    deepErase( sellines );
    lnmsfld_->getSelectedItems( sellines );
}

void uiSeis2DMultiLineSelDlg::getTrcRgs(TypeSet<StepInterval<int> >& rgs) const
{
    rgs.erase();
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( !lnmsfld_->isSelected(idx) )
	    continue;

	rgs += trcrgs_[idx];
    }
}

bool uiSeis2DMultiLineSelDlg::isAll() const
{
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
	if ( !lnmsfld_->isSelected(idx) || trcrgs_[idx] != maxtrcrgs_[idx] )
	    return false;

    return true;
}


void uiSeis2DMultiLineSelDlg::getZRanges( 
				    TypeSet<StepInterval<float> >& zrgs ) const
{
    zrgs.erase();
    for ( int idx=0; idx<lnmsfld_->size(); idx++ )
    {
	if ( !lnmsfld_->isSelected(idx) )
	    continue;

	zrgs += zrgs_[idx];
    }
}


void uiSeis2DMultiLineSelDlg::lineSel( CallBacker* )
{
    const bool multisel = lnmsfld_->nrSelected() > 1;
    trcrgfld_->setSensitive( !multisel );
    zrgfld_->setSensitive( !multisel );
    if ( multisel ) return;

    NotifyStopper ns( trcrgfld_->rangeChanged );
    if ( trcrgs_.isEmpty() || lnmsfld_->nrSelected() <= 0 )
	return;

    trcrgfld_->setLimitRange( maxtrcrgs_[0] );
    trcrgfld_->setRange( trcrgs_[0] );
}


void uiSeis2DMultiLineSelDlg::trcRgChanged( CallBacker* )
{
    const int curitm = lnmsfld_->currentItem();
    if ( curitm<0 ) return;

    trcrgs_[curitm].start = trcrgfld_->getRange().start;
    trcrgs_[curitm].stop = trcrgfld_->getRange().stop;
}


bool uiSeis2DMultiLineSelDlg::acceptOK( CallBacker* )
{
    if ( lnmsfld_->nrSelected() == 1 )
	trcRgChanged( 0 );
    return true;
}


uiSeis2DMultiLineSel::uiSeis2DMultiLineSel( uiParent* p, const Setup& setup )
    : uiCompoundParSel(p,"LineSet/LineName","Select")
    , setup_(*new uiSeis2DMultiLineSel::Setup(setup))
    , ctio_(*uiSeisSel::mkCtxtIOObj(Seis::Line,true))
    , isall_(true)
{
    if ( !setup.lbltxt_.isEmpty() ) txtfld_->setTitleText( setup.lbltxt_ );
    butPush.notify( mCB(this,uiSeis2DMultiLineSel,doDlg) );
    if ( !setup_.filldef_ )
    {
	ctio_.destroyAll();
	ctio_.ctxt.deftransl = "TwoD Dataset";
    }

    updateFromLineset();
}


uiSeis2DMultiLineSel::~uiSeis2DMultiLineSel()
{
    deepErase( sellines_ );
    delete &setup_;
    delete ctio_.ioobj;
    delete &ctio_;
}


BufferString uiSeis2DMultiLineSel::getSummary() const
{
    BufferString ret;
    if ( !ctio_.ioobj || !ctio_.ioobj->implExists(true) )
	return ret;

    ret = ctio_.ioobj->name();
    const int nrsel = sellines_.size();
    if ( nrsel==1 )
	ret += " (1 line)";
    else
    {
	ret += " (";
	if ( isall_ )
	    ret += "all";
	else
	    ret += nrsel;

	ret += " lines)";
    }

    return ret;
}


void uiSeis2DMultiLineSel::doDlg( CallBacker* )
{
    uiSeis2DMultiLineSelDlg dlg( this, ctio_, setup_ );
    if ( ctio_.ioobj )
    {
	dlg.setLineSet( ctio_.ioobj->key(), attrnm_.buf() );

	if ( isall_ )
	    dlg.setAll( true );
	else
	    dlg.setSelection( sellines_, &trcrgs_ );
    }

    if ( !dlg.go() )
	return;

    if ( setup_.withlinesetsel_ )
    {
	IOObj* newobj = dlg.getIOObj();
	if ( newobj != ctio_.ioobj )
	{
	    delete ctio_.ioobj;
	    ctio_.ioobj = newobj;
	}

	attrnm_ = dlg.getAttribName();
    }

    isall_ = dlg.isAll();
    dlg.getZRanges( zrg_ );
    dlg.getSelLines( sellines_ );
    dlg.getTrcRgs( trcrgs_ );
    updateSummary();
}


bool uiSeis2DMultiLineSel::fillPar( IOPar& par ) const
{
    if ( !ctio_.ioobj || !ctio_.ioobj->implExists(true) )
	return false;

    par.set( "DataSet.ID", ctio_.ioobj->key() );
    BufferString mergekey;
    IOPar lspar;
    for ( int idx=0; idx<sellines_.size(); idx++ )
    {
	IOPar linepar;
	linepar.set( sKey::Name(), sellines_[idx]->buf() );
	linepar.set( sKey::TrcRange(), trcrgs_[idx] );
	if ( setup_.withz_ )
	    linepar.set( sKey::ZRange(), zrg_[idx] );

	mergekey = idx;
	lspar.mergeComp( linepar, mergekey );
    }

    par.mergeComp( lspar, "Line" );
    return true;
}


void uiSeis2DMultiLineSel::usePar( const IOPar& par )
{
    MultiID lsetkey;
    if ( !par.get("LineSet.ID",lsetkey) )
	return;

    delete ctio_.ioobj;
    ctio_.ioobj = IOM().get( lsetkey );
    par.get( sKey::Attribute(), attrnm_ );
    deepErase( sellines_ );
    trcrgs_.erase();
    PtrMan<IOPar> lspar = par.subselect( "Line" );
    if ( !lspar ) return;

    for ( int idx=0; idx<1024; idx++ )
    {
	PtrMan<IOPar> linepar = lspar->subselect( idx );
	if ( !linepar )
	    break;

	FixedString lnm = linepar->find( sKey::Name() );
	StepInterval<int> trcrg;
	if ( !lnm || !linepar->get(sKey::TrcRange(),trcrg) )
	    continue;

	StepInterval<float> zrg;
	if ( linepar->get(sKey::ZRange(),zrg) )
	    zrg_ += zrg;

	sellines_.add( lnm );
	trcrgs_ += trcrg;
    }

    updateSummary();
}


const IOObj* uiSeis2DMultiLineSel::ioObj() const
{ return ctio_.ioobj; }

IOObj* uiSeis2DMultiLineSel::getIOObj()
{ return ctio_.ioobj ? ctio_.ioobj->clone() : 0; }

BufferString uiSeis2DMultiLineSel::getAttribName() const
{ return attrnm_; }

const BufferStringSet& uiSeis2DMultiLineSel::getSelLines() const
{ return sellines_; }

bool uiSeis2DMultiLineSel::isAll() const
{ return isall_; }

const TypeSet<StepInterval<float> >& uiSeis2DMultiLineSel::getZRange() const
{ return zrg_; }

const TypeSet<StepInterval<int> >&  uiSeis2DMultiLineSel::getTrcRgs() const
{ return trcrgs_; }

void uiSeis2DMultiLineSel::setLineSet( const MultiID& key, const char* attr )
{
    delete ctio_.ioobj;
    ctio_.ioobj = IOM().get( key );
    attrnm_ = attr;
    deepErase( sellines_ );
    trcrgs_.erase();
    isall_ = true;
    updateFromLineset();
    updateSummary();
}


void uiSeis2DMultiLineSel::updateFromLineset()
{
    if ( !ctio_.ioobj )
	return;

    SeisIOObjInfo oinf( ctio_.ioobj );
    BufferStringSet lnms;
    oinf.getLineNames( lnms );
    if ( !lnms.size() )
	return;

    for ( int idx=0; idx<lnms.size(); idx++ )
    {
	const char* lnm = lnms.get(idx).buf();
	StepInterval<int> trcrg( 0, 0, 1 );
	StepInterval<float> zrg;
	Pos::GeomID geomid = Survey::GM().getGeomID( lnm );
	oinf.getRanges( geomid, trcrg, zrg );
	trcrgs_ += trcrg;
	zrg_ += zrg;
	sellines_.add( lnm );
    }
}


void uiSeis2DMultiLineSel::setSelLines( const BufferStringSet& sellines )
{ isall_ = false; sellines_ = sellines; updateSummary(); }

void uiSeis2DMultiLineSel::setAll( bool yn )
{ isall_ = yn; updateSummary(); }

void uiSeis2DMultiLineSel::setZRange( const TypeSet<StepInterval<float> >& zrg )
{ zrg_ = zrg; }

void uiSeis2DMultiLineSel::setTrcRgs( const TypeSet<StepInterval<int> >& rgs )
{ trcrgs_ = rgs; }

