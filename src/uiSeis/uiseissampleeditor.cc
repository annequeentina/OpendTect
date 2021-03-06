/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Sulochana/Satyaki
 Date:          Oct 2007
________________________________________________________________________

-*/

#include "uiseissampleeditor.h"

#include "uiamplspectrum.h"
#include "uibutton.h"
#include "uicombobox.h"
#include "uiflatviewer.h"
#include "uiflatviewstdcontrol.h"
#include "uiflatviewmainwin.h"
#include "uifunctiondisplay.h"
#include "uigeninput.h"
#include "uilabel.h"
#include "uimsg.h"
#include "uiseissel.h"
#include "uiseistrcbufviewer.h"
#include "uiseparator.h"
#include "uispinbox.h"
#include "uitable.h"
#include "uitaskrunner.h"
#include "uitoolbar.h"

#include "trckeyzsampling.h"
#include "datapack.h"
#include "executor.h"
#include "arrayndimpl.h"
#include "filepath.h"
#include "dbman.h"
#include "posidxpairdataset.h"
#include "posinfo.h"
#include "posinfo2d.h"
#include "ranges.h"
#include "seis2ddata.h"
#include "seisbuf.h"
#include "seisprovider.h"
#include "seistrc.h"
#include "seisinfo.h"
#include "seiswrite.h"
#include "survgeom.h"
#include "zdomain.h"
#include "od_helpids.h"


class uiSeisSampleEditorInfoVwr : public uiAmplSpectrum
{ mODTextTranslationClass(uiSeisSampleEditorInfoVwr);
public :

			uiSeisSampleEditorInfoVwr(uiSeisSampleEditor&,
						  const SeisTrc&);

    void		setTrace(const SeisTrc&);

protected:

    uiSeisSampleEditor&	ed_;
    const bool		is2d_;
    const ZDomain::Def&	zdomdef_;

    uiGenInput*		coordfld_;
    uiGenInput*		trcnrbinidfld_;
    uiGenInput*		minamplfld_;
    uiGenInput*		maxamplfld_;
    uiGenInput*		minamplatfld_;
    uiGenInput*		maxamplatfld_;

};


uiSeisSampleEditor::Setup::Setup( const DBKey& ky )
    : uiDialog::Setup(uiString::emptyString(),mNoDlgTitle,
                      mODHelpKey(mSeisBrowserHelpID) )
    , id_(ky)
    , startpos_(mUdf(int),mUdf(int))
    , startz_(mUdf(float))
    , readonly_(false)
{
    wintitle_ = tr( "Browse/Edit '%1'" ).arg( DBM().nameOf( id_ ) );
}


#define mRetNoGo(msg) \
{ \
    new uiLabel( this, msg ); \
    setCtrlStyle( CloseOnly ); \
    delete prov_; \
    return; \
}


uiSeisSampleEditor::uiSeisSampleEditor( uiParent* p, const Setup& su )
    : uiDialog(p,su)
    , prov_(0)
    , tbl_(0)
    , ctrc_(0)
    , toolbar_(0)
    , tbuf_(*new SeisTrcBuf(false))
    , viewtbuf_(*new SeisTrcBuf(true))
    , edtrcs_(*new Pos::IdxPairDataSet(sizeof(SeisTrc*),false,false))
    , cubedata_(*new PosInfo::CubeData)
    , linedata_(*new PosInfo::LineData(su.geomid_))
    , crlwise_(false)
    , stepout_(25)
    , compnr_(0)
    , sampling_(0.f,1.f)
    , infovwr_(0)
    , trcbufvwr_(0)
    , setup_(su)
    , zdomdef_(ZDomain::SI())
    , toinlwisett_(tr("Switch to Inline-wise display"))
    , tocrlwisett_(tr("Switch to Crossline-wise display"))
{
    uiRetVal uirv;
    prov_ = Seis::Provider::create( su.id_, &uirv );
    if ( !prov_ )
	mRetNoGo( uirv )

    uirv = prov_->getComponentInfo( compnms_, &datatype_ );
    if ( compnms_.isEmpty() )
    {
	if ( uirv.isOK() )
	    compnms_.add( "Component 1" );
	else
	    mRetNoGo( uirv )
    }

    is2d_ = prov_->is2D();
    ZSampling zrg = prov_->getZRange();
    sampling_.start = zrg.start;
    sampling_.step = zrg.step;
    nrsamples_ = zrg.nrSteps() + 1;

    if ( !is2d_ )
    {
	survgeom_ = &Survey::Geometry::default3D();
	prov3D().getGeometryInfo( cubedata_ );
    }
    else
    {
	survgeom_ = Survey::GM().getGeometry( setup_.geomid_ );
	if ( !survgeom_ )
	    mRetNoGo( tr("Cannot find line geometry for ID=%1)")
			.arg(setup_.geomid_) );
	int linenr = prov2D().lineNr( setup_.geomid_ );
	if ( linenr < 0 )
	    mRetNoGo( tr("Line not present for ID=%1").arg(setup_.geomid_) )
	PosInfo::Line2DData l2dd;
	prov2D().getGeometryInfo( linenr, l2dd );
	l2dd.getSegments( linedata_ );
    }

    if ( (is2d_ ? linedata_.isEmpty() : cubedata_.isEmpty()) )
	mRetNoGo( tr("'%1' is empty").arg(prov_->name()) )


    // OK ... I guess we can do our thing

    if ( is2d_ )
	stepbid_ = BinID( 1, linedata_.minStep() );
    else
	stepbid_ = cubedata_.minStep();

    createMenuAndToolBar();
    createTable();

    if ( setPos(su.startpos_) )
	setZ( su.startz_ );

    tbl_->selectionChanged.notify(
	    mCB(this,uiSeisSampleEditor,selChgCB) );
}


