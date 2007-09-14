/*+
 * COPYRIGHT: (C) dGB Beheer B.V.
 * AUTHOR   : A.H. Bril
 * DATE     : March 1994
 * FUNCTION : general utilities
-*/

static const char* rcsID = "$Id: genc.c,v 1.87 2007-09-14 07:13:24 cvsbert Exp $";

#include "genc.h"
#include "math2.h"
#include "string2.h"
#include "envvars.h"
#include "winutils.h"
#include "timefun.h"
#include "mallocdefs.h"
#include "debugmasks.h"
#include "oddirs.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifndef __win__
# include <unistd.h>
#else
# include <float.h>
#endif


static int insysadmmode_ = 0;
int InSysAdmMode() { return insysadmmode_; }
void SetInSysAdmMode() { insysadmmode_ = 1; }


const char* GetLocalHostName()
{
    static char ret[256];
    gethostname( ret, 256 );
    return ret;
}


void SwapBytes( void* p, int n )
{
    int nl = 0;
    unsigned char* ptr = (unsigned char*)p;
    unsigned char c;

    if ( n < 2 ) return;
    n--;
    while ( nl < n )
    { 
	c = ptr[nl]; ptr[nl] = ptr[n]; ptr[n] = c;
	nl++; n--;
    }
}


void PutIsLittleEndian( unsigned char* ptr )
{
#ifdef __little__
    *ptr = 1;
#else
    *ptr = 0;
#endif
}

#ifdef __msvc__
#define getpid	_getpid
#endif

int GetPID()
{
    return getpid();
}


int ExitProgram( int ret )
{
    if ( od_debug_isOn(DBG_PROGSTART) )
	printf( "\nExitProgram (PID: %d) at %s\n",
		GetPID(), Time_getFullDateString() );

// On Mac OpendTect crashes when calling the usual exit and shows error message:
// dyld: odmain bad address of lazy symbol pointer passed to stub_binding_helper
// _Exit does not call registered exit functions and prevents crash
#ifdef __mac__
    _Exit(0);
    return 0;
#endif

#ifdef __win__

#define isBadHandle(h) ( (h) == NULL || (h) == INVALID_HANDLE_VALUE )

    // open process
    HANDLE hProcess = OpenProcess( PROCESS_TERMINATE, FALSE, GetPID() );
    if ( isBadHandle( hProcess ) )
	printf( "OpenProcess() failed, err = %lu\n", GetLastError() );
    else
    {
	// kill process
	if ( ! TerminateProcess( hProcess, (DWORD) -1 ) )
	    printf( "TerminateProcess() failed, err = %lu\n", GetLastError() );

	// close handle
	CloseHandle( hProcess );
    }
#endif

    exit(ret);
    return ret;
}


/*-> envvar.h */

char* GetOSEnvVar( const char* env )
{
    return getenv( env );
}


#define mMaxNrEnvEntries 1024
typedef struct _GetEnvVarEntry
{
    char	varname[128];
    char	value[1024];
} GetEnvVarEntry;


static void loadEntries( const char* fnm, int* pnrentries,
    			 GetEnvVarEntry* entries[] )
{
    static FILE* fp;
    static char linebuf[1024];
    static char* ptr;
    static const char* varptr;

    fp = fnm && *fnm ? fopen( fnm, "r" ) : 0;
    if ( !fp ) return;

    while ( fgets(linebuf,1024,fp) )
    {
	ptr = linebuf;
	skipLeadingBlanks(ptr);
	varptr = ptr;
	if ( *varptr == '#' || !*varptr ) continue;

	while ( *ptr && !isspace(*ptr) ) ptr++;
	if ( !*ptr ) continue;
	*ptr++ = '\0';
	skipLeadingBlanks(ptr);
	removeTrailingBlanks(ptr);
	if ( !*ptr ) continue;

	entries[*pnrentries] = mMALLOC(1,GetEnvVarEntry);
	strcpy( entries[*pnrentries]->varname, varptr );
	strcpy( entries[*pnrentries]->value, ptr );
	(*pnrentries)++;
    }
    fclose( fp );
}


const char* GetEnvVar( const char* env )
{
    static int filesread = 0;
    static int nrentries = 0;
    static GetEnvVarEntry* entries[mMaxNrEnvEntries];
    int idx;

    if ( !env || !*env ) return 0;
    if ( insysadmmode_ ) return GetOSEnvVar( env );

    if ( !filesread )
    {
	filesread = 1;
	loadEntries( GetSettingsFileName("envvars"), &nrentries, entries );
	loadEntries( GetSetupDataFileName(ODSetupLoc_ApplSetupOnly,"EnvVars"),
		     &nrentries, entries );
	loadEntries( GetSetupDataFileName(ODSetupLoc_SWDirOnly,"EnvVars"),
		     &nrentries, entries );
    }

    for ( idx=0; idx<nrentries; idx++ )
    {
	if ( !strcmp( entries[idx]->varname, env ) )
	    return entries[idx]->value;
    }

    return GetOSEnvVar( env );
}


int GetEnvVarYN( const char* env )
{
    const char* s = GetEnvVar( env );
    return !s || *s == '0' || *s == 'n' || *s == 'N' ? 0 : 1;
}


int GetEnvVarIVal( const char* env, int defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atoi(s) : defltval;
}


double GetEnvVarDVal( const char* env, double defltval )
{
    const char* s = GetEnvVar( env );
    return s ? atof(s) : defltval;
}


int SetEnvVar( const char* env, const char* val )
{
    char* buf;
    if ( !env || !*env ) return NO;
    if ( !val ) val = "";

    buf = mMALLOC( strlen(env)+strlen(val) + 2, char );
    strcpy( buf, env );
    if ( *val ) strcat( buf, "=" );
    strcat( buf, val );

    putenv( buf );
    return YES;
}


/*-> math2.h */

#ifdef sun5
# include <ieeefp.h>
#endif

int IsNormalNumber( double x )
{
#ifdef __msvc__
    return _finite( x );
#else
    return finite( x );
#endif
}


double IntPowerOf( double x, int y )
{
    if ( mcIsUndefined(x) ) return mcUndefValue;

    double ret = 1;
    if ( x == 0 )
	return y ? 0 : 1;

    if ( x > 1.5 || x < -1.5 )
    {
	if ( y > 150 ) return mcUndefValue;
	if ( y < -150 ) return 0;
	if ( x > 1.99 || x < -1.99 )
	{
	    if ( y > 100 ) return mcUndefValue;
	    if ( y < -100 ) return 0;
	}
    }
    else if ( x < 0.5 && x > -0.5 )
    {
	if ( y > 100 ) return 0;
	if ( y < -100 ) return 1;
    }

    while ( y )
    {
	if ( y > 0 )
	    { ret *= x; y--; }
	else
	    { ret /= x; y++; }
    }
    return ret;
}


double PowerOf( double x, double y )
{
    int isneg = x < 0 ? 1 : 0;
    double ret;
 
    if ( x == 0 ) return y ? 0 : 1;
    if ( isneg ) x = -x;
 
    ret = exp( y * log(x) );
    return isneg ? -ret : ret;
}


double ACos( double c )
{
    if ( c>=1 ) return 0;
    if ( c<=-1 ) return M_PI;
    return acos( c );
}


double ASin( double s )
{
    if ( s>=1 ) return M_PI_2;
    if ( s<=-1 ) return -M_PI_2;
    return asin( s );
}
