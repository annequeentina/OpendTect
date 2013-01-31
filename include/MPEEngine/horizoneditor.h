#ifndef horizoneditor_h
#define horizoneditor_h
                                                                                
/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        N. Hemstra
 Date:          March 2005
 RCS:           $Id$
________________________________________________________________________

-*/

#include "mpeenginemod.h"
#include "emeditor.h"

#include "bufstringset.h"

namespace EM { class Horizon3D; class Horizon2D; };

namespace MPE
{

/*!
\brief ObjectEditor to edit EM::Horizon3D.
*/

mExpClass(MPEEngine) HorizonEditor : public ObjectEditor
{
public:
    				HorizonEditor(EM::Horizon3D&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

    
    void			getEditIDs(TypeSet<EM::PosID>&) const;
    bool			addEditID( const EM::PosID& );
    bool			removeEditID( const EM::PosID& );

    bool			boxEditArea() const { return horbox; }
    void			setBoxEditArea( bool nb ) { horbox=nb; }

    const RowCol&		getEditArea() const { return editarea; }
    void			setEditArea( const RowCol& rc ) { editarea=rc; }

    const BufferStringSet*	getAlongMovingStyleNames() const;
    int				getAlongMovingStyle() const { return vertstyle;}
    void			setAlongMovingStyle( int nv ) { vertstyle=nv; }

protected:
    virtual			~HorizonEditor();
    void			getAlongMovingNodes( const EM::PosID&,
	    					     TypeSet<EM::PosID>&,
	                                             TypeSet<float>*) const;
    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
    void			emChangeCB( CallBacker* );

    BufferStringSet		vertstylenames;

    RowCol			editarea;
    bool			horbox;
    int				vertstyle;
};


/*!
\brief ObjectEditor to edit EM::Horizon2D.
*/

mExpClass(MPEEngine) Horizon2DEditor : public ObjectEditor
{
public:
				Horizon2DEditor(EM::Horizon2D&);
    static ObjectEditor*	create(EM::EMObject&);
    static void			initClass();

protected:
    Geometry::ElementEditor*	createEditor(const EM::SectionID&);
};


}; // namespace MPE

#endif


