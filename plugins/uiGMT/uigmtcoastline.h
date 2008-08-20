#ifndef uigmtcoastline_h
#define uigmtcoastline_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Raman Singh
 Date:		August 2008
 RCS:		$Id: uigmtcoastline.h,v 1.3 2008-08-20 05:26:14 cvsraman Exp $
________________________________________________________________________

-*/

#include "uigmtoverlay.h"

class uiCheckBox;
class uiColorInput;
class uiComboBox;
class uiGenInput;
class uiSelLineStyle;
class uiSpinBox;

class uiGMTCoastlineGrp : public uiGMTOverlayGrp
{
public:

    static void		initClass();

    bool		fillPar(IOPar&) const;
    bool		usePar(const IOPar&);
protected:

    			uiGMTCoastlineGrp(uiParent*);

    static uiGMTOverlayGrp*	createInstance(uiParent*);
    static int			factoryid_;

    uiSpinBox*		utmfld_;
    uiSpinBox*		cmfld_;
    uiGenInput*		ewfld_;
    uiComboBox*		resolutionfld_;
    uiSelLineStyle*	lsfld_;
    uiCheckBox*		fillwetfld_;
    uiCheckBox*		filldryfld_;
    uiColorInput*	wetcolfld_;
    uiColorInput*	drycolfld_;

    void		fillSel(CallBacker*);
    void		utmSel(CallBacker*);
};

#endif
