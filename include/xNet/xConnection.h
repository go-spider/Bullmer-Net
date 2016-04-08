/*
* @note		：作为 TCP 客户端建立的与服务器连接对象代理基类 xConnection
* @author	：Andy.Ro
* @email	：Qwuloo@qq.com
* @date		：2014-06
* @mark		：对外接口 xConnection，xConnectionFactory 供外部继承
*/
#ifndef _XCONNECTION_H_INCLUDE_
#define _XCONNECTION_H_INCLUDE_

#include "../Interface/xPacket.h"

class xNetObject;	// 连接对象基类
class xNetFactory;	// 代理服务基类

class xConnection;
class xConnectionFactory;

//------------------------------------------------------------------------
// xConnection 服务器连接对象代理基类
//------------------------------------------------------------------------
class XNET_API xConnection : public xNetObject
{
	friend class xConnectionFactory;
public:
	xConnection(xConnectionFactory* factory);

public:
	int made(sock_t * sock, io_t * io);
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
	xConnectionFactory * _factory;
public:
	virtual ~xConnection(void);
};

//------------------------------------------------------------------------
// xConnectionFactory 与服务器连接代理服务类(作为 TCP 客户端)
//------------------------------------------------------------------------
class XNET_API xConnectionFactory : public xNetFactory
{
	typedef std::map<SOCKET,xConnection* >  map_fd_conncection_t;
	typedef map_fd_conncection_t::iterator itor_fd_conncection_t;
public:
	xConnectionFactory(void);
	
	_inline size_t count(void) {
		return this->_list.size();
	}
	_inline xConnection * operator [] (SOCKET fd) {
		return this->get(fd);
	}
	_inline xConnection * get(SOCKET fd)
	{
		//////////////////////////////////////////////////////////////////////////
		// 禁止使用 object = this->_list[fd]，键值对会被隐性添加进map
		//////////////////////////////////////////////////////////////////////////
		this->_list_cs.enter();
		itor_fd_conncection_t itor = this->_list.find(fd);
		xConnection * ptr = (itor != this->_list.end())?itor->second:0;
		this->_list_cs.leave();

		return ptr;
	}
	xConnection * add(xConnection * object);
	xConnection * del(xConnection * object);
public:
	virtual xConnection * create(void);
	virtual void free(xConnection * object);
	virtual void onAdd(xConnection * object);
	virtual void onDel(xConnection * object);
	void notifyCloseAll(void);
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
	map_fd_conncection_t _list;
	ESMT::xSection    _list_cs;
public:
	virtual ~xConnectionFactory(void);
};

#endif // _XCONNECTION_H_INCLUDE_