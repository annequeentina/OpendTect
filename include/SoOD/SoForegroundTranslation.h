#ifndef SoForegroundTranslation_h
#define SoForegroundTranslation_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Kristofer Tingdahl
 Date:		4-11-2002
 RCS:		$Id: SoForegroundTranslation.h,v 1.5 2004-11-23 14:59:08 nanne Exp $
________________________________________________________________________


-*/

#include "Inventor/nodes/SoNode.h"
#include "Inventor/nodes/SoSubNode.h"
#include "Inventor/fields/SoSFFloat.h"

/*!\brief
A class that moves the objects further down the traversal towards the camera,
so they can be clearly viewed. An example is wireframes that should be visible
in front of the camera, and not mixed with the triangles of the surface that
it represents.

*/

class SoForegroundTranslation : public SoNode
{
    typedef SoNode		inherited;
    SO_NODE_HEADER(SoForegroundTranslation);
public:
    				SoForegroundTranslation();
    static void			initClass();

    SoSFFloat			lift;
    				//!<Distance that the object should be moved
    				

protected:
    void	GLRender(SoGLRenderAction*);
    void	getBoundingBox(SoGetBoundingBoxAction*);
    void	callback(SoCallbackAction*);
    void	getMatrix(SoGetMatrixAction*);
    void	pick(SoPickAction*);
    void	getPrimitiveCount(SoGetPrimitiveCountAction*);

    void	doAction(SoAction*);
};

#endif

