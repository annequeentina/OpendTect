/*+
 * (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 * AUTHOR   : K. Tingdahl
 * DATE     : Nov 2013
-*/

static const char* rcsID mUsedVar = "$Id$";

#include "tcpconnection.h"

#include "applicationdata.h"
#include "oscommand.h"
#include "statrand.h"
#include "varlenarray.h"
#include "limits.h"
#include "tcpserver.h"
#include "testprog.h"
#include "thread.h"

#define mRunTcpTest( test, msg ) \
    mRunStandardTest( test, BufferString( prefix_, msg ) )

class TestRunner : public CallBacker
{
public:
    bool	testTcpConnection();

    void	testCallBack(CallBacker*)
    {
	const bool testresult = testTcpConnection();
	if ( exitonfinish_ ) ApplicationData::exit( testresult ? 0 : 1 );
    }

    bool		noeventloop_;
    int			port_;
    const char*		prefix_;
    bool		exitonfinish_;
    BufferString	serverapp_;
    BufferString	serverarg_;
};


bool TestRunner::testTcpConnection()
{
    TcpConnection connection;
    connection.setTimeout( 1000 );
    connection.setNoEventLoop( noeventloop_ );

    if ( !connection.connectToHost( "localhost", port_, true ) )
    {
	if ( !ExecODProgram( serverapp_, serverarg_ ) )
	{
	    od_ostream::logStream() << "Cannot start " << serverapp_;
	    return false;
	}

	Threads::sleep( 15 );
    }
    else
    {
	connection.abort();
    }

    mRunTcpTest(
	    !connection.connectToHost( "non_existing_host", 20000, true ),
	    "Connect to non-existing host");

    mRunTcpTest(
	    connection.connectToHost( "localhost", port_, true ),
	    "Connect to echo server");

    BufferString writebuf = "Hello world";
    const int writesize = writebuf.size()+1;
    mRunTcpTest(
	    connection.writeArray( writebuf.buf(), writesize, true ),
	    "writeArray & wait to echo server" );

    char readbuf[1024];

    mRunTcpTest( connection.readArray( readbuf, writesize ),
		  "readArray after write & wait" );

    mRunTcpTest( writebuf==readbuf,
		  "Returned data identical to sent data after write & wait");

    mRunTcpTest(
	    connection.writeArray( writebuf.buf(), writesize, false ),
	    "writeArray & leave to echo server" );

    mRunTcpTest( connection.readArray( readbuf, writesize ),
		 "readArray after write & leave" );

    mRunTcpTest( writebuf==readbuf,
		  "Returned data identical to sent data after write & leave");

    mRunTcpTest(
	    connection.writeArray( writebuf.buf(), writesize, true ) &&
	    !connection.readArray( readbuf, writesize+1 ),
	    "Reading more than available should timeout and fail" );

    BufferString readstring;

    mRunTcpTest(
	    connection.write( writebuf ) &&
	    connection.read( readstring ) &&
	    readstring == writebuf,
	    "Sending and reading a string" );

    //The echo server cannot handle more than 2GB for some reason. Probably
    //because the buffers will overflow as we will not start reading
    //before everything is written.

    int doublearrsz = 200000000; //1.6GB
    ArrPtrMan<double> doublewritearr = new double[doublearrsz];

    Stats::RandGen gen;

    for ( int idx=0; idx<doublearrsz; idx++ )
	doublewritearr[idx] = gen.get();

    mRunTcpTest(
	    connection.writeDoubleArray( doublewritearr, doublearrsz, false ),
	    "Write large array" );

    ArrPtrMan<double> doublereadarr = new double[doublearrsz];
    mRunTcpTest( connection.readDoubleArray( doublereadarr, doublearrsz ),
	    "Read large array" );

    bool readerror = false;
    for ( int idx=0; idx<doublearrsz; idx++ )
    {
	if ( doublewritearr[idx] != doublereadarr[idx] )
	{
	    readerror = true;
	}
    }

    mRunTcpTest( !readerror, "Large array integrity" );

    return true;
}


// Should operate against a server that echos all input back to sender, which
// can be specified by --serverapp "application". If no serverapp is given,
// echoserver is started

int main(int argc, char** argv)
{
    mInitTestProg();
    ApplicationData app;

    BufferString serverapp = "echoserver";

    TestRunner runner;
    runner.serverapp_ = "echoserver";
    runner.serverarg_ = "--timeout 72000 --port 1025";
    runner.port_ = 1025;
    runner.prefix_ = "[ No event loop ]\t";
    runner.exitonfinish_ = false;
    runner.noeventloop_ = true;

    clparser.getVal( "serverapp", runner.serverapp_, true );
    clparser.getVal( "serverarg", runner.serverarg_, true );
    clparser.getVal( "port", runner.port_, true );

    if ( !runner.testTcpConnection() )
	ExitProgram( 1 );

    //Now with a running event loop

    runner.prefix_ = "[ With event loop ]\t";
    runner.exitonfinish_ = true;
    runner.noeventloop_ = false;
    app.addToEventLoop( mCB(&runner,TestRunner,testCallBack) );
    const int res = app.exec();

    ExitProgram( res );
}