uiSeisSampleEditor::~uiSeisSampleEditor()
{
    clearEditedTraces();
    tbuf_.deepErase();
    viewtbuf_.deepErase();
    delete prov_;
    delete &tbuf_;
    delete &viewtbuf_;
    delete &cubedata_;
    delete &linedata_;
}


void uiSeisSampleEditor::clearEditedTraces()
{
    Pos::IdxPairDataSet::SPos spos;
    while ( edtrcs_.next(spos) )
	delete (SeisTrc*)edtrcs_.getObj( spos );
    edtrcs_.setEmpty();
}


Seis::Provider2D& uiSeisSampleEditor::prov2D()
{
    return *static_cast<Seis::Provider2D*>( prov_ );
}


Seis::Provider3D& uiSeisSampleEditor::prov3D()
{
    return *static_cast<Seis::Provider3D*>( prov_ );
}


Seis::GeomType uiSeisSampleEditor::geomType() const
{
    return is2d_ ? Seis::Line : Seis::Vol;
}


TrcKey uiSeisSampleEditor::trcKey4BinID( const BinID& bid ) const
{
    TrcKey tk( bid );
    if ( is2d_ )
	tk.setGeomID( setup_.geomid_ );
    return tk;
}


TrcKey uiSeisSampleEditor::curPos() const
{
    return ctrc_ ? ctrc_->info().binID() : BinID(0,0);
}


float uiSeisSampleEditor::curZ() const
{
    return sampling_.atIndex( tbl_->currentRow() );
}


void uiSeisSampleEditor::setZ( float z )
{
    const int newrow = mIsUdf(z) ? nrsamples_/2 : sampling_.nearestIndex( z );
    tbl_->setCurrentCell( RowCol(tbl_->currentCol(),newrow) );
}


#define mAddButton(fnm,func,tip,toggle) \
    toolbar_->addButton( fnm, tip, mCB(this,uiSeisSampleEditor,func), toggle )

void uiSeisSampleEditor::createMenuAndToolBar()
{
    toolbar_ = new uiToolBar( this, tr("Sample Editor Tool Bar") );
    mAddButton( "gotopos",goToPush,tr("Goto position"),false );
    mAddButton( "info",infoPush,tr("Information"),false );
    if ( !is2d_ )
	crlwisebutidx_ = mAddButton( "crlwise", switchViewTypePush,
				     tocrlwisett_, true );
    mAddButton( "leftarrow",leftArrowPush,tr("Move left"),false );
    mAddButton( "rightarrow",rightArrowPush,tr("Move right"),false );
    showwgglbutidx_ = mAddButton( "wva",dispTracesPush,
				  tr("Display current traces"),false );

    if ( compnms_.size() > 1 )
    {
	selcompnmfld_ = new uiComboBox( toolbar_, compnms_.getUiStringSet(),
							    "Component name" );
	toolbar_->addObject( selcompnmfld_ );
	selcompnmfld_->setCurrentItem( compnr_ );
	selcompnmfld_->selectionChanged.notify(
				    mCB(this,uiSeisSampleEditor,chgCompNrCB) );
    }

    uiLabel* lbl = new uiLabel( toolbar_, tr("Nr traces") );
    toolbar_->addObject( lbl );
    nrtrcsfld_ = new uiSpinBox( toolbar_ );
    nrtrcsfld_->setInterval( StepInterval<int>(3,99999,2) );
    nrtrcsfld_->doSnap( true );
    nrtrcsfld_->setValue( 2*stepout_+1 );
    nrtrcsfld_->valueChanged.notify(
			    mCB(this,uiSeisSampleEditor,nrTracesChgCB) );
    toolbar_->addObject( nrtrcsfld_ );
}


