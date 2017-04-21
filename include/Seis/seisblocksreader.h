#pragma once

/*
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:	Bert
 Date:		Apr 2017
________________________________________________________________________

*/

#include "seisblocks.h"
#include "ranges.h"
#include "threadlock.h"
#include "uistring.h"

class SeisTrc;
namespace PosInfo { class CubeData; class CubeDataPos; }


namespace Seis
{

class SelData;

namespace Blocks
{

class FileColumn;

/*!\brief Reads data from Blocks Storage.

  if ( state().isError() ) do not attempt anything.

  The get() and getNext() should be MT-safe, but reading is not parallel.

*/

mExpClass(Seis) Reader : public IOClass
{ mODTextTranslationClass(Seis::Blocks::Reader);
public:

    typedef PosInfo::CubeData	    CubeData;

			Reader(const char* fnm_or_dirnm);
			~Reader();

    const uiRetVal&	state() const		    { return state_; }

    const SurvGeom&	survGeom() const	    { return *survgeom_; }
    const CubeData&	positions() const	    { return cubedata_; }
    Interval<float>	zRange() const		    { return zrg_; }
    Interval<int>	inlRange() const	    { return inlrg_; }
    Interval<int>	crlRange() const	    { return crlrg_; }
    inline int		nrComponents() const
			{ return componentNames().size(); }

    void		setSelData(const SelData*);

    uiRetVal		get(const BinID&,SeisTrc&) const;
    uiRetVal		getNext(SeisTrc&) const;

protected:

    typedef PosInfo::CubeDataPos    CubeDataPos;

    SurvGeom*		survgeom_;
    SelData*		seldata_;
    uiRetVal		state_;
    Interval<IdxType>	globinlidxrg_;
    Interval<IdxType>	globcrlidxrg_;
    Interval<IdxType>	globzidxrg_;
    CubeData&		cubedata_;
    CubeDataPos&	curcdpos_;
    Interval<int>	inlrg_;
    Interval<int>	crlrg_;
    Interval<float>	zrg_;
    int			maxnrfiles_;

    virtual void	setEmpty();
    void		readMainFile();
    bool		getGeneralSectionData(const IOPar&);
    bool		reset(uiRetVal&) const;
    bool		isSelected(const CubeDataPos&) const;
    bool		advancePos(CubeDataPos&) const;
    void		doGet(SeisTrc&,uiRetVal&) const;
    FileColumn*		getColumn(const GlobIdx&,uiRetVal&) const;
    void		readTrace(SeisTrc&,uiRetVal&) const;

    friend class	FileColumn;

};


} // namespace Blocks

} // namespace Seis
