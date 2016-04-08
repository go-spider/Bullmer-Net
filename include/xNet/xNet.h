/*
* @note		：网络结构体定义
* @author	：Andy.Ro
* @email	：Qwuloo@qq.com
* @date		：2014-06
*/
#ifndef _XNET_H_INCLUDE_
#define _XNET_H_INCLUDE_

#include "../Interface/xTypes.h"
#include "../xSocket/xSocket.h"
#include "../Interface/xPacket.h"

/*
*
*	TCP 服务器:
*		1. 创建套接字(socket);
*
*		2. 绑定套接字到本地端口和IP地址(bind);
*
*		3. 设置套接字为监听模式(listen),准备接受客户连接请求;
*
*		4. 接受连接请求(accept),返回新建立连接的套接字;
*
*		5. 用已建立连接的套接字和客户端进行通信(send/recv);
*
*		6. 关闭套接字;
*
*	TCP 客户端:
*		1. 创建套接字(socket);
*
*		2. 向服务器发送连接请求(connect);
*
*		3. 与服务器进行通信(send/recv);
*
*		4. 关闭套接字;
*/

/*
*
*	UDP 服务器:
*		1. 创建套接字(socket);
*
*		2. 绑定套接字到本地端口和IP地址(bind);
*
*		3. 等待接收(recvfrom)并发送数据(sendto);
*
*		4. 关闭套接字;
*
*
*	UDP 客户端:
*		1. 创建套接字(socket);
*
*		2. 向服务器发送(sendto)并接收数据(recvfrom);
*
*		3. 关闭套接字;
*
*/

//------------------------------------------------------------------------
// io_t
//------------------------------------------------------------------------
typedef struct _io_t
{
#ifdef WIN32
	WSAOVERLAPPED	ol;
#endif

	// clientfd
	SOCKET			fd;

	char*			buf;
	u32_t			len;

	// 作为服务端
#define OP_ACCEPT	0
	// 作为客户端
#define OP_CONNECT	1

#define OP_READ		2
#define OP_WRITE	3
#define OP_TRANSMIT	4

	u8_t			op;
	
	//                        | thread_A |
	// -->> r_io_2,r_io_1 -->>|          |-->> r_io_1,r_io_2 -->>
	//					      | thread_B |
	// 投递读io序列编号，处理并发接收问题，按照投递io序列先后读取，先投递的数据在前(如上所示顺序反了)
//	u64_t			id;
	
	// 0：作为服务端 1：作为客户端
	u8_t			cc;
	
	// 作为客户端
	sockaddr_in		inaddr;// 连接到对端ip

	// 本端要主动关闭连接时，设置为TRUE，并投递OP_READ
	u8_t			closing;
}io_t;

//------------------------------------------------------------------------
// listen_t
//------------------------------------------------------------------------
typedef struct _listen_t
{
	// listenfd
	SOCKET		fd;

	ulong_t		ipaddr;					// 网络字节序ipaddr
	u16_t		port;					// 主机字节序port
	
	// 作为客户端
	sockaddr_in	inaddr;// 连接到对端ip

//	_listen_t*	next;

}listen_t,sock_t;

//------------------------------------------------------------------------
// connect_t/client_t
//------------------------------------------------------------------------
typedef struct _connect_t
{
	SOCKET		fd;

	u16_t		af;

	// 本地/远程ip
	ulong_t		l_ipaddr,r_ipaddr;		// 网络字节序ipaddr
	u16_t		port; 					// 主机字节序port

	// 当前投递读io序列号，处理并发接收问题，按照投递io序列先后读取，先投递的数据在前
//	u64_t		id;
	
	// 当前读取的序列编号，处理并发接收问题，按照投递io序列先后读取，先投递的数据在前
	u64_t		cgid;
	
	// 被丢弃的数据包数量
	u32_t		discard;

	// 已接收的数据包数量
	u32_t		total;
	
	// 发送数据包全局序列id
	u32_t		gid;
	
	// 发送数据包所属整体pid
	u32_t		pid;
	
	// 连接时间戳
	time_t		ts;
/**
*	packet_pool_t * sp; // 数据包发送BUF池
*	packet_pool_t * rp; // 数据包接收BUF池(缓冲池队列有数据则通知用户线性接收)
*	map_packet_t  * hp; // 用于缓存半包数据(前半包：含全部包头及部分包体或只含部分包头，后半包：含余下包体或余下包头及全部包体)
*	map_packet_t  * bp; // 用于缓存超大数据
**/
}connect_t,client_t;

#endif // _XNET_H_INCLUDE_