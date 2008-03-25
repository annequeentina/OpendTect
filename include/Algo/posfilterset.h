#ifndef posfilterset_h
#define posfilterset_h

/*+
________________________________________________________________________

 CopyRight:     (C) dGB Beheer B.V.
 Author:        Bert
 Date:          Feb 2008
 RCS:           $Id: posfilterset.h,v 1.6 2008-03-25 11:40:39 cvsnanne Exp $
________________________________________________________________________


-*/

#include "posfilter.h"




namespace Pos
{

/*!\brief Set of Filters. Owns the Filters. */

class FilterSet : public virtual Filter
{
public:

    virtual		~FilterSet();

    void		add(Filter*);
    void		add(const IOPar&);

    static const char*	typeStr();		//!< "Set"

    virtual bool	initialize(TaskRunner*);
    virtual void	reset();
    virtual bool	includes(const Coord&,float) const;
    virtual float	adjustedZ(const Coord&,float) const;
    virtual bool	hasZAdjustment() const;
    virtual void	fillPar(IOPar&) const;
    virtual void	usePar(const IOPar&);
    virtual void	getSummary(BufferString&) const;
    virtual float	estRatio(const Provider&) const;

    bool		isEmpty() const	{ return filts_.isEmpty(); }
    int			size() const	{ return filts_.size(); }

    ObjectSet<Filter>& filters()			{ return filts_; }
    const ObjectSet<Filter>& filters() const	{ return filts_; }

protected:

    ObjectSet<Filter>	filts_;

    void		copyFrom(const FilterSet&);

};


#define mSimpPosFilterSetDefFns(dim) \
			FilterSet##dim()	{} \
			FilterSet##dim( const FilterSet##dim& fs ) \
					{ *this = fs; } \
    FilterSet##dim&	operator =( const FilterSet##dim& fs ) \
					{ copyFrom(fs); return *this; } \
    virtual Filter*	clone() const	{ return new FilterSet##dim(*this); } \
    virtual const char* type() const	{ return typeStr(); } \
    virtual bool	includes( const Coord& c, float z=1e30 ) const \
			{ return FilterSet::includes(c,z); } \


class FilterSet3D : public FilterSet
		  , public Filter3D
{
public:

    virtual bool	is2D() const	{ return false; }
    virtual bool	includes(const BinID&,float z=mUdf(float)) const;

    mSimpPosFilterSetDefFns(3D)

};


class FilterSet2D : public FilterSet
		  , public Filter2D
{
public:

    virtual bool	is2D() const	{ return true; }
    virtual bool	includes(int,float z=mUdf(float)) const;

    mSimpPosFilterSetDefFns(2D)

};


} // namespace

#endif
