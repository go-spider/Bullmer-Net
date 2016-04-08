/*
* @note		：队列
* @author	：Andy.Ro
* @email	：Qwuloo@qq.com
* @date		：2014-06
* @copyright(C) 版权归作者所有
*
*/
#ifndef _XQUEUE_H_INCLUDE_
#define _XQUEUE_H_INCLUDE_

#include "../xNet/xNetDll.h"

//------------------------------------------------------------------------
// node_t<TYPE> 节点
//------------------------------------------------------------------------
template < class TYPE > class /*XNET_API*/ q_node_t
{
public:
	inline TYPE get_data(void)
	{
		return this->data;
	}
public:
	TYPE data;
	q_node_t<TYPE> * prev,*next;
};

//------------------------------------------------------------------------
// block_q_node_t 单(一)个内存块，由N个节点 sizeof(q_node_t<TYPE>) 构成
//------------------------------------------------------------------------
template < class TYPE > class /*XNET_API*/ block_q_node_t
{
public:
	// @note: 返回块数据部分
	inline q_node_t<TYPE>* get_data(void)
	{
		return (q_node_t<TYPE> *)((char *)this + sizeof(block_q_node_t));
	}

	// @note: 申请单(一)个内存块
	// @param header <block_q_node_t*&> : 块头
	// @param tail   <block_q_node_t*&> : 块尾
	static block_q_node_t * create(block_q_node_t*& header, block_q_node_t*& tail, u32_t N)
	{
		//////////////////////////////////////////////////////////////////////////
		/// sizeof(block_q_node_t) + sizeof(q_node_t<TYPE>) * N
		//////////////////////////////////////////////////////////////////////////
		block_q_node_t * block = (block_q_node_t *)new char[
			sizeof(block_q_node_t) + sizeof(q_node_t<TYPE>) * N];
			if ( ! block )
			{
				assert(FALSE);
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
		block_q_node_t<TYPE> * block = this;

		while( block )
		{
			char * pbytes = (char *)block;
			block = block->next;
			delete [] pbytes;
		}
	}
public:
	block_q_node_t * prev,*next;
};

//------------------------------------------------------------------------
// xQueue
//------------------------------------------------------------------------
template <class TYPE> class /*XNET_API*/ xQueue
{
public:
	xQueue<TYPE>(u32_t blocksize = 20);

	// @note：入队
	void enqueue(TYPE& data)
	{
		q_node_t<TYPE>* node = NULL;
		
		this->_head_node_mutex.enter();
		if ( node = this->_create(NULL,this->_head_node) )
		{
			memcpy_s(&node->data, sizeof(TYPE), &data, sizeof(TYPE));
			
			if( this->_head_node ) {
				_head_node->prev = node;
				this->_head_node = _head_node->prev;
			}
			else {
				this->_tail_node_mutex.enter();
				this->_tail_node = this-> _head_node = node;
				this->_tail_node_mutex.leave();
			}
			++this->_queue_size;
		}
		this->_head_node_mutex.leave();
	}
	
	inline BOOL empty() { return 0 == this->_queue_size; }

	// @note：出队
	q_node_t<TYPE> * dequeue(void)
	{
		q_node_t<TYPE> * node = NULL;
		
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

	// @note: 释放内存到池
	inline void free(q_node_t<TYPE> * node)
	{
		bzero(node, sizeof(q_node_t<TYPE>));

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
		this->_free_list_mutex.enter();
		
		// 不允许入队操作
		this->_head_node_mutex.enter();
		this->_head_node = NULL;
		this->_head_node_mutex.leave();
		
		// 不允许出队操作
		this->_tail_node_mutex.enter();
		this->_tail_node = NULL;
		this->_tail_node_mutex.leave();

		this->_list_mutex.enter();
		SAFE_CLEAR(_head_block);
		_head_block = _tail_block = NULL;
		this->_list_mutex.leave();
		
		_free_list = NULL;
		this->_free_list_mutex.leave();
	}
protected:
	// @param 申请内存块
	q_node_t<TYPE> * _create(q_node_t<TYPE> * prev = NULL, q_node_t<TYPE> * next = NULL);

public:
	
	u32_t _block_size;			// 单个内存块所包含的节点 sizeof(q_node_t<TYPE>) 数，每次申请一个内存块
	u32_t _queue_size;
	
	q_node_t<TYPE> * _head_node;	// 双链表头节点
	q_node_t<TYPE> * _tail_node;	// 双链表尾节点
	ESMT::xSection   _head_node_mutex;	// 同步头部写数据队列
	ESMT::xSection   _tail_node_mutex;	// 同步尾部读数据队列

	q_node_t<TYPE> * _free_list;				// 空闲节点链表
	ESMT::xMutex   _free_list_mutex;	// 同步空闲链表
	
	block_q_node_t<TYPE> * _head_block;	// 内存块头
	block_q_node_t<TYPE> * _tail_block;	// 内存块尾
	ESMT::xMutex	 _list_mutex;	// 同步块链表
public:
	inline ~xQueue(void)
	{
		this->clear();
	}
};

template < class TYPE >
inline xQueue<TYPE>::xQueue(u32_t blocksize /* = 20 */)
{
	_queue_size = 0;
	_head_node = _tail_node = NULL;
	_free_list   = NULL;
	_block_size  = blocksize;
	_head_block = _tail_block = NULL;
}

template < class TYPE >
inline q_node_t<TYPE>* xQueue<TYPE>::_create(q_node_t<TYPE> * prev, q_node_t<TYPE> * next)
{
	this->_free_list_mutex.enter();
	if( !this->_free_list )
	{
		this->_list_mutex.enter();
		block_q_node_t<TYPE>::create(this->_head_block, this->_tail_block, _block_size);
		
		q_node_t<TYPE> * node  = (q_node_t<TYPE> *)_head_block->get_data();
		node += _block_size - 1;
		
		for(u32_t cnt = this->_block_size; cnt > 0; --cnt, --node)
		{
			bzero(node, sizeof(q_node_t<TYPE>));
			node->prev = NULL;
			node->next = this->_free_list;
			_free_list = node;
		}
		this->_list_mutex.leave();
	}
	
	q_node_t<TYPE> * node = this->_free_list;
	_free_list = this->_free_list->next;
	node->prev = prev;
	node->next = next;
	this->_free_list_mutex.leave();
	return node;
}

#endif // _XQUEUE_H_INCLUDE_