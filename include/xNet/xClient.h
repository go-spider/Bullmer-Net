/*
* @note		：作为 TCP 服务器建立的与客户端连接对象代理基类 xClient
* @author	：Andy.Ro
* @email	：Qwuloo@qq.com
* @date		：2014-06
* @mark		：对外接口 xClient，xClientFactory 供外部继承
*/
#ifndef _XCLIENT_H_INCLUDE_
#define _XCLIENT_H_INCLUDE_

#include "../Interface/xPacket.h"

class xNetObject;			// 连接对象基类
class xNetFactory;			// 代理服务基类

class xClient;
class xClientFactory;

//typedef int (xClient::*send_packet_callback_this)(char const* buf, ulong_t len);

//------------------------------------------------------------------------
// xClient 客户端连接对象代理基类
//------------------------------------------------------------------------
class XNET_API xClient : public xNetObject
{
	friend class xClientFactory;
public:
	xClient(xClientFactory* factory);
public:
	int made(listen_t * sock, io_t * io);
	int lost(u32_t err);
	int recv(char const* buf, ulong_t bytes);
	int send(char const* buf, u64_t bytes);
public:
	virtual int onMade(void);
	virtual int onLost(u32_t err);
	virtual int onRecv(char const* buf, u64_t len);

	inline void setClose(void) { this->_need_close_flag = TRUE; }
	inline BOOL getClose(void) { return this->_need_close_flag; }
	

	void clear(void);
protected:
	BOOL _need_close_flag;
	xClientFactory * _factory;
	
public:
	virtual ~xClient(void);
};

//------------------------------------------------------------------------
// xClientFactory 与客户端连接代理服务类(作为 TCP 服务器)
//------------------------------------------------------------------------
class XNET_API xClientFactory : public xNetFactory
{
	typedef std::map<SOCKET,xClient*>  map_fd_client_t;
	typedef map_fd_client_t::iterator itor_fd_client_t;
public:
	xClientFactory(void);

	_inline size_t count(void) {
		return this->_list.size();
	}
	_inline xClient * operator [] (SOCKET fd) {
		return this->get(fd);
	}
	_inline xClient * get(SOCKET fd)
	{
		//////////////////////////////////////////////////////////////////////////
		// 禁止使用 object = this->_list[fd]，键值对会被隐性添加进map
		//////////////////////////////////////////////////////////////////////////
		this->_list_cs.enter();
		itor_fd_client_t itor = this->_list.find(fd);
		xClient * ptr = (itor != this->_list.end())?itor->second:0;
		this->_list_cs.leave();
		
		return ptr;
	}
	xClient * add(xClient * object);
	xClient * del(xClient * object);
	void notifyCloseAll(void);
public:
	virtual xClient * create(void);
	virtual void free(xClient * object);
	virtual void onAdd(xClient * object);
	virtual void onDel(xClient * object);
public:
#if 0
	//------------------------------------------------------------------------
	// xDispatcher
	//------------------------------------------------------------------------
	class xDispatcher : public ESMT::xThread, public xNetFactory::xDispatcher
	{
	public:
		xDispatcher(void * argument);
		void __fastcall dispatch(xNetObject * object, e_event_t e);
		u32_t __fastcall run(void * argument);
	};
	//------------------------------------------------------------------------
	// xConsumer
	//------------------------------------------------------------------------
	class xConsumer : public ESMT::xThread
	{
	public:
		xConsumer(void * argument);
		u32_t __fastcall run(void * argument);
	};
	xNetFactory::xDispatcher * __fastcall dispatcher(void);
#endif
	map_fd_client_t   _list;
	ESMT::xSection _list_cs;
public:
	virtual ~xClientFactory(void);
};

#endif // _XCLIENT_H_INCLUDE_