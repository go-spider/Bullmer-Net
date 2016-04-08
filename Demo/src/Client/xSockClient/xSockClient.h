#ifndef _XSOCKCLIENT_H_INCLUDE_
#define _xSOCKCLIENT_H_INCLUDE_

#include "../xNet/xNet.h"
#include "../xBuffer/xBuffer.h"
#include "../xBuffer/xbufferEx.h"
#include "../xBuffer/xBuffer_p.h"

#include "../xSocket/xSocket.h"
#include "../Interface/xPacket.h"

#include "../xThread/xThread.h"
#include "../xNet/xFactory.h"

#include "../xSys/xSysMsg.h"

#ifdef MAX_THREAD
#undef MAX_THREAD
#endif
#define MAX_THREAD		8
class xNetObject;
class ESMT::xThread;

class xClientSocketPool;

#ifndef WSA_MAXIMUM_WAIT_EVENTS
#define WSA_MAXIMUM_WAIT_EVENTS 64
#endif

#define MAX_EVENT WSA_MAXIMUM_WAIT_EVENTS

//------------------------------------------------------------------------
// xSockClient
//------------------------------------------------------------------------
class xSockClient : public xNetObject
{
public:
	xSockClient(xClientSocketPool * pool);
	int initsocket(void);
	void release(void);
public:
	int connect(ulong_t ipaddr, u16_t port);
	int send(char const* buf, u64_t bytes);
	int close(void);

	// 用户注册
	int regst(char const * username, char const * passwd, char const * nickname, u8_t sex);
	int login(char const * username, char const * passwd, u8_t state, u32_t sID, u32_t pID, char const * cIp, double lng, double lat);
	
	int random_regst_test(void);
public:
	xClientSocketPool * _sock_pool;
	ESMT::xEvent _avail_event;
public:
	~xSockClient(void);
};

//------------------------------------------------------------------------
// xClientSocketPool
//------------------------------------------------------------------------
class xClientSocketPool
{
	typedef std::map<u32_t,xSockClient*> map_fd_object_t;
	typedef map_fd_object_t::iterator   itor_fd_object_t;
	friend class xSockClient;
private:
	class xReader: public ESMT::xThread
	{
	public:
		xReader(void * argument);
		u32_t __fastcall run(void * argument);
	};
public:
	xClientSocketPool(void);

	static xClientSocketPool * shareObject();

	int connect(ulong_t ipaddr, u16_t port, xSockClient*& object);

	virtual xSockClient * alloc(void);
	
	virtual void free(xSockClient * object);

	void clear(void);

	virtual int onLost(xSockClient * object, u32_t err);
	virtual int onRecv(xSockClient * object, char const* buf, u64_t bytes);

private:
	int _attach_net_event(xSockClient * object);
public:
	map_fd_object_t _object_list;
	WSAEVENT _net_event[MAX_EVENT];
	ESMT::xMutex _event_list_cs;
//	ESMT::xSection _event_list_cs;
	ESMT::xThreadPool _readers; // 读线程池

public:
	~xClientSocketPool(void);
};

#endif // _xSOCKCLIENT_H_INCLUDE_