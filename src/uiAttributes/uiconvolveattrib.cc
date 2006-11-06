/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Helene Huck
 Date:          November 2006
 RCS:           $Id: uiconvolveattrib.cc,v 1.2 2006-11-06 14:59:22 cvshelene Exp $
________________________________________________________________________

-*/

#include "uiconvolveattrib.h"
#include "convolveattrib.h"

#include "attribdesc.h"
#include "attribparam.h"
#include "attribfactory.h"
#include "uiattribfactory.h"
#include "uiattrsel.h"
#include "uigeninput.h"
#include "uispinbox.h"

using namespace Attrib;

const int cMinVal = 3;
const int cMaxVal = 29;
const int cStepVal = 2;

static const char* kerstrs[] =
{
        "LowPass (smoothing filter)",
        "Laplacian (edge enhancing filter)",
        "Prewitt (gradient filter)",
        0
};


static const char* outpstrs[] =
{
	"Average gradient",
        "Inline gradient",
        "Crossline gradient",
        "Time gradient",
        0
};


mInitAttribUI(uiConvolveAttrib,Convolve,"Convolve",sKeyFilterGrp)


uiConvolveAttrib::uiConvolveAttrib( uiParent* p )
	: uiAttrDescEd(p)
{
    inpfld_ = getInpFld();

    kernelfld_ = new uiGenInput( this, "Filter type",
                                StringListInpSpec( kerstrs ) );
    kernelfld_->attach( alignedBelow, inpfld_ );
    kernelfld_->valuechanged.notify( mCB(this,uiConvolveAttrib,kernelSel) );

    szfld_ = new uiLabeledSpinBox( this, "Filter size" );
    szfld_->box()->setMinValue( cMinVal );
    szfld_->box()->setStep( cStepVal, true );
    szfld_->attach( alignedBelow, kernelfld_ );

    shapefld_ = new uiGenInput( this, "Shape", BoolInpSpec( "Sphere", "Cube" ));
    shapefld_->attach( alignedBelow, szfld_ );

    outpfld_ = new uiGenInput( this, "Output", StringListInpSpec(outpstrs) );
    outpfld_->attach( alignedBelow, kernelfld_ );

    kernelSel(0);
    setHAlignObj( inpfld_ );
}


void uiConvolveAttrib::set2D( bool yn )
{
    inpfld_->set2D( yn );

    const int oldval = outpfld_->getIntValue();
    BufferStringSet strs;
    if ( yn )
    {
	strs.add( "Line gradient" );
	strs.add( outpstrs[3] );
    }
    else
	strs = outpstrs;

    outpfld_->newSpec( StringListInpSpec(strs), 0 );
    outpfld_->setValue( oldval );
}


void uiConvolveAttrib::kernelSel( CallBacker* cb )
{
    int kernelval = kernelfld_->getIntValue();

    szfld_->display( kernelval != 2 );
    szfld_->box()->setMaxValue( cMaxVal );
    shapefld_->display( kernelval < 2 );
    outpfld_->display( kernelval == 2 );
}


bool uiConvolveAttrib::setParameters( const Desc& desc )
{
    if ( strcmp(desc.attribName(),Convolve::attribName()) )
	return false;

    mIfGetEnum( Convolve::kernelStr(), kernel, kernelfld_->setValue(kernel) )
    mIfGetEnum( Convolve::shapeStr(), shape, shapefld_->setValue(shape) )
    mIfGetInt( Convolve::sizeStr(), size, szfld_->box()->setValue(size) )

    kernelSel(0);
    return true;
}


bool uiConvolveAttrib::setInput( const Desc& desc )
{
    putInp( inpfld_, desc, 0 );
    return true;
}


bool uiConvolveAttrib::setOutput( const Desc& desc )
{
    if ( kernelfld_->getIntValue() == 2 )
    {
	const int selout = desc.selectedOutput();
	outpfld_->setValue( inpfld_->is2D() ? selout-2 : selout );
    }

    return true;
}


bool uiConvolveAttrib::getParameters( Desc& desc )
{
    if ( strcmp(desc.attribName(),Convolve::attribName()) )
	return false;

    mSetEnum( Convolve::kernelStr(), kernelfld_->getIntValue() );
    mSetEnum( Convolve::shapeStr(), shapefld_->getIntValue() );
    mSetInt( Convolve::sizeStr(), szfld_->box()->getValue() );

    return true;
}


bool uiConvolveAttrib::getInput( Desc& desc )
{
    fillInp( inpfld_, desc, 0 );
    return true;
}


bool uiConvolveAttrib::getOutput( Desc& desc )
{
    int selout = 0;
    if ( kernelfld_->getIntValue() == 2 )
    {
	const int index = outpfld_->getIntValue();
	selout = inpfld_->is2D() ? index + 2 : index;
    }
    
    fillOutput( desc, selout );
    return true;
}


void uiConvolveAttrib::getEvalParams( TypeSet<EvalParam>& params ) const
{
    if ( kernelfld_->getIntValue() != 2 )
	params += EvalParam( filterszstr, Convolve::sizeStr() );
}
