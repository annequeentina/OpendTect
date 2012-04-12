/*+
________________________________________________________________________

 (C) dGB Beheer B.V.; (LICENSE) http://opendtect.org/OpendTect_license.txt
 Author:        K. Tingdahl
 Date:          Sep 2011
________________________________________________________________________

-*/
static const char* rcsID = "$Id: odmemory.cc,v 1.4 2012-04-12 14:04:38 cvsbert Exp $";

#include "odsysmem.h"
#include "odmemory.h"

#ifdef lux
#include "strmoper.h" 
#include <fstream>
#endif
#ifdef mac
#include <unistd.h>
#include <mach/mach_init.h>
#include <mach/mach_host.h>
#include <mach/host_info.h>
#endif

#include "iopar.h" 
#include "string2.h" 

bool OD::haveMemInfo()
{
#ifdef lux
    return true;
#endif
#ifdef mac
    return true;
#endif
#ifdef win
    return false;
#endif
}

void OD::dumpMemInfo( IOPar& res )
{
    float total, free;
    getSystemMemory( total, free );
    total /= 1024; free /= 1024;
    int itot = mNINT(total); int ifree = mNINT(free);
    res.set( "Total memory (kB)", itot );
    res.set( "Free memory (kB)", ifree );
}


#ifdef lux
static float getMemFromStr( char* str, const char* ky )
{
    char* ptr = strstr( str, ky );
    if ( !ptr ) return 0;
    ptr += strlen( ky );
    mSkipBlanks(ptr);
    char* endptr = ptr; mSkipNonBlanks(endptr);
    *endptr = '\0';
    float ret = toFloat( ptr );
    *endptr = '\n';
    return ret;
}
#endif


#define mErrRet { total = free = 0; return; }

void OD::getSystemMemory( float& total, float& free )
{
#ifdef lux

    std::ifstream strm( "/proc/meminfo" );
    BufferString filecont;
    if ( !StrmOper::readFile(strm,filecont) )
	mErrRet

    total = getMemFromStr( filecont.buf(), "MemTotal:" );
    free = getMemFromStr( filecont.buf(), "MemFree:" );

#endif
#ifdef mac
    vm_statistics_data_t vm_info;
    mach_msg_type_number_t info_count;

    info_count = HOST_VM_INFO_COUNT;
    if ( host_statistics(mach_host_self(),HOST_VM_INFO,
		(host_info_t)&vm_info,&info_count) )
	mErrRet

    total = (vm_info.active_count + vm_info.inactive_count +
	    vm_info.free_count + vm_info.wire_count) * vm_page_size;
    free = vm_info.free_count * vm_page_size;

#endif
}
