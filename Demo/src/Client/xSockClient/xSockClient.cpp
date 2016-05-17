#include "xSockClient.h"
#include "../xUtils/xUtils.h"

using namespace ESMT;

// @note：tcp 发送(单数据包)
static inline int tcp_udp_send(SOCKET fd, char const* buf, ulong_t bytes)
{
	u32_t offset = 0;
	u32_t nbytes = 0;
	while ( offset < bytes )
	{
		nbytes = ::send(fd, buf + offset, bytes - offset, 0);
		if ( SOCKET_ERROR == nbytes )
		{
			fprintf(stderr, "send() error: %d\n", errno);
			return SOCKET_ERROR;
		}
		offset += nbytes;
	}
	return NO_ERROR;
}

// @note：tcp 接收
static inline int tcp_udp_recv(SOCKET fd, char * buf, ulong_t bytes)
{
	return ::recv(fd, buf, bytes, 0);
}

// @note：tcp 接收指定长度的数据
static inline int tcp_udp_recv_len(SOCKET fd, char * buf, ulong_t bytes)
{
	u32_t offset = 0;
	u32_t nbytes = 0;

	while ( offset < bytes )
	{
		// 缺点就是若未达到指定接收长度就会被挂起无法返回
		nbytes = ::recv(fd, buf + offset, bytes - offset, 0);
		if ( SOCKET_ERROR == nbytes )
		{
			fprintf(stderr, "recv() error: %d\n", errno);
			return SOCKET_ERROR;
		}
		if ( 0 == nbytes )
		{
			printf("peer closed connection.\n");
			return INVALID_SOCKET;
		}
		offset += nbytes;
	}
	return NO_ERROR;
}

// @note：通知接收整体数据回调
static inline int recv_callback_func_this(xNetObject* pthis, char const* buf, u64_t bytes)
{
	xSockClient * object = dynamic_cast<xSockClient*>(pthis);
	return object->_sock_pool->onRecv(object, buf, bytes);
}

// @note：通知单数据包发送回调
static inline int send_callback_func_this(xNetObject* pthis, char const* buf, ulong_t bytes)
{
	int rc = tcp_udp_send(pthis->_object.fd, buf, bytes);

	if ( SOCKET_ERROR == rc )
	{
#if 0
		// 发送出错，套接字不可用
		__nonsignaled( dynamic_cast<xSockClient*>(pthis)->_avail_event );
#endif
//		pthis->closesocket();
// 		if ( WSAECONNRESET == errno )
// 		{
// 			dynamic_cast<xSockClient*>(pthis)->_sock_pool->onLost(dynamic_cast<xSockClient*>(pthis), errno);
// 		}
	}
	
	return rc;
}

//------------------------------------------------------------------------
// xSockClient
//------------------------------------------------------------------------

xSockClient::xSockClient(xClientSocketPool * pool):xNetObject(),_sock_pool(pool),_avail_event(FALSE, TRUE)
{
	init_socket_lib();
}

