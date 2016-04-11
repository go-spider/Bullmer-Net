/*
* 通用内存块及内存池实现
*
* --------------------------------------------------------------------------------
*
*			优点：支持追加缓存
*			缺点：
*				1.不支持不同类型结构或类对象共存在一个池上
*				2.由于是字节分配，无法调用构造函数及运行时类型信息(dynamic_cast<TYPE>失效)检查，不支持对象虚函数调用
* --------------------------------------------------------------------------------
*
* 1: 含追加缓存
*	xBuffer<io_t>  g_pool(5,1024);
*	p_node_t <io_t>* t = g_pool.alloc();
*	t->data.buf = t->get_offset();
*	t->data.len = g_pool.get_offset_size();
*	g_pool.free(t);
*	g_pool.clear();
* 2: 无追加缓存
*	xBuffer<xClient> g_pool(20,0);
*	p_node_t<xClient>* t = g_pool.alloc();
*	g_pool.free(t);
*	g_pool.clear();
*/

#ifndef _XBUFFER_P_H_INCLUDE_
#define _XBUFFER_P_H_INCLUDE_

#include "../xNet/xNetDll.h"
#include "../xThread/xThread.h"

//------------------------------------------------------------------------
// p_node_t<TYPE> 节点
//------------------------------------------------------------------------
class XNET_API p_node_t
{
public:
	// 返回节点数据部分
	inline char* get_data(void)
	{
//		return (char *)(this + 1);
		return (char *)((char *)this + sizeof(p_node_t));
	}
public:
	// 结构分布如下：
	// -------------------------
	// 指针结构 | 数据长度 | 追加缓存
	// -------------------------
	p_node_t * prev,*next;
	u32_t           bytes; // 数据长度
};

class ESMT::xMutex;
class ESMT::xEvent;

//------------------------------------------------------------------------
// block_p_node_t 单(一)个内存块，由N个节点(sizeof(p_node_t<TYPE>) + offset)构成
//------------------------------------------------------------------------
class XNET_API block_p_node_t
{
public:
	// @note: 返回块数据部分
	inline p_node_t* get_data(void)
	{
		return (p_node_t *)((char *)this + sizeof(block_p_node_t));
	}

	// @note: 申请单(一)个内存块
	// @param header <block_p_node_t*&> : 块头
	// @param tail   <block_p_node_t*&> : 块尾
	// @param offset <u32_t>		  : 追加缓存大小 sizeof(p_node_t<TYPE>) + offset
	// @param N      <u32_t>          : 块所包含的节点(sizeof(p_node_t<TYPE>) + offset)数
	static block_p_node_t * create(block_p_node_t*& header, block_p_node_t*& tail, u32_t offset, u32_t N)
	{
		//////////////////////////////////////////////////////////////////////////
		/// sizeof(block_p_node_t) + (sizeof(p_node_t<TYPE>) + offset) * N
		//////////////////////////////////////////////////////////////////////////
		block_p_node_t * block = (block_p_node_t *)new char[
				 sizeof(block_p_node_t) +
				(sizeof(p_node_t      ) + offset) * N];
		if ( ! block )
		{
			fprintf(stderr,"alloc/new failed,memery overflow.\n");
			return NULL;
		}
		block->prev = NULL;
		block->next = header;

		if( header )
		{
			header->prev = block;
			header = header->prev;
		}
		else tail = header = block;

		return block;
	}

	// @note: 清理内存块
	void clear(void)
	{
		block_p_node_t * block = this;

		while( block )
		{
			char * pbytes = (char *)block;
			block = block->next;
			delete [] pbytes;
		}
	}
public:
	block_p_node_t * prev,*next;
};

//------------------------------------------------------------------------
// xBuffer_p 内存池
//------------------------------------------------------------------------
class XNET_API xBuffer_p
{
public:
	// @param blocksize <u32_t> : 单个内存块中所包含的节点(p_node_t<TYPE>)数，每次申请一个内存块
	// @param offset    <u32_t> : 追加缓存大小 sizeof(p_node_t<TYPE>) + offset
	xBuffer_p(u32_t blocksize = 20,u32_t offset = 0)
		:_readable_event(TRUE,FALSE),_empty_event(TRUE,TRUE)
	{
		_queue_size = 0;
		_head_node = _tail_node = NULL;
		_free_list   = NULL;
		_block_size  = blocksize;
		_offset_size = offset;
		_head_block = _tail_block = NULL;
	}

	// @note: 从池中申请内存
	inline p_node_t * alloc(void)
	{
		return this->_create(NULL,NULL);
	}
	
