
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : May 2007
-*/


#include "uisegycommon.h"

#include "segydirecttr.h"
#include "survinfo.h"
#include "dbman.h"

#include "uisegybulkimporter.h"
#include "uisegydef.h"
#include "uisegydirectinserter.h"
#include "uisegywriteopts.h"
#include "uisegysip.h"
#include "uisegysipclassic.h"
#include "uisegydefdlg.h"
#include "uisegyexp.h"
#include "uisegyread.h"
#include "uisegyresortdlg.h"
#include "uiwellimpsegyvsp.h"
#include "uisegyreadstarter.h"
#include "uisegyimptype.h"
#include "uisegyread.h"

#include "uifileselector.h"
#include "uiseisfileman.h"
#include "uiseispsman.h"
#include "uisurvinfoed.h"
#include "uimenu.h"
#include "uiodmenumgr.h"
#include "uimsg.h"
#include "uitoolbar.h"
#include "envvars.h"

#include "odplugin.h"

static const char* segyiconid_ = "segy";
static const bool segyclassictoplevel_
			= GetEnvVarYN( "OD_SEGY_CLASSIC_TOPLEVEL" );


mDefODPluginInfo(uiSEGY)
{
    mDefineStaticLocalObject( PluginInfo, retpi,(
	"SEG-Y support",
	"OpendTect",
	"dGB (Bert)",
	"1.0",
	"Supports the SEG-Y format") );
    return &retpi;
}


class uiSEGYMgr	: public CallBacker
{ mODTextTranslationClass(uiSEGYMgr);
public:

			uiSEGYMgr(uiODMain*);
			~uiSEGYMgr();

    uiODMain*		appl_;
    uiODMenuMgr&	mnumgr_;

    void		updateMenu(CallBacker*);
    void		survChg(CallBacker*);
    void		edFiles(CallBacker*);
    void		imp2DCB(CallBacker*);
    void		imp2DPSCB(CallBacker*);
    void		imp3DCB(CallBacker*);
    void		imp3DPSCB(CallBacker*);
    void		impVSPCB(CallBacker*);
    void		exp2DCB(CallBacker*);
    void		exp2DPSCB(CallBacker*);
    void		exp3DCB(CallBacker*);
    void		exp3DPSCB(CallBacker*);
    void		reSortCB(CallBacker*);
    void		readStarterCB(CallBacker*);
    void		bulkImport(CallBacker*);

    void		impClassicCB( CallBacker* )	{ impClassic( false ); }
    void		linkClassicCB( CallBacker* )	{ impClassic( true ); }
    void		impClassic(bool);

    static const uiString sSEGYString( bool imm )
			 { return imm ? tr("SEG-Y") : m3Dots(tr("SEG-Y")); }

};


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)

mDefODPluginSurvRelToolsLoadFn(uiSEGY)
{
    uiSurveyInfoEditor::addInfoProvider( new uiSEGYSurvInfoProvider() );
    if ( segyclassictoplevel_ )
	uiSurveyInfoEditor::addInfoProvider(
			    new uiSEGYClassicSurvInfoProvider() );
}


uiSEGYMgr::uiSEGYMgr( uiODMain* a )
    : mnumgr_(a->menuMgr())
    , appl_(a)
{
    uiSEGYDirectVolOpts::initClass();
    uiSEGYDirectPS3DOpts::initClass();
    uiSEGYDirectVolInserter::initClass();
    uiSEGYDirect2DInserter::initClass();
    uiSEGYDirectPS3DInserter::initClass();

    uiSeisFileMan::BrowserDef* bdef = new uiSeisFileMan::BrowserDef(
				SEGYDirectSeisTrcTranslator::translKey() );
    bdef->tooltip_ = tr("Change file/directory names in SEG-Y file %1");
    bdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisFileMan::addBrowser( bdef );

    uiSeisPreStackMan::BrowserDef* psbdef = new uiSeisPreStackMan::BrowserDef(
				SEGYDirectSeisPS3DTranslator::translKey() );
    psbdef->tooltip_ = tr("Change file/directory names in SEG-Y file %1");
    psbdef->cb_ = muiSEGYMgrCB(edFiles);
    uiSeisPreStackMan::addBrowser( psbdef );

    mCallODPluginSurvRelToolsLoadFn( uiSEGY );
    mAttachCB( DBM().surveyChanged, uiSEGYMgr::updateMenu );

    updateMenu(0);
}


