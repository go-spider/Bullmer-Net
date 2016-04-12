/*
* @note		：跨平台socket宏定义及相关函数实现
*/

#ifndef _XSOCKET_H_INCLUDE_
#define _XSOCKET_H_INCLUDE_

#ifndef WIN32
	#include <unistd.h>
//	#include <sys/types.h>
	#include <sys/socket.h>
	#include <sys/ioctl.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>
	#include <fcntl.h>
	#include <errno.h>
	#include <sys/epoll.h>
	
	typedef u32_t				SOCKET;
	#define INVALID_SOCKET		-1
	#define SOCKET_ERROR		-1
//	#define GetLastError()		errno
	#define closesocket			close
#else
	#include <Winsock2.h>
	#include <MSWSock.h>
	#include <Ws2tcpip.h>
//	#include <conio.h>
//	#include <windows.h>
//	#include <process.h>
	#pragma comment(lib, "ws2_32.lib")
#ifdef errno
#undef errno
#endif
	#define errno				WSAGetLastError()
#endif

#define SAFE_DEL_FD(fd)			{ if(fd) { ::closesocket(fd); (fd) = INVALID_SOCKET; } }

#include "../xNet/xNetDll.h"

#ifdef __cplusplus
extern "C" {
#endif

XNET_API void init_socket_lib();
XNET_API void clear_socket_lib();
XNET_API SOCKET init_listen(ulong_t ipaddr, u16_t port);

XNET_API void setnonblocking(SOCKET fd);

XNET_API void ipaddr2str(ulong_t ipaddr, s8_t* str, u32_t len);
XNET_API ulong_t str2ipaddr(s8_t const* ipaddr);

XNET_API u8_t localhnameipaddr(s8_t * hname, s8_t * ipaddr, u32_t size);

#ifdef __cplusplus
}
#endif

#endif // _XSOCKET_H_INCLUDE_
