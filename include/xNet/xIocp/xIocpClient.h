/*
* @note		：iocp服务器
* @mark		：三个类：xNetFactory，xClient，xConnection 供外部继承(位于/xNet/目录下)
*
*/

#ifndef _XIOCPCLIENT_H_INCLUDE_
#define _XIOCPCLIENT_H_INCLUDE_

#ifdef MAX_THREAD
#undef MAX_THREAD
#endif
#define MAX_THREAD		2

class ESMT::xThread;
class ESMT::xThreadPool;
class ESMT::xEvent;

class xConnectionFactory;

//------------------------------------------------------------------------
// iocp客户端(单例模式)
//------------------------------------------------------------------------
class XNET_API xIocpClient
{
	typedef xBuffer<io_t> io_pool_t;
	typedef std::map<u32_t ,listen_t*> map_fd_listen_t;
//	typedef std::map<SOCKET,listen_t*> map_fd_listen_t;
	typedef map_fd_listen_t::iterator itor_fd_listen_t;
	typedef std::map<SOCKET,node_t<io_t>*> map_fd_io_t;
	typedef map_fd_io_t::iterator         itor_fd_io_t;
	typedef std::map<u32_t,ESMT::xEvent*> map_idx_event_t;
	typedef map_idx_event_t::iterator    itor_idx_event_t;
public:
	// 工作线程
	class XNET_API xWorker: public ESMT::xThread
	{
	public:
		xWorker(void * argument);
		u32_t __fastcall run(void * argument);
	};
public:
	static xIocpClient * shareObject(xConnectionFactory* factory);
private:
	// @note: 指定代理服务对象
	// @param service <xConnectionFactory*> : 代理服务对象
	//
	// @example:
	//	UserServer factory;
	//	xIocpClient iocp(&factory);
	//
	xIocpClient(xConnectionFactory* factory);
	void made(sock_t * sock, node_t<io_t>* io);
	void lost(node_t<io_t>* io, u32_t err);
	void recv(node_t<io_t>* io);
public:
	// @note: 作为 TCP 客户端连接远程服务器
	// @param ipaddr <ulong_t> : 目标主机ip
	// @param port	 <u16_t>   : 连接起始端口
	// @param scope	 <u32_t>   : 连接端口范围
	void connectTCP(ulong_t ipaddr, u16_t port = 6000, u32_t scope = 1);
	void stop(void) { this->_shutdown = TRUE; }
	
	// @note: connect/recv/send io buf 投递
	int post_connect_buf(sock_t * sock);
	int post_recv_buf   (SOCKET fd);
	int post_send_buf   (SOCKET fd, char const* buf, ulong_t len);
	int post_close_buf  (SOCKET fd);
	// @note: connect/recv/send io buf 释放
	void free_connect_buf(node_t<io_t>* io);
	void free_recv_buf  (node_t<io_t>* io);
	void free_send_buf	(node_t<io_t>* io);

protected:
	// @note: 退出所有工作线程
	void _end_workers(void);

	// @note: 连接指定端口范围
	// @param port	<u16_t> : 起始端口
	// @param scope	<u32_t> : 监听范围
	int _connect(ulong_t ipaddr, u16_t port = 6000, u32_t scope = 1);
	
	// @note: io操作
	// @param sock	<listen_t*>     : 连接套接字(connectfd)对象
	// @param io	<node_t<io_t>*>	: io对象
	// @param bytes	<u32_t>	        : 传输字节数
	// @param err	<u32_t>	        : 错误代码
	int _handle_io(listen_t* sock, node_t<io_t>* io, ulong_t bytes, u32_t err);

	// @note: 投递ConnectEx
	// @param sock	<listen_t*>     : 连接套接字(connectfd)
	// @param io	<node_t<io_t>*>	: io_t新对象
	int _post_connect(SOCKET fd, node_t<io_t>* io);
	
	// @note: 投递WSARecv
	// @param fd	<SOCKET>        : 连接套接字(connectfd)
	// @param io	<node_t<io_t>*>	: io_t新对象
	int _post_recv   (SOCKET fd, node_t<io_t>* io);
	
	// @note: 投递WSASend
	// @param fd	<SOCKET>        : 连接套接字(connectfd)
	// @param io	<node_t<io_t>*>	: io_t新对象
	int _post_send   (SOCKET fd, node_t<io_t>* io);

	// @note: 监听指定ip地址和端口
	// @param ipaddr <ulong_t>: 网络字节序ipaddr
	// @param port   <u16_t>  : 主机字节序port
	int _connect_one(ulong_t ipaddr, u16_t port);
public:
	BOOL _shutdown;
	HANDLE _hComPort;					// 完成端口
	u16_t _port;						// 起始端口
	u32_t _scope_port;					// 端口范围

	ESMT::xThreadPool _workers;			// 工作者线程池
	xConnectionFactory * _factory;		// 代理服务对象
	ESMT::xMutex         _factory_cs;
protected:
	BOOL _init_LPFN_PTR(void);
	LPFN_CONNECTEX _lpfnConnectEx;
	LPFN_TRANSMITFILE _lpfnTransmitFile;

public:
	~xIocpClient(void);
};

#endif // _XIOCPCLIENT_H_INCLUDE_ 