void uiSeisSampleEditor::createTable()
{
    const int nrrows = nrsamples_;
    const int nrcols = 2*stepout_ + 1;
    tbl_ = new uiTable( this, uiTable::Setup( nrrows, nrcols )
			     .selmode(uiTable::Multi)
			     .manualresize( true ), "Seismic data" );

    tbl_->valueChanged.notify( mCB(this,uiSeisSampleEditor,tblValChgCB) );
    tbl_->setStretch( 1, 1 );
    tbl_->setPrefHeight( 400 );
    tbl_->setPrefWidth( 600 );
    tbl_->setTableReadOnly( setup_.readonly_ );
}


BinID uiSeisSampleEditor::getBinID4RelIdx( const BinID& bid, int idx ) const
{
    return crlwise_
	? BinID( bid.inl() + idx*stepbid_.inl(), bid.crl() )
	: BinID( bid.inl(), bid.crl() + idx*stepbid_.crl() );
}


bool uiSeisSampleEditor::setPos( const TrcKey& tk )
{
    return doSetPos( tk.binID(), false );
}


bool uiSeisSampleEditor::setPos( const BinID& bid )
{
    return doSetPos( bid, false );
}


bool uiSeisSampleEditor::doSetPos( const BinID& bid, bool force )
{
    if ( !tbl_ )
	return false;
    if ( !force && ctrc_ && bid == ctrc_->info().binID() )
	return true;

    commitChanges();
    BinID binid( bid );
    const bool inldef = is2d_ || !mIsUdf(bid.inl());
    const bool crldef = !mIsUdf(bid.crl());
    if ( !inldef || !crldef )
    {
	if ( is2d_ )
	    binid.crl() = linedata_.centerNumber();
	else
	    binid = cubedata_.centerPos();
    }

    tbuf_.deepErase();
    viewtbuf_.deepErase();
    for ( int idx=-stepout_; idx<=stepout_; idx++ )
    {
	addTrc( getBinID4RelIdx(binid,idx) );
	if ( idx == 0 )
	    ctrc_ = tbuf_.last();
    }

    tbuf_.copyInto( viewtbuf_ );
    fillTable();
    return true;
}


void uiSeisSampleEditor::addTrc( const BinID& bid )
{
    SeisTrc* trc = new SeisTrc;

    Pos::IdxPairDataSet::SPos spos = edtrcs_.find( bid );
    if ( spos.isValid() )
	*trc = *((SeisTrc*)edtrcs_.getObj( spos ));
    else
    {
	const TrcKey tk( trcKey4BinID(bid) );
	if ( !prov_->get(tk,*trc).isOK() )
	{
	    trc->info().setBinID( bid );
	    trc->info().coord_ = survgeom_->toCoord( bid );
	    fillUdf( *trc );
	}
    }

    tbuf_.add( trc );
}


void uiSeisSampleEditor::setStepout( int nr )
{
    stepout_ = nr;
    nrtrcsfld_->setValue( nr*2+1 );
}


void uiSeisSampleEditor::fillUdf( SeisTrc& trc )
{
    while ( trc.nrComponents() > nrComponents() )
	trc.data().delComponent(0);
    while ( trc.nrComponents() < nrComponents() )
	trc.data().addComponent( nrsamples_, DataCharacteristics() );

    trc.reSize( nrsamples_, false );

    for ( int icomp=0; icomp<nrComponents(); icomp++ )
    {
	for ( int isamp=0; isamp<nrsamples_; isamp++ )
	    trc.set( isamp, mUdf(float), icomp );
    }
}


static BufferString getZValStr( float z, int zfac )
{
    BufferString txt;
    float dispz = zfac * z * 10;
    int idispz = mNINT32( dispz );
    dispz = idispz * 0.1f;
    txt = dispz;
    return txt;
}