	// @note: 入队：数据来了，写入数据(生产者)
	// @param data <TYPE const*> : 数据
	// @param buf  <char const*> : 追加数据(接口预留)
	// @param len  <u32_t>       : 追加数据大小(接口预留)
	p_node_t * enqueue(char const* buf = NULL, u32_t len = 0)
	{
		p_node_t * node = NULL;
		
		// 头部写入，尾部读出
		this->_head_node_mutex.enter();
		if ( node = this->_create(NULL,this->_head_node) )
		{
			// 实际所含数据长度
			node->bytes = len;
			
			// 拷贝追加缓存数据
			memcpy_s(node->get_data(), len, (void const*)buf, len);
			
			if( this->_head_node ) {
				_head_node->prev = node;
				this->_head_node = _head_node->prev;
			}
			else {
				this->_tail_node_mutex.enter();
				this->_tail_node = this-> _head_node = node;
				this->_tail_node_mutex.leave();

				// 有数据可读
//				__signaled(this->_readable_event);
				
				// 队列非空
//				__nonsignaled(this->_empty_event);
			}
			++this->_queue_size;
		}
		this->_head_node_mutex.leave();
		return node;
	}
	// @note: 等待数据
	p_node_t * wait_data(ulong_t timeout = INFINITE)
	{
//		__wait_signal(this->_readable_event, timeout);
		return this->dequeue();
	}
	// @note: 出队：有数据了，读出数据(消费者)
	p_node_t * dequeue(void)
	{
		p_node_t * node = NULL;
		
		// 头部写入，尾部读出
		this->_tail_node_mutex.enter();
		if ( this->_tail_node )
		{
			// 读的不是头节点
			if ( this->_tail_node != this->_head_node )
			{
				node = this->_tail_node;
				this->_tail_node = this->_tail_node->prev;
				node->prev = node->next = NULL;
				--this->_queue_size;
			}
			else
			{
				// 读头节点，可能正在写数据，等写完再读(不能同时读写相同节点)
				// 此时 _tail_node 与 _head_node 为相同节点，应此处上锁
				this->_head_node_mutex.enter();
				
				node = this->_tail_node;
				this->_tail_node = this->_tail_node->prev;
				node->prev = node->next = NULL;
				--this->_queue_size;

				if ( node == this->_head_node ) {
					
					assert( !this->_tail_node );

					this->_head_node = this->_tail_node/* = NULL*/;

					// 队列已空
//					__signaled(this->_empty_event);

					// 无数据可读
//					__nonsignaled(this->_readable_event);
				}
				else {
					
					assert( this->_tail_node );
					
					this->_tail_node->next = NULL;

					// 不能指向头节点，不要管它!!!
//					this->_tail_node = this-> _head_node;
				}
				
				this->_head_node_mutex.leave();
			}
		}
		this->_tail_node_mutex.leave();
		return node;
	}
	
	// @note：比较两数据包节点序列编号
	// @param pa <p_node_t*>：新数据节点
	// @param pb <p_node_t*>：待比较节点
	typedef int (*compare_func_ptr) (p_node_t * pa, p_node_t * pb);
	// @note: 根据比较规则添加到有序队列
	void orderinsert(compare_func_ptr compare_ptr, char const* buf, u32_t len)
	{
		this->_head_node_mutex.enter();
		p_node_t * pre = NULL,*cur = _head_node,*ptr;
		
		// ptr节点为新数据节点，cur节点为遍历节点
		if ( ptr = this->_create(NULL,NULL) )
		{
			// 实际所含数据长度
			ptr->bytes = len;
			
			// 拷贝追加缓存数据
			memcpy_s(ptr->get_data(), len, (void const*)buf, len);

			while ( cur )
			{
				// 执行比较规则
				if( (*compare_ptr)(ptr, cur) > 0 )
				{
					break;
				}
				pre = cur;
				cur = cur->next;
			}
			// 当前节点之前插入
			if ( !pre )
			{
				// 插入头部
				ptr-> next = this->_head_node;
				_head_node = ptr;

				++this->_queue_size;
			}
			else
			{
				ptr->next = cur;
				pre->next = ptr;
				
				++this->_queue_size;
			}
		}
		this->_head_node_mutex.leave();
	}
	// @note: 
	inline p_node_t * top(void)
	{
		return this->_head_node;
	}
	// note: 弹出队头节点
	inline p_node_t * pop(void)
	{
		p_node_t * node  = this->_head_node;
		this->_head_node = node->next;
		node->next = node->prev = NULL;
		if( this->_head_node )
			this->_head_node->prev = NULL;
		--this->_queue_size;
		return node;
	}

