/*
* @note		：同步事件/信号量/互斥/临界区/线程/线程池实现(win32/linux)，不支持unicode
*/

#ifndef _XTHREAD_H_INCLUDE_
#define _XTHREAD_H_INCLUDE_

#ifdef WIN32
#include <Windows.h>
#include <process.h>
#else

#include <sched.h>
#include <pthread.h>
#include <semaphore.h> // POSIX信号量(用户态)
//#include <sys/sem.h>   // SYSTEM V信号量(用户态)
#include <sys/types.h>
#include <unistd.h>
#define INFINITE 0xFFFFFFFF
#endif

#include "../Interface/xTypes.h"

#ifdef WIN32
typedef u32_t pthread_t;
#endif

namespace ESMT {

	class xSyncObject;
	class xEvent;					// 事件内核对象
	class xSemaphore;				// 信号量内核对象：linux 下仅实现了POSIX信号量
	class xMutex;					// 互斥内核对象
	class xSection;					// 临界区用户对象

	class xThread;					// 线程类
	class xThreadPool;				// 线程池

	//------------------------------------------------------------------------
	// xSyncObject 接口类
	//------------------------------------------------------------------------
	class XNET_API xSyncObject
	{
	public:
		/*XNET_API*/ xSyncObject(void);
	
	public:
		// @note: 上锁
		virtual BOOL lock(void)							/*= 0;*/ { return FALSE; }
		
		// @note: 挂起调用线程，直到该内核对象被解锁
		// @param timeout <ulong_t> : 等待时间(毫秒)
		virtual BOOL wait(ulong_t timeout = INFINITE)	/*= 0;*/ { return FALSE; }
		
		// @note: 解锁
		virtual BOOL Unlock(void)						/*= 0;*/ { return FALSE; }
		
		// @note: 占用资源并进入访问
		virtual BOOL enter(void)						/*= 0;*/ { return FALSE; }

		// @note: 离开并释放占用资源
		virtual BOOL leave(void)						/*= 0;*/ { return FALSE; }

	public:
		/*XNET_API*/ virtual ~xSyncObject(void);
	};

	//------------------------------------------------------------------------
	// xEvent 多线程事件同步
	// linux 下参考：http://blog.csdn.net/ffilman/article/details/4871920
	//------------------------------------------------------------------------
	class XNET_API xEvent : public xSyncObject
	{
#define		__signaled(e)		( (e).unlock() )
#define		__nonsignaled(e)	( (e).lock()   )
#define		__wait_signal(e,s)	( (e).wait(s)  )
	public:
		/*XNET_API*/ xEvent(
			BOOL manualreset = FALSE, BOOL initstate = FALSE
#ifdef WIN32
		   ,LPCTSTR lpName = NULL, LPSECURITY_ATTRIBUTES lpsa = NULL
#else
		   ,u32_t pshared = PTHREAD_PROCESS_PRIVATE
		   ,u32_t type = PTHREAD_MUTEX_TIMED_NP
#endif
			);
	public:
		// @note: 设置事件未受信(non-signaled)状态[人工重置(manual-reset)事件]
		/*inline*/ BOOL lock();
		
		// @note: 挂起调用线程，直到该事件对象受信(signaled)
		// @param timeout <ulong_t> : 等待时间(毫秒)
		/*inline*/ BOOL wait(ulong_t timeout = INFINITE);
		
		// @note: 设置事件受信(signaled)状态
		/*inline*/ BOOL unlock();
	
	public:
#ifdef WIN32
		HANDLE _event;
#else
#if 0
		typedef struct _cond_check_t
		{
			typedef BOOL (*cond_check_callback_func)(void * args);
			cond_check_callback_func _handler;
			void * _args;
		}cond_check_t;
		cond_check_t _cond_check;
#endif
		pthread_mutex_t _mutex;
		pthread_cond_t _cond;
		BOOL _manual_reset,_signaled;
#endif
	public:
		/*XNET_API*/ ~xEvent(void);
	};

	//------------------------------------------------------------------------
	// xSemaphore，控制多线程对同一共享资源的并发访问
	// linux 下参考：http://blog.csdn.net/qinxiongxu/article/details/7830537
	//------------------------------------------------------------------------
	class XNET_API xSemaphore : public xSyncObject
	{
	public:
		// @note:构造信号内核对象 
		// @param initvalue <ulong_t> : 初始可用访问资源计数
		// @param maxvalue  <ulong_t> : 允许最大访问资源计数，指定并发访问共享资源的最大线程数
		/*XNET_API*/ xSemaphore(ulong_t initvalue = 1
#ifdef WIN32
			,ulong_t maxvalue = 1
#endif
			,char const * name = NULL);
	
	public:	
		// @note: 占用资源并进入访问
		/*inline*/ BOOL enter();
		
		// @note: 离开并释放占用资源
		/*inline*/ BOOL leave();
	
	public:
#ifdef WIN32
		HANDLE _sem;
#else
		sem_t  _sem,*_p_sem;
		std::string _S_NAME;
#endif
	public:
		/*XNET_API*/ ~xSemaphore(void);
	};

	//------------------------------------------------------------------------
	// xMutex 控制多线程对同一共享资源的互斥访问
	//------------------------------------------------------------------------
	class XNET_API xMutex : public xSyncObject
	{
	public:
		/*XNET_API*/ xMutex(
#ifdef WIN32
			 LPCTSTR lpName = NULL, BOOL bInitOwner = FALSE, LPSECURITY_ATTRIBUTES lpsa = NULL
#else
			 u32_t pshared = PTHREAD_PROCESS_PRIVATE
			,u32_t type = PTHREAD_MUTEX_TIMED_NP
#endif
			);
	
