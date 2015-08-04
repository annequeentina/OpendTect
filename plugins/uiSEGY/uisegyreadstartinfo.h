#ifndef uisegyreadstartinfo_h
#define uisegyreadstartinfo_h
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          July 2015
 RCS:           $Id$
________________________________________________________________________

-*/

#include "uisegycommon.h"
#include "uisegyimptype.h"

class DataClipSampler;
namespace SEGY { class ImpType; class uiScanData; class uiScanDef; }
class uiTable;
class uiSpinBox;
class uiLineEdit;
class uiComboBox;
class uiFileInput;
class uiSEGYByteNr;
class uiSEGYImpType;
class uiHistogramDisplay;


/*!\brief Starts reading process of 'any SEG-Y file'. */

mExpClass(uiSEGY) uiSEGYReadStartInfo : public uiGroup
{ mODTextTranslationClass(uiSEGYReadStartInfo);
public:

			uiSEGYReadStartInfo(uiParent*,SEGY::uiScanDef&);

    void		setImpTypIdx(int);
    void		setScanData(const SEGY::uiScanData&);

    void		useScanDef();
    void		fillScanDef();

    Notifier<uiSEGYReadStartInfo> scandefChanged;

    void		clearInfo();

protected:

    uiTable*		tbl_;
    uiComboBox*		revfld_;
    uiComboBox*		fmtfld_;
    uiSpinBox*		nsfld_;
    uiLineEdit*		zstartfld_;
    uiLineEdit*		srfld_;
    uiSEGYByteNr*	xcoordbytefld_;
    uiSEGYByteNr*	ycoordbytefld_;
    uiSEGYByteNr*	key1bytefld_;
    uiSEGYByteNr*	key2bytefld_;
    uiSEGYByteNr*	offsetbytefld_;

    SEGY::uiScanDef&	scandef_;
    SEGY::ImpType	imptype_;
    bool		parsbeingset_;

    void		parChg(CallBacker*);
    void		setCellTxt(int col,int row,const char*);

};


#endif