void uiSeisSampleEditor::fillTable()
{
    NotifyStopper notifstop( tbl_->valueChanged );

    const int zfac = zdomdef_.userFactor();
    const BufferString zunstr = zdomdef_.unitStr(false).getFullString();
    for ( int idx=0; idx<nrsamples_; idx++ )
    {
	const BufferString zvalstr( getZValStr(sampling_.atIndex(idx),zfac) );
	tbl_->setRowLabel( idx, toUiString(zvalstr) );
	uiString tt;
	tt = toUiString("%1%2 sample at %3 %4").arg(idx+1)
			.arg(getRankPostFix(idx+1)).arg(zvalstr).arg(zunstr);
	tbl_->setRowToolTip( idx, tt );
    }

    for ( int idx=0; idx<tbuf_.size(); idx++ )
	fillTableColumn( *tbuf_.get(idx), idx );

    tbl_->resizeRowsToContents();
    const int middlecol = tbuf_.size()/2;
    tbl_->selectColumn( middlecol );
    tbl_->ensureCellVisible( RowCol(nrsamples_/2,middlecol) );
}


void uiSeisSampleEditor::fillTableColumn( const SeisTrc& trc, int colidx )
{
    BufferString coltxt;
    if ( !is2d_ )
	coltxt.set( trc.info().binID().toString(false) );
    else
    {
	coltxt.set( trc.info().trcNr() );
	const float refnr = trc.info().refnr_;
	if ( !mIsUdf(refnr) && refnr > 0.5 )
	    coltxt.add( " [" ).add( refnr ).add( "]" );
    }
    tbl_->setColumnLabel( colidx, toUiString(coltxt) );

    RowCol rc( 0, colidx );
    for ( ; rc.row()<nrsamples_; rc.row()++ )
    {
	const float val = trc.get( rc.row(), compnr_ );
	tbl_->setValue( rc, val );
    }
}


void uiSeisSampleEditor::infoPush( CallBacker* )
{
    const SeisTrc* trc = curTrace( true );
    if ( !trc )
	return;

    if ( !infovwr_ )
    {
	infovwr_ = new uiSeisSampleEditorInfoVwr( *this, *trc );
	infovwr_->windowClosed.notify( mCB(this,uiSeisSampleEditor,infoClose) );
    }

    infovwr_->setTrace( *trc );
    infovwr_->show();
}


SeisTrc* uiSeisSampleEditor::curTrace( bool forview )
{
    const int curcol = tbl_->currentCol();
    if ( curcol < 0 )
	return ctrc_;

    SeisTrcBuf& tbuf = forview ? viewtbuf_ : tbuf_;
    return curcol < tbuf.size() ? tbuf.get( curcol ) : 0;
}


void uiSeisSampleEditor::selChgCB( CallBacker* )
{
    const SeisTrc* trc = curTrace( true );
    if ( trc && infovwr_ )
	infovwr_->setTrace( *trc );
}


