/*
*  更灵活通用的内存池扩展实现
*
*	优点：
*	    1.支持追加缓存，并且支持任意大小(追加数据有多大就追加多大缓存,不过相同结构类型尽量追加一样大，如1024字节)
*	    2.支持不同类型结构或类对象共存在一个池上
*	缺点：由于是字节分配，不支持对象虚函数调用
*
*	注意：由于是字节分配，无法调用构造函数及运行时类型信息(dynamic_cast<TYPE>失效)检查，不过丝毫不影响不同类型数据的正常读取!!!，数据入队列时指定唯一标识类或TYPE结构的 typeid,
*		  数据出队列时即可获取指定 typeid 相关类型的数据!!!!，另外，可能遇到TYPE结构类型不同，
*		  但是size大小相同的不同TYPE对象数据压入同一个池中，所以尽量避免用 sizeof(TYPE) 作为 typeid，
*		  当然在预先知道没有相同size大小的不同TYPE结构或类时是可以用的，所以最好用唯一标识类或结构体的静态成员作为 typeid
*
*  struct t1_t t1;
*  struct t2_t t2;
*
*  xBufferEx pool;
*  x_node_t * ptr;
*
*  u32_t typeid_1 = 1001,size_type_1 = sizeof(t1_t);
*  u32_t typeid_2 = 1002,size_type_2 = sizeof(t2_t);
*
*  // 压入 t1_t,t2_t 两不同类型结构对象数据到相同池中
*  pool.enqueue(&t1,typeid_1,size_type_1);
*  pool.enqueue(&t2,typeid_2,size_type_2);
*
*  // 从池中分别提取 t1_t,t2_t 结构对象数据
*  ptr = pool.dequeue(typeid_1,sizeof(t1_t));
*  while( ptr )
*  {
* 	   // 读取t1_t结构数据
*      ...
*      // 读取完则释放节点
*      pool.free(ptr,typeid_1,type_size_1,0);
* 
*      // 继续提取下一个数据
*      ptr = pool.dequeue(typeid_1,sizeof(t1_t));
*  }
*  
*  ptr = pool.dequeue(typeid_2,sizeof(t2_t));
*  while( ptr )
*  {
* 	   // 读取t2_t结构数据
*      ...
*      // 读取完则释放节点
*      pool.free(ptr,typeid_2,type_size_2,0);
* 
*      // 继续提取下一个数据
*      ptr = pool.dequeue(typeid_2,sizeof(t2_t));
*  }
*  
*  // 最后内存池不用了则释放
*  pool.clear();
*
*/

#ifndef _XBUFFEREX_H_INCLUDE_
#define _XBUFFEREX_H_INCLUDE_

#include "../xNet/xNetDll.h"
//#include "../Interface/xTypes.h"
#include "../xThread/xThread.h"

class x_node_t;
class x_block_node_t;

extern u32_t const ptr_size;
extern u32_t const ptr_sizex2;
extern u32_t const block_size;

class XNET_API x_node_t
{
public:
	inline void * get_data(void)
	{
		return (void *)(char *)this;
	}

	inline x_node_t ** get_prev_ptr(u32_t type_size)
	{
		return (x_node_t **)((char *)this+type_size);
	}

	inline x_node_t ** get_next_ptr(u32_t type_size)
	{
		return (x_node_t **)((char *)this+type_size + ptr_size);
	}

	inline char * get_offset(u32_t type_size)
	{
		return (char *)((char *)this + type_size + ptr_sizex2);
	}
protected:
	// 与xBuffer一样，结构分布如下：
	// --------------------------
	// TYPE结构 | 指针结构 | 追加缓存
	// --------------------------
	// 至于为什么这样分布查看xBuffer中解释，不再赘述
	// 这里和xBuffer区别就是TYPE和指针都隐藏，为什么这样做呢？这里解释下，因为我们可能用一个内存池保存不同类型的数据，
	// 所以呢，TYPE 类型不能定死，也不能用模板，俗话说模板模板，就是按样板定制的来，所以模板的缺点还是很明显的，
	// 由于支持不同size类型数据，所以指针位置就不能固定死了，让其动态变化，所以成员定义都是无形的，所谓以无形为有形，
};

//------------------------------------------------------------------------
// x_block_node_t 单(一)个内存块，由N个节点(sizeof(x_node_t<TYPE>) + offset)构成
//------------------------------------------------------------------------

class XNET_API x_block_node_t
{
public:
	// @note: 返回块数据部分
	inline x_node_t* get_data(void)
	{
		return (x_node_t *)((char *)this + block_size);
	}

	// @note: 申请单(一)个内存块
	// @param header	  <x_block_node_t*&> : 块头
	// @param tail		  <x_block_node_t*&> : 块尾
	// @param type_size	  <u32_t>		     : TYPE结构大小
	// @param offset_size <u32_t>		     : 追加缓存大小 sizeof(x_node_t<TYPE>) + offset_size
	// @param N			  <u32_t>            : 块所包含的节点(sizeof(x_node_t<TYPE>) + offset_size)数

