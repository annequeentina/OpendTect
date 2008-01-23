/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : Y.C. Liu
 * DATE     : March 2007
-*/

static const char* rcsID = "$Id: uipsviewerpi.cc,v 1.5 2008-01-23 15:11:44 cvsbert Exp $";

#include "plugins.h"
#include "uipsviewermanager.h"
#include "visprestackviewer.h"


extern "C" int GetuiPreStackViewerPluginType()
{
    return PI_AUTO_INIT_LATE;
}


extern "C" PluginInfo* GetuiPreStackViewerPluginInfo()
{
    static PluginInfo retpi = {
    "Pre-Stack Viewer",
    "dGB - Kristofer/Yuancheng",
    "1.1.1",
    "This is the PreStack Viewer in the 3D scene."
	"\nIt can be activated by right-clicking on a plane in the scene." };
    return &retpi;
}


extern "C" const char* InituiPreStackViewerPlugin( int, char** )
{
    PreStackView::PreStackViewer::initClass();
    static PreStackView::uiPSViewerMgr* mgr=0;
    if ( mgr ) return 0;
    mgr = new PreStackView::uiPSViewerMgr();    
    
    return 0; 
}
