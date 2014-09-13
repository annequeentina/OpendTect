#ifndef uisegysip_h
#define uisegysip_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Feb 2004
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uiseismod.h"
#include "uisip.h"
#include "iopar.h"


/* uiSurvInfoProvider taking it's source in (a) SEG-Y file(s) */

mExpClass(uiSeis) uiSEGYSurvInfoProvider : public uiSurvInfoProvider
{
public:

			uiSEGYSurvInfoProvider()
			    : xyinft_(false)	{}

    const char*		usrText() const		{ return "Scan SEG-Y file(s)"; }
    uiDialog*		dialog(uiParent*);
    bool		getInfo(uiDialog*,TrcKeyZSampling&,Coord crd[3]);
    bool		xyInFeet() const	{ return xyinft_; }

    IOPar*		getImportPars() const
			{ return imppars_.isEmpty() ? 0 : new IOPar(imppars_); }
    void		startImport(uiParent*,const IOPar&);
    const char*		importAskQuestion() const;

    friend class	uiSEGYSIPMgrDlg;
    IOPar		imppars_;
    bool		xyinft_;

};


#endif