int xSockClient::connect(ulong_t ipaddr, u16_t port)
{
	// 目标主机ip/port
	sockaddr_in srvaddr={0};
	srvaddr.sin_family		= AF_INET;
	srvaddr.sin_addr.s_addr = ipaddr;
	srvaddr.sin_port		= ::htons(port);

	// 连接目标主机
	if( SOCKET_ERROR == ::connect(this->_object.fd,(sockaddr *)&srvaddr, sizeof(sockaddr_in)) )
	{
		
		fprintf(stderr,"xSockClient::connect() failed,code: %d\n",errno);
		this->closesocket();
		return SOCKET_ERROR;
	}

	this->_object.r_ipaddr = srvaddr.sin_addr.s_addr;
	this->_object.port	   = port;
	this->_object.af	   = srvaddr.sin_family;

#if 0
	// 连接成功，套接字可用
	__signaled(this->_avail_event);
#endif
	// 连接成功，关联网络事件(先建立连接再关联，否则对于非阻塞套接字连接会立即返回 10035 错误)
	this->_sock_pool->_attach_net_event(this);

	BOOL enabled = TRUE;
#ifdef _DEBUG
	assert (SOCKET_ERROR != setsockopt(this->_object.fd,SOL_SOCKET,SO_REUSEADDR,(char const*)&enabled,sizeof(BOOL)) );
	assert (SOCKET_ERROR != setsockopt(this->_object.fd,SOL_SOCKET,SO_LINGER,(char const*)&enabled,sizeof(BOOL)) );
	assert (SOCKET_ERROR != setsockopt(this->_object.fd,IPPROTO_TCP,TCP_NODELAY,(char const*)&enabled,sizeof(BOOL)) );
#else
	setsockopt(this->_object.fd,SOL_SOCKET,SO_REUSEADDR,(char const*)&enabled,sizeof(BOOL));
	setsockopt(this->_object.fd,SOL_SOCKET,SO_LINGER,(char const*)&enabled,sizeof(BOOL));
	setsockopt(this->_object.fd,IPPROTO_TCP,TCP_NODELAY,(char const*)&enabled,sizeof(BOOL));
#endif

	// set non-blocking
	setnonblocking(this->_object.fd);
	return NO_ERROR;
}

int xSockClient::send(char const* buf, u64_t bytes)
{
	int rc = 0;

	this->lock(); // 并发安全锁，线性写入
	rc = split_packet(buf, bytes, send_callback_func_this, this);
	this->unlock();

	return rc;
}

int xSockClient::close(void)
{
	// 关闭连接套接字
	SAFE_DEL_FD(this->_object.fd);
	return 0;
}


// 用户注册
int xSockClient::regst(char const * username, char const * passwd, char const * nickname, u8_t sex)
{
	C2S_REGISTER_Request msg;
	memcpy_s(msg.username,strlen(username),username,strlen(username));
	memcpy_s(msg.nickname,strlen(nickname),nickname,strlen(nickname));
	memcpy_s(msg.passwd,strlen(passwd),passwd,strlen(passwd));
	msg.sex = sex;
	return this->send((char const *)&msg, sizeof(msg));
}

int xSockClient::random_regst_test(void)
{
	char username[256]={0},nickname[256]={0};
	x_random_string("0123456789", 11, username, sizeof(nickname));
	x_random_string("AaBbCcDdEeFfGgHhIiJjKkLlMmNnOoPpQqRrSsTtUuVvWwXxYyZz", 5, nickname, sizeof(nickname));
	return this->regst(username, "123456", nickname, 1);
}

int xSockClient::login(char const * username,char const * passwd, u8_t state, u32_t sID, u32_t pID, char const * cIp, double lng, double lat)
{
	C2S_LOGIN_Request msg;
	memcpy_s(msg.username,strlen(username),username,strlen(username));
	memcpy_s(msg.passwd,strlen(passwd),passwd,strlen(passwd));
	memcpy_s(msg.ipaddr,strlen(cIp),cIp,strlen(cIp));
	msg.state    = state;
	msg.sID      = sID;
	msg.pID      = pID;
	msg.geo[LNG] = lng;
	msg.geo[LAT] = lat;
	return this->send((char const *)&msg, sizeof(msg));
}

xSockClient::~xSockClient(void)
{
	
} // class xSockClient

//////////////////////////////////////////////////////////////////////////
// @note：线程调用阻塞接收
//////////////////////////////////////////////////////////////////////////
static inline int global_thread_recv(xSockClient * pthis)
{
	char buf[MAX_BUF_SIZE]={0}; bzero(buf, sizeof(buf));

#if 0
	// 等待套接字可用
	__wait_signal(dynamic_cast<xSockClient*>(pthis)->_avail_event, INFINITE);
#endif
	
	ulong_t bytes = tcp_udp_recv(pthis->_object.fd, buf, sizeof(buf));
	
	if( SOCKET_ERROR == bytes )
	{
		fprintf(stderr, "recv() error: %d\n", errno);
#if 0
		// 接收出错，套接字不可用
		__nonsignaled( dynamic_cast<xSockClient*>(pthis)->_avail_event );
		pthis->closesocket();
#endif
// 		if ( WSAECONNRESET == errno )
// 		{
// 			pthis->_sock_pool->onLost(pthis, errno);
// 		}
		return SOCKET_ERROR;
	}
	if ( 0 == bytes )
	{
		printf("peer closed connection.\n");
#if 0
		// 连接关闭，套接字不可用
		__nonsignaled( dynamic_cast<xSockClient*>(pthis)->_avail_event );
		pthis->closesocket();
#endif
//		pthis->_sock_pool->onLost(pthis, 0);

		return INVALID_SOCKET;
	}

	separate_continuous_packet(buf, bytes, recv_callback_func_this, pthis);
	
	return NO_ERROR;
}

