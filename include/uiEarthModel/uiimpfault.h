#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Nanne Hemstra
 Date:          May 2002
________________________________________________________________________

-*/

#include "uiearthmodelmod.h"
#include "uidialog.h"
#include "od_iosfwd.h"

class CtxtIOObj;
class uiFileSel;
class uiGenInput;
class uiIOObjSel;
class uiTableImpDataSel;

namespace EM { class Fault3D; class Fault; }
namespace Table { class FormatDesc; }

/*! \brief Dialog for fault import */

mExpClass(uiEarthModel) uiImportFault : public uiDialog
{ mODTextTranslationClass(uiImportFault);
public:
			~uiImportFault();
    DBKey		getSelID() const;

    Notifier<uiImportFault> importReady;

protected:
			uiImportFault(uiParent*,const char*,bool is2d=false);

    void		createUI();
    void		inputChgd(CallBacker*);
    void		typeSel(CallBacker*);
    void		stickSel(CallBacker*);
    bool		checkInpFlds();
    bool		handleAscii();
    bool		handleLMKAscii();
    virtual bool	acceptOK() { return false; }
    virtual bool	getFromAscIO(od_istream&,EM::Fault&);
    EM::Fault*		createFault() const;

    uiFileSel*		infld_;
    uiFileSel*		formatfld_;
    uiGenInput*		typefld_;
    uiIOObjSel*		outfld_;
    uiGenInput*		sortsticksfld_;
    uiGenInput*		stickselfld_;
    uiGenInput*		thresholdfld_;
    CtxtIOObj&		ctio_;
    Table::FormatDesc*	fd_;
    uiTableImpDataSel*	dataselfld_;
    bool		isfss_;
    const char*		type_;
    bool		is2d_;

    static const char*	sKeyAutoStickSel();
    static const char*	sKeyInlCrlSep();
    static const char*	sKeySlopeThres();
    static const char*  sKeyGeometric();
    static const char*  sKeyIndexed();
    static const char*  sKeyFileOrder();
};


/*Brief Dialog for 3D Fault*/
mExpClass(uiEarthModel) uiImportFault3D : public uiImportFault
{
public:
			uiImportFault3D(uiParent*,const char* type);
protected:
    bool		acceptOK();
};


/*Brief Dialog for 2D FaultStickSet*/

mExpClass(uiEarthModel) uiImportFaultStickSet2D : public uiImportFault
{
public:
			uiImportFaultStickSet2D(uiParent*,const char* type);

protected:

    bool		acceptOK();
    bool		getFromAscIO(od_istream&,EM::Fault&);

};
