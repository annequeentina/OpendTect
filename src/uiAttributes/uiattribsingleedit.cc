/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Bert
 Date:          Jan 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: uiattribsingleedit.cc,v 1.1 2011-01-06 16:19:09 cvsbert Exp $";

#include "uiattribsingleedit.h"
#include "uiattrdesced.h"
#include "attribdesc.h"
#include "attribdescsetman.h"
#include "uigeninput.h"
#include "uimsg.h"


uiSingleAttribEd::uiSingleAttribEd( uiParent* p, Attrib::Desc& ad, bool isnew )
    : uiDialog(p,Setup(isnew ? "Add attribute" : "Edit attribute",
		    "Define attribute parameters",mTODOHelpID))
    , desc_(ad)
    , setman_(new Attrib::DescSetMan(ad.is2D(),ad.descSet(),false))
    , nmchgd_(false)
    , anychg_(false)
{
    desced_ = uiAF().create( this, desc_.attribName(), desc_.is2D(), true );
    desced_->setDesc( &desc_, setman_ );

    namefld_ = new uiGenInput( this, "Name", desc_.userRef() );
    namefld_->attach( alignedBelow, desced_ );
}


uiSingleAttribEd::~uiSingleAttribEd()
{
    delete setman_;
}


bool uiSingleAttribEd::acceptOK( CallBacker* )
{
    const BufferString oldnm( desc_.userRef() );
    const BufferString newnm(  namefld_->text() );
    if ( newnm.isEmpty() )
	{ uiMSG().error( "Please enter a valid name" ); return false; }

    const char* msg = desced_->commit();
    if ( msg && *msg )
	{ uiMSG().error( msg ); return false; }

    desc_.setUserRef( newnm );

    anychg_ = true;
    nmchgd_ = oldnm != newnm;
    return true;
}
