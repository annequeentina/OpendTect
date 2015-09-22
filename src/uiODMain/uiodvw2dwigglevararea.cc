/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Umesh Sinha
 Date:		June 2010
 RCS:		$Id$
________________________________________________________________________

-*/

#include "uiodvw2dwigglevararea.h"

#include "uiattribpartserv.h"
#include "uiflatviewwin.h"
#include "uiflatviewer.h"
#include "uimenuhandler.h"
#include "uiodapplmgr.h"
#include "uiodviewer2d.h"
#include "uiodviewer2dmgr.h"
#include "uistrings.h"
#include "uitreeview.h"

#include "attribdesc.h"
#include "attribdescset.h"
#include "attribdescsetsholder.h"
#include "ioobj.h"
#include "seisdatapack.h"
#include "seisioobjinfo.h"
#include "survinfo.h"
#include "view2dseismic.h"
#include "view2ddataman.h"


uiODVW2DWiggleVarAreaTreeItem::uiODVW2DWiggleVarAreaTreeItem()
    : uiODVw2DTreeItem( tr("Wiggle") )
    , dummyview_(0)
    , menu_(0)
    , selattrmnuitem_(uiStrings::sSelAttrib())
{}


uiODVW2DWiggleVarAreaTreeItem::~uiODVW2DWiggleVarAreaTreeItem()
{
    if ( viewer2D()->viewwin()->nrViewers() )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
	vwr.dataChanged.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );
    }

    if ( menu_ )
    {
	menu_->createnotifier.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,createMenuCB) );
	menu_->handlenotifier.remove(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,handleMenuCB) );
	menu_->unRef();
    }

    viewer2D()->dataMgr()->removeObject( dummyview_ );
}


const char* uiODVW2DWiggleVarAreaTreeItem::iconName() const
{ return "tree-wva"; }


bool uiODVW2DWiggleVarAreaTreeItem::init()
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return false;

    uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    vwr.dataChanged.notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,dataChangedCB) );

    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( true );
    uitreeviewitem_->setChecked( ddp.wva_.show_ );

    checkStatusChange()->notify(
	    mCB(this,uiODVW2DWiggleVarAreaTreeItem,checkCB) );

    dummyview_ = new VW2DSeis();
    viewer2D()->dataMgr()->addObject( dummyview_ );

    return uiODVw2DTreeItem::init();
}


bool uiODVW2DWiggleVarAreaTreeItem::select()
{
    if ( !uitreeviewitem_->isSelected() )
	return false;

    viewer2D()->dataMgr()->setSelected( dummyview_ );

    return true;
}


void uiODVW2DWiggleVarAreaTreeItem::checkCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
	viewer2D()->viewwin()->viewer(ivwr).setVisible( true, isChecked() );
}


void uiODVW2DWiggleVarAreaTreeItem::dataChangedCB( CallBacker* )
{
    if ( !viewer2D()->viewwin()->nrViewers() )
	return;

    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    const FlatView::DataDispPars& ddp = vwr.appearance().ddpars_;
    uitreeviewitem_->setCheckable( ddp.vd_.show_ && vwr.hasPack(true) );
    uitreeviewitem_->setChecked( ddp.wva_.show_ );
}


void uiODVW2DWiggleVarAreaTreeItem::dataTransformCB( CallBacker* )
{
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(ivwr);
	const TypeSet<DataPack::ID> ids = vwr.availablePacks();
	for ( int idx=0; idx<ids.size(); idx++ )
	    if ( ids[idx]!=vwr.packID(false) && ids[idx]!=vwr.packID(true) )
		vwr.removePack( ids[idx] );
    }

    Attrib::SelSpec& selspec = viewer2D()->selSpec( true );
    if ( selspec.isZTransformed() ) return;

    const DataPack::ID dpid = createDataPack( selspec );
    if ( dpid != DataPack::cNoID() )
	viewer2D()->setUpView( dpid, true );
}


bool uiODVW2DWiggleVarAreaTreeItem::showSubMenu()
{
    if ( !menu_ )
    {
	menu_ = new uiMenuHandler( getUiParent(), -1 );
	menu_->ref();
	menu_->createnotifier.notify(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,createMenuCB) );
	menu_->handlenotifier.notify(
		mCB(this,uiODVW2DWiggleVarAreaTreeItem,handleMenuCB) );
    }
    return menu_->executeMenu(uiMenuHandler::fromTree());
}


void uiODVW2DWiggleVarAreaTreeItem::createMenuCB( CallBacker* cb )
{
    selattrmnuitem_.removeItems();
    createSelMenu( selattrmnuitem_ );

     mDynamicCastGet(MenuHandler*,menu,cb);
     if ( selattrmnuitem_.nrItems() )
	  mAddMenuItem( menu, &selattrmnuitem_, true, false );
}


void uiODVW2DWiggleVarAreaTreeItem::handleMenuCB( CallBacker* cb )
{
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet(MenuHandler*,menu,caller);
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( handleSelMenu(mnuid) )
	menu->setIsHandled(true);
}