	public:
		// @note: 占用资源并互斥访问
		/*inline*/ BOOL enter();
		
		// @note: 离开并释放占用资源
		/*inline*/ BOOL leave();
	
	public:
#ifdef WIN32
		HANDLE          _mutex;
#else
		pthread_mutex_t _mutex;
#endif
	public:
		/*XNET_API*/ ~xMutex(void);
	};

	//------------------------------------------------------------------------
	// xSection，多线程访问临界资源
	//------------------------------------------------------------------------
	class XNET_API xSection : public xSyncObject
	{
	public:
		/*XNET_API*/ xSection(
#ifdef WIN32
#else
			 u32_t pshared = PTHREAD_PROCESS_PRIVATE
			,u32_t type = PTHREAD_MUTEX_TIMED_NP
#endif
			);
	
	public:
		// @note: 占用资源并进入访问
		/*inline*/ BOOL enter();

		// @note: 离开并释放占用资源
		/*inline*/ BOOL leave();
	
	private:
#ifdef WIN32
		CRITICAL_SECTION _cs;
#else
		pthread_mutex_t  _cs;
#endif
	public:
		/*XNET_API*/ ~xSection(void);
	};

	class xThreadPool;

	//------------------------------------------------------------------------
	// xThread 线程类
	//------------------------------------------------------------------------
	class XNET_API xThread
	{
		friend class xThreadPool;
	public:
		/*XNET_API*/ xThread(void);
		/*XNET_API*/ xThread(void * argument
#ifndef WIN32
			,BOOL detach = FALSE
			,u32_t scope = PTHREAD_SCOPE_SYSTEM
			,u32_t inherit = PTHREAD_EXPLICIT_SCHED
			,u32_t policy = SCHED_OTHER
#endif
			,u32_t priority = 0
			);
		/*virtual*/ BOOL stop() { return this->_done = TRUE;    }
	public:
		inline HANDLE operator *() const     { return _handler; }
		inline u32_t getid() const           { return (u32_t)_tid; }
		inline BOOL done() const             { return this->_done; }
		inline BOOL idle() const			 { return this->_idle; }
		inline void	setarg(void * argment)	 { this->_arg_cs.enter(); this->_arg = argment; this->_arg_cs.leave(); }
		inline void * getarg()
		{
			this->_arg_cs.enter();
			void * arg = this->_arg;
			this->_arg_cs.leave();
			return arg;
		}
	public:
		BOOL start(void * argument = NULL
#ifndef WIN32
			,BOOL detach = FALSE
			,u32_t scope = PTHREAD_SCOPE_SYSTEM     // PTHREAD_SCOPE_PROCESS，PTHREAD_SCOPE_SYSTEM
			,u32_t inherit = PTHREAD_EXPLICIT_SCHED // 继承性：PTHREAD_EXPLICIT_SCHED，PTHREAD_INHERIT_SCHED
			,u32_t policy = SCHED_OTHER             // 调度策略：SCHED_FIFO，SCHED_RR，SCHED_OTHER ..
#endif
			,u32_t priority = 0
			);
		BOOL wait(u32_t timeout = INFINITE);
#ifndef WIN32
		void detach();
#endif
		BOOL setpriority(u32_t priority);
		u32_t getpriority();
		void suspend();
		void resume();
	protected:
		// @note：run()运行时调用修改优先级
		BOOL onpriority();
	protected:
		void release();
	protected:
		virtual u32_t __fastcall run(void * argument) = 0;
	protected:
// 		typedef struct _thread_param_t
// 		{
// 			xThread * pthis;
// 			void * argument;
// 		}thread_param_t;
// 		thread_param_t _param;
		
		pthread_t      _tid;
		u32_t          _priority; // 待修改的优先级
		BOOL           _modify;   // 修改标记
		BOOL           _done;
		BOOL		   _idle;	  // 空闲：被挂起
#ifdef WIN32
		HANDLE         _handler;
#endif
		void *	       _arg;
		ESMT::xSection _arg_cs;
	private:
		static u32_t __stdcall routine(void * param);
	public:
		/*XNET_API*/ virtual ~xThread(void);
	};

	//------------------------------------------------------------------------
	// xThreadPool 线程池
	//------------------------------------------------------------------------
	class XNET_API xThreadPool
	{
		typedef std::map<u32_t,xThread*> map_id_thread_t;
		typedef map_id_thread_t::iterator itor_id_thread_t;
	public:
		/*XNET_API*/ xThreadPool(void);

		xThread * pop_idle();
		void push_idle(xThread * thread);
		size_t idle_size(void);

		xThread * add(xThread * thread);
		void del(u32_t id);
		void clear(void);
		
		inline void enter() { this->_thread_pool_cs.enter(); }
		inline void leave() { this->_thread_pool_cs.leave(); }

		xThread * get(u32_t id);
		size_t size(void);
		
	public:
		/*XNET_API*/ ~xThreadPool(void);
	private:
		map_id_thread_t _thread_pool;
		ESMT::xSection	_thread_pool_cs;
		std::stack<xThread*> _idle_list;
		ESMT::xSection       _idle_list_cs;
	};
} // namespace ESMT

#endif // _XTHREAD_H_INCLUDE_
