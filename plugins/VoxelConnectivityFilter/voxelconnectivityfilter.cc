/*+
_______________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 AUTHOR:	Kristofer Tingdahl
 DATE:		May 2011
_______________________________________________________________________________

 -*/
static const char* rcsID = "$Id: voxelconnectivityfilter.cc,v 1.1 2011-08-11 09:46:19 cvskris Exp $";

#include "voxelconnectivityfilter.h"

#include "arraynd.h"
#include "iopar.h"
#include "multidimstorage.h"
#include "odmemory.h"
#include "sortedtable.h"
#include "thread.h"

#define mRejectValue		-1
#define mUnassignedValue	0


namespace VolProc
{

DefineEnumNames( VoxelConnectivityFilter, AcceptOutput, 0, "AcceptOutput")
{ "Body-size rank", "Body size", "Value", "Input", 0 };

DefineEnumNames( VoxelConnectivityFilter, Connectivity, 0, "Connectivity")
{ "Faces", "Edges", "Corners", 0 };


void VoxelConnectivityFilter::initClass()
{
    PS().addCreator( createInstance, sFactoryKeyword(), sFactoryDisplayName() );
}


class VoxelConnectivityFilterTask : public ParallelTask
{
public:
    VoxelConnectivityFilterTask( VoxelConnectivityFilter& step,
	    			const Array3D<float>& input,
	    			Array3D<float>& output )
	: input_( input )
	, output_( output )
	, statusarr_( 0 )
	, bodyranking_( 0 )
	, step_( step )
        , bodysizes_( 1, -1 )
	, outputbodies_( 0 )
	, arrinfo_( input_.info() )
	, indexvsbodyid_( 2, 1 )
	, bodyaliases_( 2, 1 )
    {
	indexvsbodyid_.allowDuplicates( false );
	bodyaliases_.allowDuplicates( false );
    }

    ~VoxelConnectivityFilterTask() { releaseData(); }

    od_int64	nrIterations() const { return input_.info().getTotalSz(); }
    int		maxNrThreads() const { return 1; } //Todo: remove
    bool	doPrepare(int);
    bool	doWork(od_int64,od_int64,int);

protected:
    void		addBodyAlias(od_int64,od_int64);
    void		releaseData();

    Threads::Mutex			indexvsbodyidlock_;
    MultiDimStorage<char>		indexvsbodyid_;
    MultiDimStorage<char>		bodyaliases_;

    const Array3D<float>&		input_;
    Array3D<float>&			output_;
    VoxelConnectivityFilter&		step_;
    const ArrayNDInfo&			arrinfo_;

    int*				statusarr_;

    Threads::Barrier			barrier_;