uiSEGYMgr::~uiSEGYMgr()
{
    detachAllNotifiers();
}


#define muiSEGYMgrCB(fn) mCB(this,uiSEGYMgr,fn)

void uiSEGYMgr::updateMenu( CallBacker* )
{
    const bool have2d = DBM().isBad() || SI().has2D();
    const bool only2d = !DBM().isBad() && !SI().has3D();
    uiMenu* impseismnu = mnumgr_.getMnu( true, uiODApplMgr::Seis );
    uiMenu* impsgymnu = new uiMenu( appl_, sSEGYString(true), segyiconid_ );
    impseismnu->insertItem( impsgymnu );
    uiMenu* expseismnu = mnumgr_.getMnu( false, uiODApplMgr::Seis );
    uiMenu* expsgymnu = new uiMenu( appl_, sSEGYString(true), segyiconid_ );
    expseismnu->insertItem( expsgymnu );

    if ( have2d )
    {
	const char* lineicid = "seismicline2d";
	const char* linepsicid = "prestackdataset2d";
	uiString linestr = only2d ? m3Dots(tr("Line(s)"))
				  : m3Dots(uiStrings::s2D());
	uiString linepsstr = only2d ? m3Dots(tr("Pre-Stack Data"))
				: m3Dots(tr("Pre-Stack 2D"));

	impsgymnu->insertItem( new uiAction( linestr, muiSEGYMgrCB(imp2DCB),
				lineicid ) );
	impsgymnu->insertItem( new uiAction( linepsstr, muiSEGYMgrCB(imp2DPSCB),
				linepsicid ) );
	expsgymnu->insertItem( new uiAction( linestr, muiSEGYMgrCB(exp2DCB),
				lineicid ) );
	expsgymnu->insertItem( new uiAction( linepsstr, muiSEGYMgrCB(exp2DPSCB),
				linepsicid ) );
    }

    if ( !only2d )
    {
	const char* volicid = "seismiccube";
	uiString volstr = have2d ? m3Dots(uiStrings::s3D())
				 : m3Dots(uiStrings::sVolume());
	const char* volpsicid = "prestackdataset";
	uiString volpsstr = have2d ? m3Dots(tr("PreStack 3D"))
				: m3Dots(tr("Pre-Stack Volume"));

	impsgymnu->insertItem( new uiAction(volstr,muiSEGYMgrCB(imp3DCB),
					volicid) );
        impsgymnu->insertItem( new uiAction(volpsstr,muiSEGYMgrCB(imp3DPSCB),
					volpsicid) );

        expsgymnu->insertItem( new uiAction(volstr,muiSEGYMgrCB(exp3DCB),
					volicid) );
        expsgymnu->insertItem( new uiAction(volpsstr,muiSEGYMgrCB(exp3DPSCB),
					volpsicid) );
    }

    mnumgr_.getMnu( true, uiODApplMgr::Wll )->insertItem(
	new uiAction( m3Dots(tr("VSP (SEG-Y)")), muiSEGYMgrCB(impVSPCB),
			"vsp0" ) );
    mnumgr_.createSeisOutputMenu()->insertItem(
	new uiAction(m3Dots(tr("Re-sort Scanned SEG-Y")),
			muiSEGYMgrCB(reSortCB)) );

    uiString classicmnutitle = segyclassictoplevel_ ? tr("SEG-Y [Classic]")
						   : tr("Classic tool");
    uiMenu* impclassmnu = new uiMenu( appl_, classicmnutitle, "launch" );
    (segyclassictoplevel_ ? impseismnu : impsgymnu)->insertItem( impclassmnu );
    impclassmnu->insertItem( new uiAction( uiStrings::sImport(),
		   muiSEGYMgrCB(impClassicCB), "import") );
    impclassmnu->insertItem( new uiAction( tr("Link"),
		   muiSEGYMgrCB(linkClassicCB), "link") );

    int segyimp = mnumgr_.dtectTB()->addButton( segyiconid_,
						tr("SEG-Y import") );

    uiMenu* mnu = new uiMenu();
    mnu->insertItem(new uiAction(m3Dots(tr("Single-Vintage")),
		    mCB(this,uiSEGYMgr,readStarterCB),"singlefile") );
    mnu->insertItem(new uiAction(m3Dots(tr("Multiple-Vintage")),
		    mCB(this,uiSEGYMgr,bulkImport),"copyobj") );
    mnumgr_.dtectTB()->setButtonMenu( segyimp, mnu, uiToolButton::InstantPopup);

}


