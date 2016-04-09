/*
* @note		：跨平台类型定义及相关头文件
*
/

#ifndef _XTYPES_H_INCLUDE_
#define _XTYPES_H_INCLUDE_

typedef char		s8_t;
typedef short		s16_t;
typedef __int32		s32_t;
typedef __int64		s64_t;
typedef long		long_t;

typedef unsigned char		u8_t;
typedef unsigned short		u16_t;
typedef unsigned __int32	u32_t;
typedef unsigned __int64	u64_t;
typedef unsigned long		ulong_t;

//#pragma warning(push)
#pragma warning( disable:4251 )
#pragma warning( disable:4273 )
#pragma warning( disable:4049 )
#pragma warning( disable:4217 )
#pragma warning( disable:4996 )
//#pragma warning(pop )

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <time.h>
#include <vector>
#include <list>
#include <map>
#include <stack>
#include <queue>
using namespace std;

#ifndef WIN32
#define CloseHandle(h)
#define xsleep(t)			usleep((t)*1000)
#else
#define bzero(dst,size)		memset(dst,0,size)
#define xsleep(t)			Sleep(t)
#define xassert(b)			assert(b)
#endif

#define SAFE_FREE(x)	{ if(x) { ::free(x); (x) = NULL     ; } }
#define SAFE_DEL(x)		{ if(x) { delete (x); (x) = NULL    ; } }
#define SAFE_DEL_HDL(h)	{ if(h) { CloseHandle(h); (h) = NULL; } }
#define SAFE_CLEAR(n)   { if(n) { (n)->clear(); (n) = NULL  ; } }

#endif // _XTYPES_H_INCLUDE_