	static x_block_node_t * create(x_block_node_t*& header, x_block_node_t*& tail, u32_t type_size, u32_t offset_size, u32_t N)
	{
		// sizeof(x_block_node_t) + (type_size + 2 * sizeof(x_node_t*) + offset_size) * N
			
		x_block_node_t * block = (x_block_node_t *)new char[
				 block_size +
				(type_size  + ptr_sizex2 + offset_size) * N];
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

	//清理内存块
	void clear(void)
	{
		x_block_node_t * block = this;

		while( block )
		{
			char * pbytes = (char *)block;
			block = block->next;
			delete [] pbytes;
		}
	}
public:
	x_block_node_t * prev,*next;
};

// xBufferEx 扩展内存池

class XNET_API xBufferEx
{
	
	typedef u64_t key_size,key_typeid;
	
	// this->_free_list[type_size + offset_size]
	typedef std::map<key_size, x_node_t*> map_size_node_t;
	typedef map_size_node_t::iterator    itor_size_node_t;
	
	// this->_head_node[typeid],this->_tail_node[typeid]
	typedef std::map<key_typeid, x_node_t*> map_typeid_node_t;
	typedef map_typeid_node_t::iterator    itor_typeid_node_t;
public:
	struct class_type_t
	{
		u64_t type_id;					// 结构id(最好是标识结构的静态成员地址)
		u32_t type_size, offset_size;	// 结构大小，追加缓存大小
	};
	
	//blocksize <u32_t> : 单个内存块中所包含的节点(x_node_t)数，每次申请一个内存块

	xBufferEx(u32_t blocksize = 20)
	{
		this->_head_node.clear();
		this->_tail_node.clear();
		this->_free_list.clear();
		_block_size  = blocksize;
		_head_block = _tail_block = NULL;
	}
	
	inline x_block_node_t *& get_head_block() { return this->_head_block; }
	inline x_block_node_t *& get_tail_block() { return this->_tail_block; }
	inline x_node_t * get_head_block_data() { return this->_head_block->get_data(); }
	inline x_node_t * get_tail_block_data() { return this->_tail_block->get_data(); }

	// 从池中申请内存
	inline x_node_t* alloc(u32_t type_size, u32_t offset_size)
	{
		return this->_create(type_size,offset_size,NULL,NULL);
	}

	// 入队:写入数据
	// data  <TYPE const*> : 数据
	// type_id     <u64_t> : 唯一标识TYPE结构或类
	// type_size   <u32_t> : TYPE结构或类大小
	// offset_size <u32_t> : 追加缓存大小
	// buf   <char const*> : 追加缓存数据(接口预留)
	x_node_t * enqueue(void const* data, u64_t type_id, u32_t type_size, u32_t offset_size,char const* buf = NULL)
	{
		x_node_t* node = NULL;
		
		// 头部写入，尾部读出
		this->_head_node_mutex.enter();
		if ( node = this->_create(type_size, offset_size, NULL,this->_head_node[type_id]) )
		{
			// 拷贝数据
			memcpy_s(node->get_data(), type_size, (void const*)data, type_size);

			if( this->_head_node[type_id] ) {
				*this->_head_node[type_id]->get_prev_ptr(type_size) = node;
				this ->_head_node[type_id] = *_head_node[type_id]->get_prev_ptr(type_size);
			}
			else {
				this->_tail_node[type_id] = this-> _head_node[type_id] = node;
				// 队列非空
				__nonsignaled(this->_empty_event);
			}
			
			// 有数据可读
			__signaled(this->_readable_event);
		}
		this->_head_node_mutex.leave();
		
		return node;
	}
	// 等待数据
	x_node_t * wait_data(u64_t type_id,u32_t type_size)
	{
		__wait_signal(this->_readable_event,INFINITE);
		return dequeue(type_id,type_size);
	}
	// 出队：读出数据(提取指定结构类型的数据)
	// type_id     <u64_t> : 唯一标识TYPE结构或类
	// type_size   <u32_t> : TYPE结构或类大小
	x_node_t* dequeue(u64_t type_id,u32_t type_size)
	{
		x_node_t* node = NULL;
		
		// 头部写入，尾部读出
		this->_tail_node_mutex.enter();
		if ( this->_tail_node[type_id] )
		{
			// 读的不是头节点
			if ( this->_tail_node[type_id] != this->_head_node[type_id] )
			{
				x_node_t* node = this->_tail_node[type_id];
				this->_tail_node[type_id] = *this->_tail_node[type_id]->get_prev_ptr(type_size);
				*node->get_prev_ptr(type_size) = *node->get_next_ptr(type_size) = NULL;
			}
			else
			{
				// 读的是头节点，可能正在写数据，等写完再读(因为不能同时读写相同节点)
				__wait_signal(this->_readable_event,INFINITE);

				node = this->_tail_node[type_id];
//				this->_tail_node[type_id] = *this->_tail_node[type_id]->get_prev_ptr(type_size);
				*node->get_prev_ptr(type_size) = *node->get_next_ptr(type_size) = NULL;

				this->_head_node_mutex.enter(); assert( this->_tail_node[type_id] == this->_head_node[type_id] );

				if ( this->_tail_node[type_id] == this->_head_node[type_id]) {
					this->_tail_node[type_id] = this-> _head_node[type_id] = NULL;
					// 队列已空
					__signaled(this->_empty_event);
				}
				else {
					 this->_tail_node[type_id] = *this->_tail_node[type_id]->get_prev_ptr(type_size); assert( this->_tail_node[type_id] );
					*this->_tail_node[type_id]->get_next_ptr(type_size) = NULL;
				}

				this->_head_node_mutex.leave();
			}
			return node;
		}
		this->_tail_node_mutex.leave();
		
		return node;
	}

