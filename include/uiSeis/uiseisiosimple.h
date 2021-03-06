#pragma once
/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : A.H. Bril
 * DATE     : Nov 2003
-*/

#include "uiseismod.h"
#include "uidialog.h"
#include "samplingdata.h"
#include "seisiosimple.h"
#include "dbkey.h"
class IOObjContext;
class uiLabel;
class uiScaler;
class uiSeisSel;
class uiCheckBox;
class uiGenInput;
class uiFileSel;
class uiSeparator;
class uiSeisSubSel;
class uiSeis2DLineNameSel;


mExpClass(uiSeis) uiSeisIOSimple : public uiDialog
{ mODTextTranslationClass(uiSeisIOSimple);
public:

			uiSeisIOSimple(uiParent*,Seis::GeomType,bool isimp);

protected:

    uiFileSel*		fnmfld_;
    uiGenInput*		isascfld_;
    uiGenInput*		is2dfld_;
    uiGenInput*		havesdfld_;
    uiGenInput*		sdfld_;
    uiGenInput*		haveposfld_;
    uiGenInput*		havenrfld_;
    uiGenInput*		haverefnrfld_;
    uiGenInput*		isxyfld_;
    uiGenInput*		inldeffld_;
    uiGenInput*		crldeffld_;
    uiGenInput*		nrdeffld_;
    uiGenInput*		startposfld_;
    uiGenInput*		startnrfld_;
    uiGenInput*		stepposfld_;
    uiGenInput*		stepnrfld_;
    uiGenInput*		offsdeffld_;
    uiGenInput*		remnullfld_;
    uiGenInput*		multcompfld_;
    uiLabel*		pspposlbl_;
    uiCheckBox*		haveoffsbut_;
    uiCheckBox*		haveazimbut_;
    uiScaler*		scalefld_;
    uiSeisSel*		seisfld_;
    uiSeisSubSel*	subselfld_;
    uiSeis2DLineNameSel* lnmfld_;

    IOObjContext&	ctxt_;
    Seis::GeomType	geom_;
    bool		isimp_;

    void		isascSel(CallBacker*);
    void		inpSeisSel(CallBacker*);
    void		lsSel(CallBacker*);
    void		haveposSel(CallBacker*);
    void		havenrSel(CallBacker*);
    void		havesdSel(CallBacker*);
    void		haveoffsSel(CallBacker*);
    void		initFlds(CallBacker*);
    bool		acceptOK();

    static SeisIOSimple::Data&	data2d();
    static SeisIOSimple::Data&	data3d();
    static SeisIOSimple::Data&	dataps();
    SeisIOSimple::Data&	data()
			{ return  geom_ == Seis::Line ? data2d()
			       : (geom_ == Seis::Vol  ? data3d()
						      : dataps()); }

    bool		is2D() const	{ return Seis::is2D(geom_); }
    bool		isPS() const	{ return Seis::isPS(geom_); }

private:

    void		mkIsAscFld();
    uiSeparator*	mkDataManipFlds();

};
