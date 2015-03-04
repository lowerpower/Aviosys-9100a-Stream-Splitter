#ifndef __CONFIG_H__
#define __CONFIG_H__
/*! \file config.h
*/

#include "mytypes.h"
#include <sys/types.h>
#include <stdio.h>
#include <malloc.h>


// pack all structures to byte level, this should work for all compilers
#pragma pack(1)

// For windows include the following
#ifdef WIN32
//#ifndef  WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <stdlib.h>
#include <io.h>
#include <time.h>
#include <signal.h>
#include <sys/timeb.h>
#include <Rpc.h>

#define _CRTDBG_MAP_ALLOC

#include <crtdbg.h>

// Unix some winsock errors
#define EWOULDBLOCK		WSAEWOULDBLOCK
#define EINPROGRESS		WSAEINPROGRESS
#define EALREADY		WSAEALREADY
#define EINVAL			WSAEINVAL
#define EISCONN			WSAEISCONN
#define ENOTCONN		WSAENOTCONN

#define MSG_WAITALL     0x8

//#endif
#else
// For Linux include the following
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h> 
#include <arpa/inet.h>
#include <sys/select.h>
#include <fcntl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <net/if.h>
#include <errno.h>
#include <sys/timeb.h>
#include <sched.h>
//#include <ifaddrs.h>

#define SOCKET_ERROR		-1
#define INVALID_SOCKET		-1
#define closesocket			close

typedef int	SOCKET;


#endif



#endif