xClientSocketPool::xReader::xReader(void * argument):ESMT::xThread(argument)
{
}

u32_t xClientSocketPool::xReader::run(void * argument)
{
	xClientSocketPool * pthis = (xClientSocketPool *)argument;
	ulong_t index = 0;

	while ( !this->done() )
	{
		pthis->_event_list_cs.enter();
		ulong_t count = pthis->_object_list.size();
		index = ::WSAWaitForMultipleEvents(count, pthis->_net_event, FALSE, WSA_INFINITE, FALSE);
		pthis->_event_list_cs.leave();
		
		if ( WSA_WAIT_FAILED == index )
		{
			if( count > 0 )
			{
// 				assert( WSAENETDOWN           != errno );
// 				assert( WSA_NOT_ENOUGH_MEMORY != errno );
// 				assert( WSA_INVALID_HANDLE    != errno );
// 				assert( WSA_INVALID_PARAMETER != errno );
			}
		}
		else if ( WSA_WAIT_TIMEOUT == index )
		{
			continue;
		}
		else
		{
			index = index - WSA_WAIT_EVENT_0;
			WSAResetEvent(pthis->_net_event[index]); // 手动重置未受信状态
			xSockClient * object = pthis->_object_list[index];//printf("[xSocketClient]：%d count：%d\n",index,count);

			if ( object && INVALID_SOCKET != object->_object.fd )
			{
				WSANETWORKEVENTS ne = {0};
#ifndef _DEBUG
				WSAEnumNetworkEvents(object->_object.fd, pthis->_net_event[index], &ne);
#else
				assert( SOCKET_ERROR != WSAEnumNetworkEvents(object->_object.fd, pthis->_net_event[index], &ne) );
#endif
				// 有可读数据
				if ( FD_READ == ( FD_READ & ne.lNetworkEvents ) )
				{
					object->lock(); // 并发安全锁，线性读取
					global_thread_recv(object);
					object->unlock();
				}
				// 套接字关闭
				else if ( FD_CLOSE == ( FD_CLOSE & ne.lNetworkEvents ) )
				{
//					assert( ERROR_SUCCESS == ne.iErrorCode[FD_CLOSE_BIT] );
					
					// 主线程关闭连接套接字触发该事件
					pthis->_event_list_cs.enter();
					
					// 关闭接收事件
					WSACloseEvent(pthis->_net_event[index]);
					pthis->_net_event[index] = NULL;
					
					// 通知连接关闭
					pthis->onLost(object, errno);
					
					// 删除连接对象
					SAFE_DEL_FD(object->_object.fd);
					pthis->_object_list.erase(index);
					pthis->free(object);

					if ( count > 1 && (index < (count - 1)) )
					{
						// 移动事件
						pthis->_net_event[ index] = pthis->_net_event[count-1];
						// 移动对象
						pthis->_object_list[index] = pthis->_object_list[count-1];
						pthis->_object_list.erase(count-1);
					}
					pthis->_event_list_cs.leave();
				}
			}
		}
	}
	printf("工作线程退出....\n");
	return 0;
}

xClientSocketPool::xClientSocketPool(void)
{
	init_socket_lib();

	bzero(this->_net_event, sizeof(WSAEVENT)*WSA_MAXIMUM_WAIT_EVENTS);
	
	// 初始化接收线程
	SYSTEM_INFO si;
	::GetSystemInfo(&si);
	for (u32_t i=0; i<5*si.dwNumberOfProcessors&&i<MAX_THREAD; ++i)
	{
		this->_readers.add(new xClientSocketPool::xReader(this));
	}
}

