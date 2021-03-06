
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Arnaud Huck
 Date:		January 2017
________________________________________________________________________

-*/
static const char* rcsID mUsedVar = "$Id$";

#include "uiconstvel.h"

#include "uistring.h"
#include "unitofmeasure.h"

namespace Vel
{

float getGUIDefaultVelocity()
{
    return SI().depthsInFeet() ? 8000.f : 2000.f;
}

} // namespace Vel



uiConstantVel::uiConstantVel( uiParent* p, float defvel, const uiString& lbl )
    : uiGenInput(p,tr( "%1 %2" )
		   .arg(lbl.isEmpty() ? uiStrings::sVelocity() : lbl)
		   .arg(UnitOfMeasure::surveyDefVelUnitAnnot(true,true)),
		   FloatInpSpec(defvel) )
{}


void uiConstantVel::setInternalVelocity( float vel )
{
    this->setValue(
	     getConvertedValue( vel, 0, UnitOfMeasure::surveyDefVelUnit() ) );
}


float uiConstantVel::getInternalVelocity() const
{
    return getConvertedValue( this->getFValue(),
			      UnitOfMeasure::surveyDefVelUnit(), 0 );
}
