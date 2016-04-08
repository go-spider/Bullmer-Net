/*
* @note		：跨平台网络通信框架相关头文件
* @author	：Andy.Ro
* @email	：Qwuloo@qq.com
* @date		：2014-06
*/
#ifndef _XMACRO_USER_H_INCLUDE_
#define _XMACRO_USER_H_INCLUDE_

#include "../xNet/xNetDll.h"

#include "xNet.h"

#include "../xBuffer/xBuffer.h"
#include "../xBuffer/xBuffer_p.h"
#include "../xBuffer/xBufferEx.h"

#include "xFactory.h"
#include "xClient.h"
#include "xConnection.h"

/*
* win32平台采用 iocp
* linux平台采用 epoll(2.6及以上内核)
*/
#ifdef WIN32
#include "xIocp/xIocp.h"
#include "xIocp/xIocpClient.h"
#else
#include "xEpoll/xEpoll.h"
#endif

#include "xReactor.h"

#endif // _XMACRO_USER_H_INCLUDE_