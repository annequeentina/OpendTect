#ifndef uiiosurfacedlg_h
#define uiiosurfacedlg_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Nanne Hemstra
 Date:          July 2003
 RCS:           $Id: uiiosurfacedlg.h,v 1.8 2003-12-18 12:45:15 nanne Exp $
________________________________________________________________________

-*/

#include "uidialog.h"

class IOObj;
class uiSurfaceRead;
class uiSurfaceWrite;
class MultiID;

namespace EM { class Surface; class SurfaceIODataSelection; };


/*! \brief Dialog for horizon export */

class uiWriteSurfaceDlg : public uiDialog
{
public:
			uiWriteSurfaceDlg(uiParent*,const EM::Surface&);

    bool		auxDataOnly() const;
    bool		surfaceOnly() const;
    bool		surfaceAndData() const;
    void		getSelection(EM::SurfaceIODataSelection&);
    IOObj*		ioObj() const;

protected:

    int			auxdataidx;

    uiSurfaceWrite*	iogrp;
    const EM::Surface&	surf;

    bool		checkIfAlreadyPresent(const char*);
    bool		acceptOK(CallBacker*);
};


class uiReadSurfaceDlg : public uiDialog
{
public:
			uiReadSurfaceDlg(uiParent*,bool ishor);

    IOObj*		ioObj() const;
    void		getSelection(EM::SurfaceIODataSelection&);

protected:

    uiSurfaceRead*	iogrp;

    bool		acceptOK(CallBacker*);
};


#endif