    TypeSet<od_int64>			bodysizes_;
    int*				outputbodies_;
    int*				bodyranking_;
};


void VoxelConnectivityFilterTask::releaseData()
{
    delete [] statusarr_;
    statusarr_ = 0;

    delete [] outputbodies_;
    outputbodies_ = 0;

    delete [] bodyranking_;
    bodyranking_ = 0;
}


bool VoxelConnectivityFilterTask::doPrepare( int nrthreads )
{
    releaseData();
    barrier_.setNrThreads( nrthreads );
    const od_int64 size = input_.info().getTotalSz();
    mTryAlloc( statusarr_, int[size] );
    if ( !statusarr_ )
	return false;

    MemSetter<int> memsetter( statusarr_, mUnassignedValue, size );
    return memsetter.execute();
}

#define mTestValue( val )  if ( !mIsUdf(val) && range.includes( val ) )

#define mHandleNeighbor \
    if ( arrinfo_.validPos( neighbor ) ) \
    { \
	const od_int64 neighboridx = arrinfo_.getOffset( neighbor ); \
	if ( neighboridx<start || neighboridx>stop ) \
	{ \
	    const float neighborval = inputvs->value( neighboridx ); \
	    mTestValue( neighborval ) \
	    { \
		const od_int64 pos[] = { neighboridx, curbodyid }; \
		const bool val = true; \
		indexvsbodyidlock_.lock(); \
		indexvsbodyid_.add( &val, pos ); \
		indexvsbodyidlock_.unLock(); \
	    } \
	} \
	else \
	{ \
	    const od_int64 neighborid = statusarr_[neighboridx]; \
	    if ( neighborid!=mRejectValue && neighborid!=curbodyid ) \
	    { \
		if ( neighborid!=mUnassignedValue ) \
		{ \
		    const od_int64 pos[] = { mMAX(neighborid,curbodyid), \
					     mMIN(neighborid,curbodyid) }; \
		    const bool val = true; \
		    bodyaliases.add( &val, pos ); \
		} \
		else \
		{ \
 \
		    const float neighborval = inputvs->value( neighboridx ); \
		    mTestValue( neighborval ) \
		    { \
			statusarr_[neighboridx] = curbodyid; \
			nradded++; \
			queue += neighboridx; \
			queuebodyids += curbodyid; \
		    } \
		    else \
		    { \
			statusarr_[neighboridx] = mRejectValue; \
			nrdone++; \
		    } \
		} \
	    } \
	} \
    }

#define mDoEdge( dim0, dim1 ) \
    memcpy( neighbor, arrpos, 3*sizeof(int) ); \
    neighbor[dim0]--; \
    neighbor[dim1]--; \
    mHandleNeighbor; \
\
    neighbor[dim1] += 2; \
    mHandleNeighbor; \
\
    neighbor[dim0] += 2; \
    mHandleNeighbor; \
\
    neighbor[dim1] -= 2; \
    mHandleNeighbor

#define mDoCorner( op0, op1, op2 ) \
	    memcpy( neighbor, arrpos, 3*sizeof(int) );  \
	    neighbor[0] op0; \
	    neighbor[1] op1; \
	    neighbor[2] op2; \
	    mHandleNeighbor

bool VoxelConnectivityFilterTask::doWork( od_int64 start, od_int64 stop, int )
{
    const Interval<float> range = step_.getAcceptRange();

    const ValueSeries<float>* inputvs = input_.getStorage();
    const VoxelConnectivityFilter::Connectivity connectivity =
	step_.getConnectivity();

    TypeSet<od_int64> queue;
    TypeSet<int> queuebodyids;

    SortedTable<int,od_int64> bodysize;
    MultiDimStorage<char> bodyaliases( 2, 1 );
    bodyaliases.allowDuplicates( false );

    od_int64 nrdone = 0;

    for ( od_int64 idx=start; idx<=stop; idx++ )
    {
	if ( statusarr_[idx]!=mUnassignedValue ) //Assigned before
	    continue;

	if ( nrdone>1000 )
	{
	    reportNrDone( nrdone );
	    nrdone = 0;
	}

	const float curval = inputvs->value( idx );
	mTestValue( curval )
	{
	    barrier_.mutex().lock();
	    const int curbodyid = bodysizes_.size();
	    bodysizes_ += 0;
	    barrier_.mutex().unLock();

	    statusarr_[idx] = curbodyid;
	    queue += idx;
	    queuebodyids += curbodyid;
	    
	    bodysize.set( curbodyid, 1 );
	}
	else
	{
	    statusarr_[idx] = mRejectValue; //reject
	    nrdone++;
	    continue;
	}

	//run as long as we can
	while ( queue.size() )
	{
	    const int last = queue.size()-1;
	    const od_int64 curpos = queue[last];
	    const int curbodyid = queuebodyids[last];

	    queue.remove( last );
	    queuebodyids.remove( last );

	    if ( statusarr_[curpos]!=curbodyid )
	    {
		pErrMsg("Sanity check failed");
		continue;
	    }

	    int arrpos[3];
	    arrinfo_.getArrayPos( curpos, arrpos );

	    int nradded = 0;

	    //Do faces
	    for ( char dim=0; dim<3; dim++ )
	    {
		int neighbor[] = { arrpos[0], arrpos[1], arrpos[2] };
		neighbor[dim]--;
		mHandleNeighbor;
		neighbor[dim] += 2;
		mHandleNeighbor;
	    }

	    if ( connectivity!=VoxelConnectivityFilter::Faces )
	    {
		//Do edges
		int neighbor[3];
		mDoEdge( 0, 1 );
		mDoEdge( 0, 2 );
		mDoEdge( 1, 2 );
	    }

	    if ( connectivity==VoxelConnectivityFilter::Corners )
	    {
		int neighbor[3];
		mDoCorner( --, --, -- );
		mDoCorner( --, --, ++ );
		mDoCorner( --, ++, -- );
		mDoCorner( --, ++, ++ );
		mDoCorner( ++, --, -- );
		mDoCorner( ++, --, ++ );
		mDoCorner( ++, ++, -- );
		mDoCorner( ++, ++, ++ );
	    }

	    od_int64 cursize;
	    bodysize.get( curbodyid, cursize );
	    bodysize.set( curbodyid, cursize + nradded );

	    if ( nrdone>1000 )
	    {
		reportNrDone( nrdone );
		nrdone = 0;
	    }
	}
    }

    reportNrDone( nrdone );
    nrdone = 0;

    barrier_.waitForAll( true ); //After this nothing more will be added
    				 //to indexvsbodyid_
    //Convert all indexvsbodyid_ entries in my range to bodyaliases 
    
    barrier_.mutex().lock();
    bodyaliases_.append( bodyaliases );
    barrier_.mutex().unLock();

    barrier_.waitForAll( true );

    //Updated shared table of body sizes. Each body id is only updated
    //by one thread, so it can be done in parallel.
    for ( int idx=bodysize.size()-1; idx>=0; idx-- )
	bodysizes_[bodysize.id(idx)] += bodysize.val(idx);

    const int nrbodies = bodysizes_.size();

    const VoxelConnectivityFilter::AcceptOutput acceptoutput =
	step_.getAcceptOutput();

    if ( barrier_.waitForAll( false ) )
    {
	//Create outputbodies_ table. For each body, which id should it have
	//on the output.
	mTryAlloc( outputbodies_, int[nrbodies] );

	if ( !outputbodies_ )
	{
	    barrier_.mutex().unLock();
	    return false;
	}

	for ( int idx=nrbodies-1; idx>=0; idx-- )
	    outputbodies_[idx] = idx;

	for ( int idx=0; idx<bodyaliases_.size(); idx++ )
	{
	    const int pos = bodyaliases_.getPos( idx );
	    const MultiDimStorage<char>& dim1 = *bodyaliases_[idx];
	    int alias = dim1.getPos(0);
	    alias = outputbodies_[alias];

	    outputbodies_[pos] = alias;
	    
	    for ( int idy=1; idy<dim1.size(); idy++ )
	    {
		const int curpos = dim1.getPos(idy);
		if ( curpos<pos )
		    continue;

		outputbodies_[curpos] = alias;
	    }
	}

	//Go through all bodies and compute their size
	for ( int idx=nrbodies-1; idx>=1; idx-- )
	{
	    const int outputbody = outputbodies_[idx];
	    if ( outputbody != idx )
	    {
		bodysizes_[outputbody] += bodysizes_[idx];
		bodysizes_[idx] = 0;
	    }
	}

	//Rank all bodies on size 0 is largest
	if ( acceptoutput==VoxelConnectivityFilter::Ranking )
	{
	    TypeSet<od_int64> bodysizecopy( bodysizes_ );
	    mAllocVarLenIdxArr( int, idxs, nrbodies );
	    sort_coupled( bodysizecopy.arr(), idxs, bodysizecopy.size() );

	    mTryAlloc( bodyranking_, int[nrbodies] );

	    for ( int idx=nrbodies-1; idx>=0; idx-- )
		bodyranking_[idxs[idx]] = nrbodies-idx-1;
	}

	//Filter away all bodies that are too small
	const od_int64 minbodysize = step_.getMinimumBodySize();
	for ( int idx=nrbodies-1; idx>=0; idx-- )
	{
	    if ( bodysizes_[idx]<minbodysize )
	    {
		outputbodies_[idx] = -1;
	    }
	}
    }

    barrier_.mutex().unLock();

    if ( !outputbodies_ )
	return false;

    //Phase 2
    //Go through all bodyids, and find the lowest aliases and replace all
    //instances
    ValueSeries<float>* outputvs = output_.getStorage();

#define mOutputLoopStart \
        if ( nrdone>10000 ) \
	{ \
    	    reportNrDone( nrdone ); \
	    nrdone = 0; \
	} \
	int bodynr = statusarr_[idx]; \
        if ( bodynr!=-1 ) \
    	{ \
    	    bodynr = outputbodies_[bodynr]; \
	    nrdone++; \
	} \
 \
	if ( bodynr==-1 )  \
	{ \
	    outputvs->setValue( idx, step_.getRejectValue() ); \
	    continue; \
	}


    if ( acceptoutput==VoxelConnectivityFilter::Ranking )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    mOutputLoopStart;
	    outputvs->setValue( idx, bodyranking_[bodynr] );
        }
    }
    else if ( acceptoutput==VoxelConnectivityFilter::BodySize )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    mOutputLoopStart;
	    outputvs->setValue( idx, bodysizes_[bodynr] );
        }
    }
    else if ( acceptoutput==VoxelConnectivityFilter::Value )
    {
	const float acceptval = step_.getAcceptValue();
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    mOutputLoopStart;
	    outputvs->setValue( idx, acceptval );
	}
    }
    else if ( acceptoutput==VoxelConnectivityFilter::Input )
    {
	for ( od_int64 idx=start; idx<=stop; idx++ )
	{
	    mOutputLoopStart;
	    outputvs->setValue( idx, inputvs->value( idx ) );
	}
    }

    reportNrDone( nrdone );

    return true;
}



