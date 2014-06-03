#ifndef uiseis2dgeom_h
#define uiseis2dgeom_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          January 2002
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uidialog.h"
class IOObj;
class uiSeisSel;
class CtxtIOObj;
class uiGenInput;
class uiFileInput;


mExpClass(uiSeis) uiSeisDump2DGeom : public uiDialog
{ mODTextTranslationClass(uiSeisDump2DGeom);
public:
                        uiSeisDump2DGeom(uiParent*,const IOObj* ioobj=0);
			~uiSeisDump2DGeom();

protected:

    uiSeisSel*		seisfld;
    uiGenInput*		lnmsfld;
    uiFileInput*	outfld;

    CtxtIOObj&		ctio;

    virtual bool	acceptOK(CallBacker*);

    void		seisSel(CallBacker*);
};

#endif

