/*
* @note		：iocp服务器
* @author	：Andy.Ro
* @mark		：三个类：xNetFactory，xClient，xConnection 供外部继承(位于/xNet/目录下)
*/

#ifndef _XIOCP_H_INCLUDE_
#define _XIOCP_H_INCLUDE_

#ifdef MAX_THREAD
#undef MAX_THREAD
#endif

#ifndef WSA_MAXIMUM_WAIT_EVENTS
#define WSA_MAXIMUM_WAIT_EVENTS 64
#endif

#define MAX_THREAD		1
#define mAX_LISTEN		WSA_MAXIMUM_WAIT_EVENTS // 最大监听端口数
#define MAX_ACCEPT		150						// 最大并发连接数

class ESMT::xThread;
class ESMT::xThreadPool;
class ESMT::xEvent;

class xClientFactory;

// 连接异常中断检测
// http://blog.csdn.net/ghosc/article/details/5718978

//------------------------------------------------------------------------
// iocp服务器(单例模式)
//------------------------------------------------------------------------
class XNET_API xIocp
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
	// 监听线程：监测连接请求网络事件(FD_ACCEPT)
	class XNET_API xListener: public ESMT::xThread
	{
	public:
		u32_t __fastcall run(void * argument);
	};
	// 工作线程
	class XNET_API xWorker: public ESMT::xThread
	{
	public:
		xWorker(void * argument);
		u32_t __fastcall run(void * argument);
	};
public:
	static xIocp * shareObject(xClientFactory* factory);
private:
	// @note: 指定代理服务对象
	// @param service <xClientFactory*> : 代理服务对象
	//
	// @example:
	//	UserServer factory;
	//	xIocp iocp(&factory);
	//
	xIocp(xClientFactory* factory);
public:
	// @note：作为 TCP 服务器监听远程连接
	// @param port	<u16_t> : 起始端口
	// @param scope	<u32_t> : 监听范围
	void listenTCP(u16_t port = 6000, u32_t scope = 1);
	void stop(void) { this->_shutdown = TRUE; }
	// @note: 清理所有监听套接字对象及对应监听网络事件
	void close_all_listen(void);
	
	// @note: 清除未决acceptEx投递，不再响应新的连接请求
	void clear_pending_accept(void);
	// @note: 关闭所有连接对象(本端主动关闭)[正常关闭]
	void notify_close_all(void);
	// @note: accept/recv/send io buf 投递
	int post_accept_buf(SOCKET fd);
	int post_recv_buf  (SOCKET fd);
	int post_send_buf  (SOCKET fd, char const* buf, ulong_t len);
	int post_close_buf (SOCKET fd);
	
	// @note: accept/recv/send io buf 释放
	void free_accept_buf(node_t<io_t>* io);
	void free_recv_buf  (node_t<io_t>* io);
	void free_send_buf	(node_t<io_t>* io);

protected:
	// @note: 退出所有工作线程
	void _end_workers(void);

	// @note: 监听指定端口范围
	// @param port	<u16_t> : 起始端口
	// @param scope	<u32_t> : 监听范围
	int _listening(u16_t port = 6000, u32_t scope = 1);

	// @note: io操作
	// @param sock	<listen_t*>     : 监听套接字(listenfd)对象
	// @param io	<node_t<io_t>*>	: io对象
	// @param bytes	<u32_t>	        : 传输字节数
	// @param err	<u32_t>	        : 错误代码
	int _handle_io(listen_t* sock, node_t<io_t>* io, ulong_t bytes, u32_t err);

	// @note: 投递AcceptEx
	// @param sock	<SOCKET>        : 监听套接字(listenfd)
	// @param io	<node_t<io_t>*>	: io_t新对象
	int _post_accept(SOCKET fd, node_t<io_t>* io);
	
	// @note: 投递WSARecv
	// @param fd	<SOCKET>        : 客户套接字(clientfd)
	// @param io	<node_t<io_t>*>	: io_t新对象
	int _post_recv  (SOCKET fd, node_t<io_t>* io);
	
	// @note: 投递WSASend
	// @param fd	<SOCKET>        : 客户套接字(clientfd)
	// @param io	<node_t<io_t>*>	: io_t新对象
	int _post_send  (SOCKET fd, node_t<io_t>* io);

	// @note: 监听指定ip地址和端口
	// @param ipaddr <ulong_t>: 网络字节序ipaddr
	// @param port   <u16_t>  : 主机字节序port
	int _listening_one(ulong_t ipaddr, u16_t port);
private:
	BOOL _shutdown;
	HANDLE _hComPort;					// 完成端口
	u16_t _port;						// 监始端口
	u32_t _scope_port;					// 监听范围
	map_fd_listen_t _listen_list;
	// 已经投递的未决的AcceptEx io 列表
	map_fd_io_t _pending_accps;
public:
	// 监听FD_ACCEPT/FD_CLOSE网络事件，检测到连接请求则自动投递AcceptEx来响应连接，
	// 之前是初始投递固定若干数量AcceptEx来接受客户连接，现在改为自动检测投递方式
	WSAEVENT _net_event[mAX_LISTEN];
	ESMT::xMutex _event_list_cs;

	xListener _listener;				// 监听者线程
	ESMT::xThreadPool _workers;			// 工作者线程池
	xClientFactory * _factory;			// 代理服务对象
	ESMT::xSection	 _factory_cs;
public:
	BOOL _init_LPFN_PTR(void);
	LPFN_ACCEPTEX _lpfnAcceptEx;
	LPFN_GETACCEPTEXSOCKADDRS _lpfnGetAcceptExSockAddrs;
public:
	~xIocp(void);
};

#endif // _XIOCP_H_INCLUDE_