int xClientSocketPool::connect(ulong_t ipaddr, u16_t port, xSockClient*& object)
{
	int rc = NO_ERROR;
	object = this->alloc();
	if( SOCKET_ERROR == (rc = object->connect(ipaddr, port)) )
	{
		this->free(object);
		object = NULL;
	}
	return rc;
}

xSockClient * xClientSocketPool::alloc(void)
{
	xSockClient * object = new xSockClient(this);

	// 初始化套接字
#if 1
	if ( INVALID_SOCKET == (object->_object.fd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) )
#else
	if ( INVALID_SOCKET == (object->_object.fd = WSASocket(AF_INET,SOCK_STREAM,IPPROTO_TCP,NULL,0,WSA_FLAG_OVERLAPPED)) )
#endif
	{
		fprintf(stderr, "socket() Failed, Error Code: %d\n", errno);
		return NULL;
	}
	return object;
}

int xClientSocketPool::_attach_net_event(xSockClient * object)
{
	int rc = NO_ERROR;
	
	int id = this->_object_list.size();
	
	this->_event_list_cs.enter();
	
	this->_object_list[id] = object;
	
	WSAEVENT e = this->_net_event[id] = WSACreateEvent(); 

#ifndef _DEBUG
	rc = WSAEventSelect(object->_object.fd, e, FD_READ | FD_CLOSE);
#else
	assert( SOCKET_ERROR != WSAEventSelect(object->_object.fd, e, FD_READ | FD_CLOSE) );
#endif
	
	this->_event_list_cs.leave();
	
	return SOCKET_ERROR == rc ? SOCKET_ERROR:NO_ERROR;
}

void xClientSocketPool::free(xSockClient * object)
{
	SAFE_DEL(object);
}

int xClientSocketPool::onLost(xSockClient * object, u32_t err)
{
	printf("连接关闭 : %d\n",err);
	return 0;
}

int xClientSocketPool::onRecv(xSockClient * object, char const* buf, u64_t bytes)
{
	MSG_Root * pmsg = (MSG_Root *)buf;
	switch ( pmsg->id() )
	{
	case ID_S2C_REGISTER_Response:
		{
			S2C_REGISTER_Response * rsp = (S2C_REGISTER_Response *)pmsg;
			printf("{ code: %d, errmsg: \"%s\" }\n",rsp->code,rsp->errmsg);

			char clng[10];
			std::string lng = "104.67";
			x_random_string("1234567890", 6, clng, 10);
			lng += clng;

			char clat[10];
			std::string lat = "39.57";
			x_random_string("1234567890", 6, clat, 10);
			lat += clat;
			return object->login(rsp->username, rsp->passwd, 1, 1, 1, "127.0.0.1", atof(lng.c_str()), atof(lat.c_str()));

			break;
		}
	case ID_S2C_LOGIN_Response:
		{
			S2C_LOGIN_Response * rsp = (S2C_LOGIN_Response *)pmsg;
			printf("{ code: %d, errmsg: \"%s\",uid: %d, sessid: %d }\n",rsp->code,rsp->errmsg,rsp->userid,rsp->sessionid);
			break;
		}
	default:
		{
			printf("[xSockClient]：%s\n",buf);
//			assert( FALSE );
			break;
		}
	}

	return 0;
}

void xClientSocketPool::clear(void)
{
	itor_fd_object_t itor = this->_object_list.begin();
	for ( ; itor != this->_object_list.end(); )
	{
		// 关闭连接套接字
		itor->second->close();
	}
}

static xClientSocketPool * g_clientsocket_pool = NULL;

xClientSocketPool * xClientSocketPool::shareObject()
{
	if ( !g_clientsocket_pool )
	{
		g_clientsocket_pool = new xClientSocketPool();
	}
	return g_clientsocket_pool;
}

xClientSocketPool::~xClientSocketPool(void)
{
	clear_socket_lib();
} // class xClientSocketPool
