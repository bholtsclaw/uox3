#ifndef __StdHeaders_H__
#define __StdHeaders_H__

#ifdef __BORLANDC__
    #define __STD_ALGORITHM
#endif

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <cstring>
#include <cstdarg>
#include <cmath>

// STL containers
#include <vector>
#include <map>
#include <string>
#include <set>
#include <list>
#include <deque>
#include <queue>

// Note - not in the original STL, but exists in SGI STL and STLport
//#ifdef EXT_HASH
//#   include <ext/hash_map>
//#   include <ext/hash_set>
//#else
//#   include <hash_set>
//#   include <hash_map>
//#endif

// STL algorithms & functions
#include <algorithm>
#include <functional>
#include <limits>

// C++ Stream stuff
#include <fstream>
#include <iostream>
#include <iomanip>
#include <sstream>

#ifdef __BORLANDC__
namespace UOX
{
    using namespace std;
}
#endif

extern "C" {

#   include <sys/types.h>
#   include <sys/stat.h>

}

#if UOX_PLATFORM == PLATFORM_WIN32

#undef min
#undef max

	#define WIN32_LEAN_AND_MEAN
	#include <sys/timeb.h>
	typedef long int32;

#endif

#if UOX_PLATFORM == PLATFORM_LINUX
extern "C" {

#   include <unistd.h>
#   include <dlfcn.h>
#	include <ctype.h>
#	include <sys/time.h>
#	include <sys/signal.h>
#	include <unistd.h>
#	include <sys/stat.h>	// for mkdir
#	include <errno.h>		// for errno

}
#endif

#if UOX_PLATFORM == PLATFORM_APPLE
extern "C" {
#   include <unistd.h>
#   include <sys/param.h>
#   include <CoreFoundation/CoreFoundation.h>
}
#endif

#include "socket_interface.h"
#include "jsapi.h"

#endif
