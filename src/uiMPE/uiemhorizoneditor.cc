/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id: uiemhorizoneditor.cc,v 1.5 2005-08-05 01:37:57 cvsduntao Exp $
________________________________________________________________________

-*/

#include "uiemhorizoneditor.h"

#include "datainpspec.h"
#include "emhorizoneditor.h"
#include "emsurfaceedgelineimpl.h"
#include "emsurfacegeometry.h"
#include "emsurface.h"
#include "uidialog.h"
#include "uigeninput.h"
#include "uimpepartserv.h"
#include "uimsg.h"

namespace MPE
{


class uiEMHorizonEditorSetting : public uiDialog
{
public:
    		uiEMHorizonEditorSetting( uiParent*, HorizonEditor* );

protected:
    bool	acceptOK(CallBacker*);

    uiGenInput*		horshapefld;
    uiGenInput*		horsizefld;
    uiGenInput*		vertshapefld;
    HorizonEditor*	editor;
};


uiEMHorizonEditorSetting::uiEMHorizonEditorSetting( uiParent* p,
						    HorizonEditor* he )
    : uiDialog( p, uiDialog::Setup("Horizon Editor","Settings") )
    , editor( he )
{
    horshapefld = new uiGenInput( this, "Horizontal shape",
	    			  BoolInpSpec("Box", "Ellipse",
				  editor->boxEditArea() ) );

    horsizefld = new uiGenInput( this, "Horizontal size",
	    BinIDCoordInpSpec( false, true, false,
			       editor->getEditArea().row,
			       editor->getEditArea().col) );
    horsizefld->attach( alignedBelow, horshapefld );

    if ( editor->getVertMovingStyleNames() )
    {
	StringListInpSpec spec( *editor->getVertMovingStyleNames() );
	spec.setValue( editor->getVertMovingStyle() );
	vertshapefld =  new uiGenInput( this, "Vertical shape", spec );
	vertshapefld->attach( alignedBelow, horsizefld );
    }
    else
	vertshapefld = 0;
}


bool uiEMHorizonEditorSetting::acceptOK(CallBacker*)
{
    const RowCol rc( horsizefld->getIntValue(0), horsizefld->getIntValue(1) );
    const Interval<int> range( 0,25 );
    if ( !range.includes(rc.row) || !range.includes(rc.col) )
    {
	BufferString msg = "Allowed size is 0 to ";
	msg += range.stop;
	msg += ".";
	uiMSG().error(msg);
	return false;
    }

    editor->setEditArea(rc);
    if ( vertshapefld ) editor->setVertMovingStyle(vertshapefld->getIntValue());
    editor->setBoxEditArea(horshapefld->getBoolValue());

    return true;
}


void uiEMHorizonEditor::initClass()
{
    uiMPE().editorfact.addFactory( uiEMHorizonEditor::create );
}


uiEMEditor* uiEMHorizonEditor::create( uiParent* p, MPE::ObjectEditor* e )
{
    mDynamicCastGet( MPE::HorizonEditor*, he, e );
    return he ? new uiEMHorizonEditor( p, he ) : 0;
}


uiEMHorizonEditor::uiEMHorizonEditor( uiParent* p, MPE::HorizonEditor* he )
    : uiEMObjectEditor( p, he )
    , editsettingsmnuitem( "Editor settings" )
    , splitsectionmnuitem( "Split" )
    , makestoplinemnuitem( "Disable Tracking" )
{}


void uiEMHorizonEditor::createNodeMenus(CallBacker* cb)
{
    if ( node.objectID()==-1 ) return;
    mDynamicCastGet( MenuHandler*, menu, cb );

    uiEMObjectEditor::createNodeMenus(cb);

    mAddMenuItem( menu, &editsettingsmnuitem, true, false );
}


void uiEMHorizonEditor::handleNodeMenus(CallBacker* cb)
{
    uiEMObjectEditor::handleNodeMenus(cb);
    
    mCBCapsuleUnpackWithCaller( int, mnuid, caller, cb );
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    if ( mnuid==editsettingsmnuitem.id )
    {
	menu->setIsHandled(true);

	uiEMHorizonEditorSetting dlg( parent,
		reinterpret_cast<HorizonEditor*>( editor) );
	dlg.go();
    }
}


void uiEMHorizonEditor::createInteractionLineMenus(CallBacker* cb)
{
    mDynamicCastGet( MenuHandler*, menu, cb );

    uiEMObjectEditor::createInteractionLineMenus(cb);

    const EM::EdgeLineSegment& interactionline =
	*editor->getInteractionLine(0)->getLine(0)->getSegment(0);

    const EM::SectionID sid = interactionline.getSection();

    EM::EMObject& emobj = const_cast<EM::EMObject&>(editor->emObject());
    mDynamicCastGet( EM::Surface&, surface, emobj );
    EM::EdgeLineSet* lineset = surface.edgelinesets.getEdgeLineSet(sid,true);
    if ( !lineset )
        return;

    const int mainlineidx = lineset->getMainLine();
    EM::EdgeLine* mainline = lineset->getLine(mainlineidx);
    if ( !mainline )
        return;

    bool noneonedge = false;
    bool canstop = false;
    if ( mainline->getSegment( interactionline.first() )!=-1 &&
	 mainline->getSegment( interactionline.last() )!=-1 )
    {
	noneonedge = true;
    
	for ( int idx=1; idx<interactionline.size()-1; idx++ )
	{
	    const EM::PosID posid( interactionline.getSurface().id(), sid,
				   interactionline[idx].getSerialized());
	    if ( surface.geometry.isAtEdge(posid) )
		noneonedge = false;
	}
    
	int dummy;
	bool dummybool;
	canstop = canMakeStopLine( *lineset, interactionline, dummy, dummybool);
    }

    mAddMenuItem( menu, &splitsectionmnuitem, noneonedge, false );
    mAddMenuItem( menu, &makestoplinemnuitem, canstop, false );
}


void uiEMHorizonEditor::handleInteractionLineMenus(CallBacker* cb)
{
    mCBCapsuleUnpackWithCaller(int,mnuid,caller,cb);
    mDynamicCastGet( MenuHandler*, menu, caller );
    if ( mnuid==-1 || menu->isHandled() )
	return;

    EM::EdgeLineSegment& interactionline =
	*editor->getInteractionLine(0)->getLine(0)->getSegment(0);

    const EM::SectionID sid = interactionline.getSection();
    EM::EMObject& emobj = const_cast<EM::EMObject&>(editor->emObject());
    mDynamicCastGet( EM::Surface&, surface, emobj );
    EM::EdgeLineSet* lineset = surface.edgelinesets.getEdgeLineSet(sid,true);

    if ( !lineset )
        return;


    const int mainlineidx = lineset->getMainLine();
    EM::EdgeLine* mainline = lineset->getLine(mainlineidx);
    if ( !mainline ) return;

    bool handled = false;
    if ( mnuid==splitsectionmnuitem.id )
    {
	const EM::SectionID sid = interactionline.getSection();
	const EM::SectionID newsection = surface.geometry.cloneSection(sid);

	EM::SurfaceConnectLine* part1cut =
	    EM::SurfaceConnectLine::create( surface, sid );
	part1cut->setConnectingSection( newsection );
	part1cut->copyNodesFrom(&interactionline, true );

	EM::SurfaceConnectLine* part2cut =
	    EM::SurfaceConnectLine::create( surface, newsection );
	part2cut->setConnectingSection( sid );
	part2cut->copyNodesFrom(&interactionline, false );

	EM::EdgeLineSet* lineset2 =
	    surface.edgelinesets.getEdgeLineSet(newsection,false); 
										        const int mainlineidx = lineset2->getMainLine();
	EM::EdgeLine* mainline2 = lineset2->getLine(mainlineidx);
	if ( !mainline2 )
	    return;

	mainline->insertSegment(part1cut,-1,true);
	lineset->removeAllNodesOutsideLines();
	mainline2->insertSegment(part2cut,-1,true);
	lineset2->removeAllNodesOutsideLines();
	handled = true;
    }
    else if ( mnuid==makestoplinemnuitem.id )
    {
	int linenr;
	bool forward;
	if ( !canMakeStopLine( *lineset, interactionline, linenr, forward) )
	    return;

	const EM::SectionID sid = interactionline.getSection();

	EM::EdgeLineSegment* terminationsegment =
	    EM::TerminationEdgeLineSegment::create( surface, sid );
	terminationsegment->copyNodesFrom(&interactionline, !forward );

	lineset->getLine(linenr)->insertSegment( terminationsegment, -1, true );
	handled =true;
    }

    if ( handled )
    {
	menu->setIsHandled(true);
	interactionline.removeAll();
    }
}


bool uiEMHorizonEditor::canMakeStopLine( const EM::EdgeLineSet& lineset,
				 const EM::EdgeLineSegment& interactionline,
				 int& linenr, bool& forward ) const
{
    bool canstop = false;
    for ( int idx=0; !canstop && idx<lineset.nrLines(); idx++ )
    {
	const EM::EdgeLine* curline = lineset.getLine(idx);

	int firstsegpos;
	const int firstsegment =
	    curline->getSegment(interactionline[0],&firstsegpos);


	if ( firstsegment==-1 )
	    continue;

	EM::EdgeLineIterator fwditer(*curline,true,firstsegment,firstsegpos);
	if ( !fwditer.isOK() ) continue;
	EM::EdgeLineIterator backiter(*curline,false,firstsegment,firstsegpos );
	if ( !backiter.isOK() ) continue;

	canstop = true;
	for ( int idy=1; canstop && idy<interactionline.size(); idy++ )
	{
	    canstop = false;

	    if ( idy==1 || forward )
	    {
		fwditer.next();
		if ( fwditer.currentRowCol()==interactionline[idy] )
		{
		    if ( idy==1 ) forward = true;
		    canstop = true;
		    continue;
		}
	    }

	    if ( idy==1 || !forward )
	    {
		backiter.next();
		if ( backiter.currentRowCol()==interactionline[idy] )
		{
		    if ( idy==1 ) forward=false;
		    canstop = true;
		}
	    }
	}

	if ( canstop )
	{
	    linenr = idx;
	    return true;
	}
    }

    return false;
}






};