#define mImplImpCB(typ,arg) \
void uiSEGYMgr::imp##typ##CB( CallBacker* ) \
{ \
    const SEGY::ImpType imptyp( arg ); \
    uiSEGYReadStarter dlg( appl_, false, &imptyp ); \
    dlg.go(); \
}

mImplImpCB( 2D, Seis::Line )
mImplImpCB( 3D, Seis::Vol )
mImplImpCB( 2DPS, Seis::LinePS )
mImplImpCB( 3DPS, Seis::VolPS )
mImplImpCB( VSP, true )


#define mImplExpCB(typ,arg) \
void uiSEGYMgr::exp##typ##CB( CallBacker* ) \
{ \
    uiSEGYExp dlg( appl_, arg ); \
    dlg.go(); \
}

mImplExpCB( 2D, Seis::Line )
mImplExpCB( 2DPS, Seis::LinePS )
mImplExpCB( 3D, Seis::Vol )
mImplExpCB( 3DPS, Seis::VolPS )


void uiSEGYMgr::impClassic( bool islink )
{
    uiSEGYRead::Setup su( islink ? uiSEGYRead::DirectDef : uiSEGYRead::Import );
    if ( islink )
	su.geoms_ -= Seis::Line;
    new uiSEGYRead( appl_, su );
}


void uiSEGYMgr::reSortCB( CallBacker* )
{
    uiResortSEGYDlg dlg( appl_ );
    dlg.go();
}


void uiSEGYMgr::edFiles( CallBacker* cb )
{
    mDynamicCastGet(uiObjFileMan*,sfm,cb)
    if ( !sfm || !sfm->curIOObj() )
	return;

    uiEditSEGYFileDataDlg dlg( sfm, *sfm->curIOObj() );
    dlg.go();
}


void uiSEGYMgr::readStarterCB( CallBacker* cb )
{
    uiSEGYReadStarter readstdlg( ODMainWin(), false );
    readstdlg.go();
}


void uiSEGYMgr::bulkImport( CallBacker* )
{
    uiFileSelector::Setup fssu;
    fssu.selectMultiFile()
	.formats( uiSEGYFileSpec::fileFmts() );
    uiFileSelector uifs( ODMainWin(), fssu );
    uifs.caption() = tr( "Select SEG-Y files" );
    uifs.go();

    BufferStringSet selfiles;
    uifs.getSelected( selfiles );
    if ( selfiles.isEmpty() )
	return;

    uiSEGYBulkImporter bulkimpdlg( ODMainWin(), selfiles );
    bulkimpdlg.go();
}

mDefODInitPlugin(uiSEGY)
{
    mDefineStaticLocalObject( PtrMan<uiSEGYMgr>, theinst_, = 0 );
    if ( theinst_ ) return 0;

    theinst_ = new uiSEGYMgr( ODMainWin() );
    if ( !theinst_ )
	return "Cannot instantiate SEG-Y plugin";

    return 0;
}