	// @note: 释放内存到池
	inline void free(p_node_t * node)
	{
// 		bzero(node->get_data(), this->_offset_size);
		bzero(node, sizeof(p_node_t) + this->_offset_size);

		this->_free_list_mutex.enter();
		node->prev = NULL;
		node->next = this->_free_list;
		if( this->_free_list ) _free_list->prev = node;
		this->_free_list = node;
		this->_free_list_mutex.leave();
	}
	
	// @note: 清理内存池
	inline void clear(BOOL linear = FALSE)
	{
		// 不允许申请节点
//		this->_free_list_mutex.enter();
		
		// 不允许入队操作
//		this->_head_node_mutex.enter();
		this->_head_node = NULL;
//		this->_head_node_mutex.leave();
		
//		if ( linear ) __wait_signal(this->_empty_event,INFINITE);
		// 不允许出队操作
//		this->_tail_node_mutex.enter();
		this->_tail_node = NULL;
//		this->_tail_node_mutex.leave();

//		this->_list_mutex.enter();
		SAFE_CLEAR(_head_block);
		_head_block = _tail_block = NULL;
//		this->_list_mutex.leave();
		
//		__signaled(this->_empty_event);
//		__nonsignaled(this->_readable_event);

		_free_list = NULL;
//		this->_free_list_mutex.leave();
	}
	// @note: 返回追加缓存大小
	inline u32_t get_offset_size(void)
	{
		return this->_offset_size;
	}
protected:
	// @param 申请内存块
	inline p_node_t * _create(p_node_t * prev = NULL, p_node_t * next = NULL)
	{
		this->_free_list_mutex.enter();
		if( !this->_free_list )
		{
			this->_list_mutex.enter();
			block_p_node_t::create(this->_head_block, this->_tail_block, _offset_size, _block_size);
#if 0
			// _offset_size == 0
			p_node_t * node  = (p_node_t *)_head_block->get_data();
			node += ()_block_size - 1;

			for(u32_t cnt = this->_block_size; cnt > 0; --cnt, --node)
			{
				// 			bzero(node->get_data(), this->_offset_size);
				bzero(node, sizeof(p_node_t) + this->_offset_size);
				node->prev = NULL;
				node->next = this->_free_list;
				_free_list = node;
			}
#else
			// _offset_size >  0
			char * node  = (char *)_head_block->get_data();
			node += (sizeof(p_node_t)+this->_offset_size) * (_block_size - 1);

			for(u32_t cnt = this->_block_size; cnt > 0;
				--cnt, node -= (sizeof(p_node_t)+this->_offset_size))
			{
				// 			bzero(((p_node_t *)node)->get_data(), this->_offset_size);
				bzero(node, sizeof(p_node_t) + this->_offset_size);

				((p_node_t *)node)->prev = NULL;
				((p_node_t *)node)->next = this->_free_list;
				_free_list = (p_node_t *)node;
			}
#endif
			this->_list_mutex.leave();
		}

		p_node_t * node = this->_free_list;
		_free_list = this->_free_list->next;
		node->prev = prev;
		node->next = next;
		this->_free_list_mutex.leave();
		return node;
	}

public:
	
	u32_t _offset_size;			// 追加缓存大小 sizeof(p_node_t<TYPE>) + offset
	u32_t _block_size;			// 单个内存块所包含的节点(sizeof(p_node_t<TYPE>) + offset)数，每次申请一个内存块
	u32_t _queue_size;
	
	p_node_t * _head_node;	// 双链表头节点
	p_node_t * _tail_node;	// 双链表尾节点
	ESMT::xSection   _head_node_mutex;	// 同步头部写数据队列
	ESMT::xSection   _tail_node_mutex;	// 同步尾部读数据队列
	
	ESMT::xEvent   _readable_event; // 事件通知并发读
	ESMT::xEvent   _empty_event;	// 数据队列为空

	p_node_t * _free_list;				// 空闲节点链表
	ESMT::xMutex   _free_list_mutex;	// 同步空闲链表
	
	block_p_node_t * _head_block;	// 内存块头
	block_p_node_t * _tail_block;	// 内存块尾
	ESMT::xMutex	 _list_mutex;	// 同步块链表
public:
	inline ~xBuffer_p(void)
	{
		this->clear();
	}
};

#endif // _XBUFFER_P_H_INCLUDE_
