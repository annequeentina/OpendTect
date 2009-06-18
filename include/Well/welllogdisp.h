#ifndef welllogdisp_h
#define welllogdisp_h

/*+
________________________________________________________________________

 CopyRight:	(C) dGB Beheer B.V.
 Author:	Bruno
 Date:		June 2009
 RCS:		$Id: welllogdisp.h,v 1.1 2009-06-18 14:53:54 cvsbert Exp $
________________________________________________________________________


-*/

#include "ranges.h"
#include "color.h"
#include "bufstring.h"

namespace Well
{

mClass LogDisplayPars
{
public:
			LogDisplayPars( const char* nm=0 )
			    : name_(nm)
 			    , cliprate_(mUdf(float))
			    , range_(mUdf(float),mUdf(float))
			    , nocliprate_(false)	
			    , logarithmic_(false)
			    , repeat_(1)	
			    , repeatovlap_(mUdf(float))
			    , seisstyle_(false)	
			    , linecolor_(Color::White())	
			    , logfill_(false)
	    		    , logfillcolor_(Color::White())
			    , seqname_("")
       			    , singlfillcol_(false)				
						        {}
			~LogDisplayPars()		{}

    BufferString	name_;
    float		cliprate_;	//!< If undef, use range_
    Interval<float>	range_;		//!< If cliprate_ set, filled using it
    bool		logarithmic_;
    bool		seisstyle_;
    bool		nocliprate_;
    bool		logfill_;
    int 		repeat_;
    float		repeatovlap_;
    Color		linecolor_;
    Color		logfillcolor_;
    const char*		seqname_;
    bool 		singlfillcol_;
};


mClass LogDisplayParSet
{
public:
			LogDisplayParSet ()
			{
			    Interval<float> lrg( 0, 0 );
			    Interval<float> rrg( 0, 0 );
			    leftlogpar_ = new LogDisplayPars( "None" );
			    rightlogpar_ = new LogDisplayPars( "None" );
			}
			~LogDisplayParSet()  
			{
			    delete leftlogpar_;
			    delete rightlogpar_;
			}

    LogDisplayPars*	getLeft() const { return leftlogpar_; }
    LogDisplayPars*	getRight() const { return rightlogpar_; }
    void		setLeft( LogDisplayPars* lp ) { leftlogpar_ = lp; }
    void		setRight( LogDisplayPars* rp ) { rightlogpar_ = rp; }

protected:

    LogDisplayPars*	leftlogpar_;
    LogDisplayPars*	rightlogpar_;

};

}; // namespace Well

#endif