void uiODVW2DWiggleVarAreaTreeItem::createSelMenu( MenuItem& mnu )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( true, true );
    if ( !dp ) return;

    const Attrib::SelSpec& as = viewer2D()->selSpec( true );
    MenuItem* subitem = 0;
    applMgr()->attrServer()->resetMenuItems();

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,dp.ptr());
    const bool is2d = regfdp && regfdp->is2D();
    if ( is2d )
    {
//	BufferString lnm;
//	dp2ddh->getLineName( lnm );
// TODO: Use lnm to get attributes for this line only
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,true,false);
    }
    else
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,false);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    subitem = applMgr()->attrServer()->calcAttribMenuItem( as, is2d, true );
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );

    if ( is2d )
    {
//	BufferString lnm;
//	dp2ddh->getLineName( lnm );
// TODO: Use lnm to get attributes for this line only
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,true,true);
    }
    else
	subitem = applMgr()->attrServer()->storedAttribMenuItem(as,false,true);
    mAddMenuItem( &mnu, subitem, subitem->nrItems(), subitem->checked );
}


bool uiODVW2DWiggleVarAreaTreeItem::handleSelMenu( int mnuid )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( true, true );
    if ( !dp ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    bool dousemulticomp, stored, steering;
    dousemulticomp = stored = steering = false;

    BufferString attrbnm;
    mDynamicCastGet(const RegularFlatDataPack*,regfdp,dp.ptr());
    if ( regfdp && regfdp->is2D() )
	attrserv->info2DAttribSubMenu( mnuid, attrbnm, steering, stored );

    Attrib::SelSpec selas = viewer2D()->selSpec( true );
    if ( !stored && !attrserv->handleAttribSubMenu(mnuid,selas,dousemulticomp) )
	return false;

    const DataPack::ID dpid =
	createDataPack( selas, attrbnm.buf(), steering, stored );
    if ( dpid == DataPack::cNoID() ) return false;

    viewer2D()->setSelSpec( &selas, true );
    viewer2D()->useStoredDispPars( true );
    for ( int ivwr=0; ivwr<viewer2D()->viewwin()->nrViewers(); ivwr++ )
    {
	FlatView::DataDispPars& ddpars =
	    viewer2D()->viewwin()->viewer(ivwr).appearance().ddpars_;
	ddpars.wva_.show_ = true;
    }

    viewer2D()->setSelSpec( &selas, true );
    viewer2D()->setUpView( dpid, true );
    return true;
}


DataPack::ID uiODVW2DWiggleVarAreaTreeItem::createDataPack(
			Attrib::SelSpec& selas, const BufferString& attrbnm,
			const bool steering, const bool stored )
{
    const uiFlatViewer& vwr = viewer2D()->viewwin()->viewer(0);
    ConstDataPackRef<FlatDataPack> dp = vwr.obtainPack( true, true );
    if ( !dp ) return false;

    uiAttribPartServer* attrserv = applMgr()->attrServer();
    attrserv->setTargetSelSpec( selas );

    mDynamicCastGet(const RegularFlatDataPack*,regfdp,dp.ptr());
    mDynamicCastGet(const RandomFlatDataPack*,randfdp,dp.ptr());
    if ( regfdp && regfdp->is2D() )
    {
	if ( stored )
	{
	    const SeisIOObjInfo objinfo( attrbnm );
	    if ( !objinfo.ioObj() )
		return DataPack::cNoID();

	    Attrib::DescID attribid = attrserv->getStoredID(
			    objinfo.ioObj()->key(), true, steering ? 1 : 0 );
	    selas.set( attrbnm, attribid, false, 0 );
	    selas.set2DFlag();

	    const Attrib::DescSet* ds = Attrib::DSHolder().getDescSet( true,
								       true );
	    if ( !ds ) return DataPack::cNoID();
	    selas.setRefFromID( *ds );
	    selas.setUserRef( attrbnm );

	    const Attrib::Desc* targetdesc = ds->getDesc( attribid );
	    if ( !targetdesc ) return DataPack::cNoID();

	    BufferString defstring;
	    targetdesc->getDefStr( defstring );
	    selas.setDefString( defstring );
	    attrserv->setTargetSelSpec( selas );
	}
    }
    else if ( randfdp )
    {
	TypeSet<BinID> bids;
	for ( int idx=0; idx<randfdp->getPath().size(); idx++ )
	    bids += randfdp->getPath()[idx].pos();

	const DataPack::ID dpid = attrserv->createRdmTrcsOutput(
					randfdp->getZRange(), &bids, &bids );
	return viewer2D()->createFlatDataPack( dpid, 0 );
    }

    return viewer2D()->createDataPack( selas );
}


uiTreeItem* uiODVW2DWiggleVarAreaTreeItemFactory::createForVis(
					    const uiODViewer2D&, int id ) const
{
    return 0;
}

