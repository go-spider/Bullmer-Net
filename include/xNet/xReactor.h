/*
* @note		：xReactor 响应触发器
*/

#ifndef _XREACTOR_H_INCLUDE_
#define _XREACTOR_H_INCLUDE_

#define MAX_PROCESS_THREAD_SIZE 2000

class ESMT::xThread;

class xClientFactory;
class xConnectionFactory;

//------------------------------------------------------------------------
// xReactor
//------------------------------------------------------------------------
class XNET_API xReactor
{
	friend class xNetObject;
public:
	//------------------------------------------------------------------------
	// xCollector
	//------------------------------------------------------------------------
	class XNET_API xCollector : public ESMT::xThread
	{
	public:
		xCollector(void * argument);
	private:
		u32_t __fastcall run(void * argument);
	};
	//------------------------------------------------------------------------
	// xDispatcher
	//------------------------------------------------------------------------
	class XNET_API xDispatcher : public ESMT::xThread
	{
	public:
		xDispatcher(void * argument);
		void __fastcall dispatch(xNetObject * object, e_event_t e);
	private:
		u32_t __fastcall run(void * argument);
	};
	//------------------------------------------------------------------------
	// xConsumer
	//------------------------------------------------------------------------
	class XNET_API xConsumer : public ESMT::xThread
	{
	public:
		xConsumer(void * argument);
	private:
		u32_t __fastcall run(void * argument);
	};
public:
	// @note：作为 TCP 服务器监听远程连接
	// @param factory <xClientFactory*>：作为 TCP 服务器工厂
	// @param port				<u16_t>：起始端口
	// @param scope				<u32_t>：监听范围
	static void listenTCP(xClientFactory* factory, u16_t port = 6000, u32_t scope = 1);

	// @note: 作为 TCP 客户端连接远程服务器
	// @param factory <xConnectionFactory*>：作为 TCP 客户端工厂
	// @param ipaddr			  <ulong_t>：目标主机ip
	// @param port					<u16_t>：连接起始端口
	static void connectTCP(xConnectionFactory* factory, char const* ipaddr, u16_t port = 6000);

	// @param nMax <u32_t>：事务处理最大并发线程数(不超过 MAX_PROCESS_THREAD_SIZE)
	static int poll(u32_t nMax = 1);
	
	static xDispatcher * dispatcher(void);
	
	static u32_t total();
	static void increment();
	static void decrement();
};

#endif // _XREACTOR_H_INCLUDE_