class uiSeisSampleEditorGoToDlg : public uiDialog
{ mODTextTranslationClass(uiSeisSampleEditorGoToDlg);
public:

uiSeisSampleEditorGoToDlg( uiSeisSampleEditor* p )
    : uiDialog( p, uiDialog::Setup(tr("Reposition"),
				   tr("Specify a new position"),
				   mNoHelpKey) )
    , ed_(*p)
    , inlfld_(0)
{
    const bool is2d = ed_.is2D();

    uiLabeledSpinBox* lbsb = new uiLabeledSpinBox( this,
		is2d ? tr("New trace number") : tr("New position") );

    if ( is2d )
	trcnrfld_ = lbsb->box();
    else
    {
	inlfld_ = lbsb->box();
	trcnrfld_ = new uiSpinBox( this );
	trcnrfld_->attach( rightOf, lbsb );
    }

    tonearestbut_ = new uiToolButton( this, "tonearestpos",
			    tr("Adjust to existing position"),
			    mCB(this,uiSeisSampleEditorGoToDlg,toNearestCB) );

    const TrcKey curpos = ed_.curPos();
    trcnrfld_->setValue( curpos.trcNr() );
    mAttachCB( trcnrfld_->valueChanging, uiSeisSampleEditorGoToDlg::valChgCB );
    if ( !inlfld_ )
	tonearestbut_->attach( rightOf, lbsb );
    else
    {
	tonearestbut_->attach( rightOf, trcnrfld_ );
	inlfld_->setValue( curpos.inl() );
	mAttachCB( inlfld_->valueChanged, uiSeisSampleEditorGoToDlg::valChgCB );
    }

    statelbl_ = new uiLabel( this, uiString::emptyString() );
    statelbl_->setPrefWidthInChar( 30 );
    statelbl_->attach( alignedBelow, lbsb );

    valChgCB( 0 );
}


bool posIsReasonable() const
{
    if ( inlfld_ )
	return SI().isReasonable(pos_);

    return pos_.trcNr() > 0;
}


bool posIsInside() const
{
    if ( inlfld_ )
       return SI().includes( pos_ );
    return ed_.survgeom_->includes( ed_.setup_.geomid_, pos_.trcNr() );
}


void valChgCB( CallBacker* )
{
    const bool is2d = !inlfld_;
    pos_.crl() = trcnrfld_->getIntValue();
    if ( inlfld_ )
	pos_.inl() = inlfld_->getIntValue();

    uiString lbl; bool isok = false;
    if ( !posIsReasonable() )
	lbl = is2d ? tr("Invalid trace number") : tr( "Invalid position" );
    else if ( !posIsInside() )
	lbl = is2d ? tr("Trace number not in line geometry")
		   : tr("Position is outside survey");
    else
    {
	lbl = tr("Not present in data");
	if ( ed_.is2D() && ed_.linedata_.includes(pos_.trcNr()) )
	    { isok = true; lbl = tr("Trace number present"); }
	else if ( !ed_.is2D() && ed_.cubedata_.includes(pos_) )
	    { isok = true; lbl = tr("Position present"); }
    }

    statelbl_->setText( lbl );
    tonearestbut_->display( !isok );
}

void toNearestCB( CallBacker* )
{
    if ( !inlfld_ )
	pos_.crl() = ed_.linedata_.nearestNumber( pos_.crl() );
    else
    {
	pos_ = ed_.cubedata_.nearestBinID( pos_ );
	inlfld_->setValue( pos_.inl() );
    }
    trcnrfld_->setValue( pos_.crl() );
    tonearestbut_->display( false );
}

bool acceptOK()
{
    if ( !posIsInside() )
    {
	uiMSG().error(uiStrings::phrSpecify(tr("a usable position")));
	return false;
    }

    return true;
}

    uiSeisSampleEditor&	ed_;
    uiSpinBox*		inlfld_;
    uiSpinBox*		trcnrfld_;
    uiLabel*		statelbl_;
    uiToolButton*	tonearestbut_;

    BinID		pos_;

};


void uiSeisSampleEditor::goToPush( CallBacker* cb )
{
    uiSeisSampleEditorGoToDlg dlg( this );
    if ( dlg.go() )
    {
	if ( doSetPos( dlg.pos_, false ) )
	    selChgCB( cb );
    }
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
        trcbufvwr_->handleBufChange();
}


void uiSeisSampleEditor::rightArrowPush( CallBacker* cb )
{
    if ( !goTo( getBinID4RelIdx(curBinID(),stepout_) ) )
	return;
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
    selChgCB( cb );
}


void uiSeisSampleEditor::leftArrowPush( CallBacker* cb )
{
    if ( !goTo( getBinID4RelIdx(curBinID(),-stepout_) ) )
	return;
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
    selChgCB( cb );
}