void VoxelConnectivityFilterTask::addBodyAlias( od_int64 id0, od_int64 id1)
{
}


VoxelConnectivityFilter::VoxelConnectivityFilter( Chain& chain )
    : Step( chain )
    , minbodysize_( mUdf(int) )
    , rejectvalue_( mUdf(float) )
    , acceptoutput_( Input )
    , acceptvalue_( mUdf(float) )
    , connectivity_( Corners )
    , range_( mUdf(float), mUdf(float) )
{}


VoxelConnectivityFilter::~VoxelConnectivityFilter()
{ }


Task* VoxelConnectivityFilter::createTask()
{
    return new VoxelConnectivityFilterTask( *this, input_->getCube(0),
	    				    output_->getCube(0) );
}


void VoxelConnectivityFilter::fillPar( IOPar& par ) const
{
    Step::fillPar( par );
    
    par.set( sKeyRange(), range_ );
    par.set( sKeyConnectivity(), toString(connectivity_) );

    par.set( sKeyAcceptOutput(), toString(acceptoutput_) );
    if ( acceptoutput_==Value )
	par.set( sKeyAcceptValue(), acceptvalue_ );

    par.set( sKeyMinimumSize(), minbodysize_ );
    par.set( sKeyRejectValue(), rejectvalue_ );
}


bool VoxelConnectivityFilter::usePar( const IOPar& par )
{
    if ( !Step::usePar( par ) )
	return false;

    return par.get( sKeyRange(), range_ ) &&
	 parseEnum( par, sKeyConnectivity(), connectivity_ ) &&
	 parseEnum( par, sKeyAcceptOutput(), acceptoutput_ ) &&
	 (acceptoutput_!=Value || par.get( sKeyAcceptValue(), acceptvalue_) ) &&
	 par.get( sKeyMinimumSize(), minbodysize_ ) &&
	 par.get( sKeyRejectValue(), rejectvalue_ );
}

}; //Namespace