	// 释放内存到池
	// type_id     <u64_t> : 唯一标识TYPE结构或类
	// type_size   <u32_t> : TYPE结构或类大小
	// offset_size <u32_t> : 追加缓存大小
	inline void free(x_node_t* node, u64_t type_id, u32_t type_size, u32_t offset_size)
	{
		u64_t size_key = type_size + offset_size;

		// bzero(node->get_data(), type_size);
		// bzero(node->get_offset(type_size), offset_size);
		bzero(node, type_size + ptr_sizex2 + offset_size);
		*node->get_prev_ptr(type_size) = NULL;
		*node->get_next_ptr(type_size) = this->_free_list[size_key];
		if( this->_free_list[size_key] )
			* _free_list[size_key]->get_prev_ptr(type_size) = node;
		this->_free_list[size_key] = node;
	}

	// 清理内存池
	inline void clear(void)
	{
		this->_head_node.clear();
		this->_tail_node.clear();
		this->_free_list.clear();
		_head_block->clear();
		_head_block = _tail_block = NULL;
	}
protected:
	// 申请内存块
	inline x_node_t * _create(u32_t type_size, u32_t offset_size, x_node_t * prev = NULL, x_node_t * next = NULL)
	{
		this->_free_list_mutex.enter();
		u64_t size_key = type_size + offset_size;
		if( !this->_free_list[size_key] )
		{
			this->_list_mutex.enter();
			x_block_node_t::create(this->_head_block, this->_tail_block, type_size, offset_size, _block_size);

			u64_t size_x_node_t = type_size + ptr_sizex2 + offset_size;

			char * node  = (char *)_head_block->get_data();
			node += size_x_node_t * (_block_size - 1);

			for(u32_t cnt = this->_block_size; cnt > 0;
				--cnt, node -= size_x_node_t)
			{
				// bzero(((x_node_t *)node)->get_data(), type_size);
				// bzero(((x_node_t *)node)->get_offset(type_size), offset_size);
				bzero(node, (u32_t)size_x_node_t);

				*((x_node_t *)node)->get_prev_ptr(type_size) = NULL;
				*((x_node_t *)node)->get_next_ptr(type_size) = this->_free_list[size_key];
				_free_list[size_key] = (x_node_t *)node;
			}
			this->_list_mutex.leave();
		}

		x_node_t * node = this->_free_list[size_key];
		this->_free_list[size_key] = *_free_list[size_key]->get_next_ptr(type_size);
		*node->get_prev_ptr(type_size) = prev;
		*node->get_next_ptr(type_size) = next;
		this->_free_list_mutex.leave();

		return node;
	}

protected:

	u32_t _block_size;		// 单个内存块所包含的节点(sizeof(x_node_t) + offset)数，每次申请一个内存块

	map_typeid_node_t _head_node;	// 双链表头节点
	map_typeid_node_t _tail_node;	// 双链表尾节点
	ESMT::xMutex   _head_node_mutex;	// 同步头部写数据队列
	ESMT::xMutex   _tail_node_mutex;	// 同步尾部读数据队列

	ESMT::xEvent   _readable_event; // 事件通知并发读
	ESMT::xEvent   _empty_event;	// 数据队列为空

	map_size_node_t   _free_list;		// 空闲节点链表
	ESMT::xMutex   _free_list_mutex;	// 同步空闲链表

	x_block_node_t * _head_block;	// 内存块头
	x_block_node_t * _tail_block;	// 内存块尾
	ESMT::xMutex	 _list_mutex;	// 同步块链表
public:
	inline ~xBufferEx(void)
	{
		this->clear();
	}
};

#endif // _XBUFFEREX_H_INCLUDE_