void uiSeisSampleEditor::switchViewTypePush( CallBacker* )
{
    crlwise_ = toolbar_->isOn( crlwisebutidx_ );
    toolbar_->setToolTip( crlwisebutidx_,
			  crlwise_ ? toinlwisett_ : tocrlwisett_ );
    doSetPos( curBinID(), true );
    setTrcBufViewTitle();
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisSampleEditor::commitChanges()
{
    if ( !ctrc_ )
	return;

    BoolTypeSet changed( tbuf_.size(), false );
    for ( RowCol pos(0,0); pos.col()<tbuf_.size(); pos.col()++)
    {
        SeisTrc& trc = *tbuf_.get( pos.col() );
	for ( pos.row()=0; pos.row()<nrsamples_; pos.row()++)
	{
	    const float tableval = tbl_->getFValue( pos );
	    const float trcval = trc.get( pos.row(), compnr_ );
	    const bool isudftbl = mIsUdf( tableval );
	    const bool isudftrc = mIsUdf( trcval );
	    bool ischgd = isudftbl != isudftrc;
	    if ( !ischgd )
	    {
		const float diff = tableval - trcval;
		ischgd = !mIsZero( diff, 1e-6f );
	    }
	    if ( ischgd )
	    {
		trc.set( pos.row(), tableval, compnr_ );
		changed[pos.col()] = true;
	    }
	}
    }

    for ( int idx=0; idx<changed.size(); idx++ )
    {
	if ( !changed[idx] )
	    continue;

	const SeisTrc& buftrc = *tbuf_.get( idx );
	Pos::IdxPairDataSet::SPos spos = edtrcs_.find( buftrc.info().binID() );
	if ( !spos.isValid() )
	    edtrcs_.add( buftrc.info().binID(), new SeisTrc(buftrc) );
	else
	{
	    SeisTrc* trc = (SeisTrc*)edtrcs_.getObj( spos );
	    *trc = buftrc;
	}
    }
}


void uiSeisSampleEditor::launch( uiParent* p, const DBKey& dbky,
				 Pos::GeomID geomid )
{
    uiSeisSampleEditor::Setup setup( dbky );
    setup.geomid_ = geomid;
    uiSeisSampleEditor dlg( p, setup );
    dlg.go();
}


bool uiSeisSampleEditor::acceptOK()
{
    if ( !tbl_ || !prov_ )
	return true;

    commitChanges();
    if ( edtrcs_.isEmpty() )
	return true;

    const int res =
	uiMSG().askSave(tr("Do you want to save the changes permanently?"),
			true);
    if ( res == 1 )
	return storeChgdData();

    return res == 0;
}


class uiSeisSampleEditorWriter : public Executor
{ mODTextTranslationClass(uiSeisSampleEditorWriter);
public:

uiSeisSampleEditorWriter( uiSeisSampleEditor& ed, const DBKey& dbky )
    : Executor( "Writing Edited Seismics" )
    , msg_(tr("Initializing"))
    , ed_(ed)
    , prov_(*ed.prov_)
    , dbky_(dbky)
    , wrr_(0)
    , tksampiter_(0)
{
    nrdone_ = 0;
    totalnr_ = prov_.totalNr();
}


~uiSeisSampleEditorWriter()
{
    delete wrr_;
    delete tksampiter_;
}

od_int64 totalNr() const	{ return totalnr_; }
od_int64 nrDone() const         { return nrdone_; }
uiString message() const	{ return msg_; }
uiString nrDoneText() const	{ return tr("Traces done"); }

void createIter2D()
{
    //TODO support 2D
}

void createIter3D()
{
    Interval<int> inlrg, crlrg;
    ed_.cubedata_.getRanges( inlrg, crlrg );
    TrcKeySampling tks;
    tks.set( inlrg, crlrg );
    tks.step_ = ed_.cubedata_.minStep();
    Pos::IdxPairDataSet::SPos spos;
    while ( ed_.edtrcs_.next(spos) )
    {
	const SeisTrc* trc = (const SeisTrc*)ed_.edtrcs_.getObj( spos );
	tks.include( TrcKey(trc->info().binID()) );
    }
    tksampiter_ = new TrcKeySamplingIterator( tks );
    curbinid_ = tksampiter_->curBinID();
}

int startWork()
{
    //TODO support 2D
    if ( ed_.is2D() )
    {
	msg_ = mTODONotImplPhrase();
	return ErrorOccurred();
    }

    wrr_ = new SeisTrcWriter( dbky_ );
    if ( !wrr_->errMsg().isEmpty() )
    {
	msg_ = wrr_->errMsg();
	return ErrorOccurred();
    }

    if ( ed_.is2D() )
	createIter2D();
    else
	createIter3D();

    return MoreToDo();
}

bool toNextPos()
{
    if ( tksampiter_ )
    {
	if ( !tksampiter_->next() )
	    return false;
	curbinid_ = tksampiter_->curBinID();
	return true;
    }

    return false; //TODO support 2D
}

int nextStep()
{
    if ( !wrr_ )
	return startWork();

    const SeisTrc* towrite = 0;
    while ( !towrite )
    {
	Pos::IdxPairDataSet::SPos spos = ed_.edtrcs_.find( curbinid_ );
	if ( spos.isValid() )
	    towrite = (const SeisTrc*)ed_.edtrcs_.getObj( spos );
	else
	{
	    const TrcKey tk( ed_.trcKey4BinID(curbinid_) );
	    if ( prov_.get(tk,trc_).isOK() )
		towrite = &trc_;
	    else if ( !toNextPos() )
		return Finished();
	}
    }

    if ( !wrr_->put(*towrite) )
    {
	msg_ = wrr_->errMsg();
	return ErrorOccurred();
    }

    return toNextPos() ? MoreToDo() : Finished();
}

    uiSeisSampleEditor&	ed_;
    DBKey		dbky_;
    const Seis::Provider& prov_;
    SeisTrcWriter*	wrr_;
    TrcKeySamplingIterator* tksampiter_;
    od_int64		totalnr_;
    od_int64		nrdone_;
    SeisTrc		trc_;
    BinID		curbinid_;
    uiString		msg_;

};


bool uiSeisSampleEditor::storeChgdData()
{
    CtxtIOObj ctio( uiSeisSel::ioContext(geomType(),false) );
    uiSeisSel::Setup selsu( geomType() );
    selsu.allowsetdefault( false );
    uiSeisSelDlg dlg( this, ctio, selsu );
    if ( !dlg.go() )
	return false;

    PtrMan<uiSeisSampleEditorWriter> wrtr =
	new uiSeisSampleEditorWriter( *this, dlg.ioObj()->key() );
    uiTaskRunner uitr( this );
    return TaskRunner::execute( &uitr, *wrtr );
}


void uiSeisSampleEditor::dispTracesPush( CallBacker* )
{
    if ( trcbufvwr_ )
	trcbufvwr_->start();
    else
    {
	uiSeisTrcBufViewer::Setup stbvsetup( uiString::emptyString() );
	trcbufvwr_ = new uiSeisTrcBufViewer( this, stbvsetup );
	trcbufvwr_->selectDispTypes( true, false );
	mAttachCB( trcbufvwr_->windowClosed, uiSeisSampleEditor::bufVwrClose );

	trcbufvwr_->setTrcBuf( &viewtbuf_, geomType(), "Browsed seismic data",
				    DBM().nameOf(setup_.id_), compnr_ );
	trcbufvwr_->start(); trcbufvwr_->handleBufChange();

	if ( (viewtbuf_.isEmpty()) )
	    uiMSG().error( tr("No data at the specified position ") );
    }

    updateWiggleButtonStatus();
    setTrcBufViewTitle();
}


void uiSeisSampleEditor::updateWiggleButtonStatus()
{
    const bool turnon = !trcbufvwr_ || trcbufvwr_->isHidden();
    toolbar_->turnOn( showwgglbutidx_, !turnon );
}


void uiSeisSampleEditor::bufVwrClose( CallBacker* )
{
    trcbufvwr_ = 0;
    updateWiggleButtonStatus();
}


void uiSeisSampleEditor::tblValChgCB( CallBacker* )
{
    const RowCol rc = tbl_->currentCell();
    if ( rc.row()<0 || rc.col()<0 )
	return;

    SeisTrc* trace = viewtbuf_.get( rc.col() );
    const float chgdval = tbl_->getFValue( rc );
    trace->set( rc.row(), chgdval, compnr_ );
    if ( trcbufvwr_ )
	trcbufvwr_->handleBufChange();
}


void uiSeisSampleEditor::setTrcBufViewTitle()
{
    if ( trcbufvwr_ )
	trcbufvwr_->setWinTitle( tr( "Central Trace: %1")
			       .arg(curBinID().toString(is2d_) ) );

}


void uiSeisSampleEditor::chgCompNrCB( CallBacker* )
{
    NotifyStopper notifstop( tbl_->valueChanged );
    commitChanges();
    compnr_ = selcompnmfld_ ? selcompnmfld_->currentItem() : 0;
    fillTable();
}


void uiSeisSampleEditor::nrTracesChgCB( CallBacker* )
{
    const int nrtrcs = nrtrcsfld_->getIntValue();
    stepout_ = nrtrcs / 2;
    tbl_->clear();
    tbl_->setNrCols( nrtrcs );
    doSetPos( curBinID(), true );
    if ( trcbufvwr_ )
    {
	trcbufvwr_->setTrcBuf( &viewtbuf_, geomType(), "Browsed seismic data",
				    DBM().nameOf(setup_.id_), compnr_ );
	trcbufvwr_->handleBufChange();
    }
}



uiSeisSampleEditorInfoVwr::uiSeisSampleEditorInfoVwr( uiSeisSampleEditor& p,
							const SeisTrc& trc )
    : uiAmplSpectrum(&p)
    , ed_(p)
    , is2d_(ed_.is2D())
    , zdomdef_(ed_.zdomdef_)
{
    setDeleteOnClose( true );
    setCaption( tr("Trace information") );

    uiGroup* valgrp = new uiGroup( this, "Values group" );

    PositionInpSpec coordinpspec( PositionInpSpec::Setup(true,is2d_,false) );
    coordfld_ = new uiGenInput( valgrp, uiStrings::sCoordinate(),
				coordinpspec.setName("X",0).setName("Y",0) );
    coordfld_->setReadOnly();

    uiString label( is2d_ ? tr("Trace/Ref number") : uiStrings::sPosition() );
    IntInpSpec iis; FloatInpSpec fis;
    DataInpSpec* pdis = &iis; if ( is2d_ ) pdis = &fis;
    trcnrbinidfld_ = new uiGenInput( valgrp, label, iis, *pdis );
    trcnrbinidfld_->attach( alignedBelow, coordfld_ );
    trcnrbinidfld_->setReadOnly();

    uiString dtypstr( ed_.datatype_ == Seis::UnknownData ? tr("amplitude")
				 : toUiString(ed_.datatype_) );
    minamplfld_ = new uiGenInput( valgrp, tr("Minimum %1").arg(dtypstr),
				  FloatInpSpec() );
    minamplfld_->attach( alignedBelow, trcnrbinidfld_ );
    minamplfld_->setElemSzPol( uiObject::Small );
    minamplfld_->setReadOnly();
    minamplatfld_ = new uiGenInput( valgrp, tr("at"), FloatInpSpec() );
    minamplatfld_->attach( rightOf, minamplfld_ );
    minamplatfld_->setElemSzPol( uiObject::Small );
    minamplatfld_->setReadOnly();
    uiLabel* lbl = new uiLabel( valgrp, zdomdef_.unitStr(true) );
    lbl->attach( rightOf, minamplatfld_ );

    maxamplfld_ = new uiGenInput( valgrp, tr("Maximum %1").arg(dtypstr),
				  FloatInpSpec() );
    maxamplfld_->attach( alignedBelow, minamplfld_ );
    maxamplfld_->setElemSzPol( uiObject::Small );
    maxamplfld_->setReadOnly();
    maxamplatfld_ = new uiGenInput( valgrp, tr("at"), FloatInpSpec() );
    maxamplatfld_->attach( rightOf, maxamplfld_ );
    maxamplatfld_->setElemSzPol( uiObject::Small );
    maxamplatfld_->setReadOnly();
    lbl = new uiLabel( valgrp, zdomdef_.unitStr(true) );
    lbl->attach( rightOf, maxamplatfld_ );

    uiSeparator* sep = new uiSeparator( this, "Hor sep" );
    sep->attach( stretchedBelow, exportfld_ );
    valgrp->attach( centeredBelow, sep );
    valgrp->attach( ensureBelow, sep );

    setPrefHeightInChar( 20 );
    setPrefWidthInChar( 50 );

    setTrace( trc );
}


void uiSeisSampleEditorInfoVwr::setTrace( const SeisTrc& trc )
{
    coordfld_->setValue( trc.info().coord_ );
    if ( !is2d_ )
	trcnrbinidfld_->setValue( trc.info().binID() );
    else
    {
	trcnrbinidfld_->setValue( trc.info().binID().crl(), 0 );
	trcnrbinidfld_->setValue( trc.info().refnr_, 1 );
    }

    if ( trc.size() < 1 ) return;

    float v0 = mUdf(float), z0 = mUdf(float); int isamp;
    for ( isamp=0; isamp<trc.size(); isamp++ )
    {
	const float v = trc.get( isamp, 0 );
	if ( !mIsUdf(v) )
	    { v0 = v; z0 = trc.info().samplePos( isamp ); break; }
    }
    if ( mIsUdf(z0) ) return;

    Interval<float> amplrg( v0, v0 );
    Interval<float> peakzs( z0, z0 );
    TypeSet<float> vals;
    for ( ; isamp<trc.size(); isamp++ )
    {
	const float v = trc.get( isamp, 0 );
	if ( mIsUdf(v) || mIsUdf(-v) || !Math::IsNormalNumber(v) )
	    continue;

	vals += v;
	if ( v < amplrg.start )
	    { amplrg.start = v; peakzs.start = trc.info().samplePos(isamp); }
	if ( v > amplrg.stop )
	    { amplrg.stop = v; peakzs.stop = trc.info().samplePos(isamp); }
    }

    const int zfac = zdomdef_.userFactor();
    minamplfld_->setValue( amplrg.start );
    minamplatfld_->setText( getZValStr(peakzs.start,zfac) );
    maxamplfld_->setValue( amplrg.stop );
    maxamplatfld_->setText( getZValStr(peakzs.stop,zfac) );

    setup_.nyqvistspspace_ = trc.info().sampling_.step;
    Array1DImpl<float> a1d( vals.size() );
    for ( int idx=0; idx<vals.size(); idx++ )
	a1d.set( idx, vals[idx] );
    setData( a1d );
}
