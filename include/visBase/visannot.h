#ifndef visannot_h
#define visannot_h

/*+
________________________________________________________________________

 CopyRight:	(C) de Groot-Bril Earth Sciences B.V.
 Author:	Kris Tingdahl
 Date:		Jan 2002
 RCS:		$Id: visannot.h,v 1.3 2002-02-26 17:54:40 kristofer Exp $
________________________________________________________________________


-*/


#include "visobject.h"
#include "sets.h"
#include "draw.h"

class SoCoordinate3;
class SoText2;
class SoTranslation;
class SoDrawStyle;

namespace visBase
{

/*!\brief
    Annotation is a simple wireframe box with text on its
    axis.
*/

class Annotation : public VisualObject
{
public:

				Annotation( Scene &);
    void			setCorner( int, float, float, float );
    void			setText( int dim, const char * );
    void			setLineStyle( const LineStyle& );
    const LineStyle&		lineStyle() const { return linestyle; }

protected:
    LineStyle			linestyle;
    void			updateLineStyle();

    void			updateTextPos();
    SoCoordinate3*		coords;

    ObjectSet<SoText2>		texts;
    ObjectSet<SoTranslation>	textpositions;

};

};

#endif
