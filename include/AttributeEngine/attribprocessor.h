#pragma once

/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        Kristofer Tingdahl
 Date:          07-10-1999
________________________________________________________________________

-*/

#include "attributeenginemod.h"
#include "executor.h"
#include "binid.h"

class TrcKeyZSampling;
class SeisTrcInfo;
namespace Seis { class SelData; }

namespace Attrib
{
class DataHolder;
class Desc;
class Output;
class Provider;

/*!
\brief Attribute Processor
*/

mExpClass(AttributeEngine) Processor : public Executor
{ mODTextTranslationClass(Processor)
public:
				Processor(Desc&,const char* linenm,
					  uiString& errmsg);
				~Processor();

    virtual bool		isOK() const;
    void			addOutput(Output*);
    void			setLineName(const char*);

    int				nextStep();
    void			init();
    od_int64			totalNr() const;
    od_int64			nrDone() const;
    uiString			message() const;
    uiString			nrDoneText() const
				{ return tr("Positions processed"); }

    void			addOutputInterest(int sel);
    bool			setZIntervals(TypeSet< Interval<int> >&,
					      const TrcKey&,const Coord&);
    void			computeAndSetRefZStepAndZ0();

    Notifier<Attrib::Processor>	moveonly;
				/*!< triggered after a position is reached that
				     requires no processing, e.g. during initial
				     buffer fills. */

    const char*			getAttribName() const;
    const char*			getAttribUserRef() const;
    Provider*			getProvider()		{ return provider_; }
    ObjectSet<Output>	outputs_;

    void			setRdmPaths(TypeSet<BinID>* truepath,
					    TypeSet<BinID>* snappedpath);
				//for directional attributes

    void			showDataAvailabilityErrors(bool yn);

protected:
    void		useFullProcess(int&);
    void		useSCProcess(int&);
    void		fullProcess(const SeisTrcInfo*);

    void		defineGlobalOutputSpecs(TypeSet<int>&,TrcKeyZSampling&);
    void		prepareForTableOutput();
    void		computeAndSetPosAndDesVol(TrcKeyZSampling&);

    bool		isHidingDataAvailabilityError() const;

    Desc&		desc_;
    Provider*		provider_;
    int			nriter_;
    int			nrdone_;
    bool		is2d_;
    TypeSet<int>	outpinterest_;
    uiString		errmsg_;
    bool		isinited_;
    bool		useshortcuts_;

    BinID		prevbid_;
    Seis::SelData*	sd_;

    bool		showdataavailabilityerrors_;
};


} // namespace Attrib
