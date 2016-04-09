/*
* @note		：数据包协议结构
*
* @mark		：数据包收发流程：
*				发送端：拆包/分包：将整体数据拆分成一个或多个数据包(数据套上包头) -> 依次通知发送回调(单包发送)
*				接收端：并发接收 -> 粘包分离，缝包(前后半包拼接) -> 拼包/组包：去掉各数据包包头并拼接整体数据 -> 通知接收回调(整体接收)
*
*/

#ifndef _XPACKET_H_INCLUDE_
#define _XPACKET_H_INCLUDE_

#include "../xBuffer/xBuffer_p.h"

#pragma pack(push,1)

//------------------------------------------------------------------------
// packet_t 数据包协议结构，发送端和接收端协议必须完全一致
//------------------------------------------------------------------------
typedef struct _packet_t
{
#define FMT_RAW		0
#define FMT_JSON	1
#define FMT_XML		2
#define FMT_TEXT	3
	//////////////////////////////////////////////////////////////////////////
	// 包头(header)

	// 0  ~ 1 位：包体(数据部分)格式(0：raw|1：json|2：xml|3：text)，相对整体而言，各数据包包体格式肯定是一致的
	// 2      位：数据包开始符(0：非开始 1：开始)，相对整体而言，当前包是否是第一个包，即首包
	// 3      位：数据包结束符(0：非结束 1：结束)，相对整体而言，当前包是否是最后个包，即尾包
	// 4  ~ 31位：所在整体所含数据包总数(total)
	// 32 ~ 47位：当前包体(数据部分)大小(len )，即追加缓存部分存放的实际数据长度，不超过 MAX_PACKETSZ
	// 48 ~ 63位：尾包包体(数据部分)大小(tail)，不超过 MAX_PACKETSZ
	u64_t		flag;					// 不论是网络或其它类型数据接收都是以基本类型(字节，整型，浮点)为单位的，
										// 尽量将包头信息集中在一个类型变量上，并且作为包头首成员，提高接收可靠性，
										// 分散在不同成员上不安全，可能只收到部分成员数据
	
	u64_t		gid;					// 发送端全局唯一标识各数据包发送序列且连续(编号1开始)，接收端处理并发接收问题
	
	u32_t		pid;					// 唯一标识数据包所在整体(编号1开始)，即发送数据时将一个整体分拆成多个数据包的 pid 相同
	
	u32_t		id;						// 相对整体而言，其相关各数据包间的序列id唯一且连续(编号1开始)，拼包后检验其连续性(针对 UDP 不可靠传输来说，完全漏掉某个包有可能的)
	
	u64_t		offset;					// 当前包体(数据部分)相对整体的偏移
	
	// 发送时间戳
	time_t		ts;
	
	// 校验和
	u16_t		checksum;
	
	//////////////////////////////////////////////////////////////////////////
	// 包体(body-content)，即数据部分		// 追加缓存部分，首址为 (char *)this + sizeof(packet_header_t)

	// 对于小数据量(总数据大小不超过MAX_PACKETSZ，total=1)而言，发送端无需分包发送，接收端无需拼包，一个数据包即可表示整体：len
	// 对于大数据量(总数据大小已超过MAX_PACKETSZ，total>1)而言，发送端需要分包发送，接收端需要拼包，由各个数据包拼接成整体：(total - 1) * len(MAX_PACKETSZ) + tail

}packet_t;

#pragma pack(pop)

// 直观看其实就是数据包头结构
typedef packet_t packet_header_t;

#define MAX_BUF_SIZE	1024// 一次数据收发最大缓存大小(自定义任意大小，但不能超过 65535 + sizeof(packet_header_t))
#define MAX_PACKETSZ   ((MAX_BUF_SIZE - sizeof(packet_header_t)) >= 65535 ? 65535:(MAX_BUF_SIZE - sizeof(packet_header_t)) ) // 单数据包包体大小不超过65535

// 获取数据包格式/开始符/结束符/总包数/包体大小/尾包包体大小
#define __packet_get_type(flag)		(u8_t )(((flag) & 0x0000000000000003)      )
#define __packet_get_start(flag)	(u8_t )(((flag) & 0x0000000000000004) >> 2 )
#define	__packet_get_end(flag)		(u8_t )(((flag) & 0x0000000000000008) >> 3 )
#define __packet_get_total(flag)	(u32_t)(((flag) & 0x00000000FFFFFFF0) >> 4 )
#define __packet_get_len(flag)		(u16_t)(((flag) & 0x0000FFFF00000000) >> 32)
#define __packet_get_tail(flag)     (u16_t)(((flag) & 0xFFFF000000000000) >> 48)

// 指定数据包格式/开始符/结束符/总包数/包体大小/尾包包体大小
#define __packet_set_type( flag, t)	((flag) = ( ((flag) & 0xFFFFFFFFFFFFFFFC) | (( (u8_t )(t))        & 0x0000000000000003) ))
#define __packet_set_start(flag, b)	((flag) = ( ((flag) & 0xFFFFFFFFFFFFFFFB) | ((((u8_t )(b)) <<  2) & 0x0000000000000004) ))
#define	__packet_set_end(  flag, b)	((flag) = ( ((flag) & 0xFFFFFFFFFFFFFFF7) | ((((u8_t )(b)) <<  3) & 0x0000000000000008) ))
#define __packet_set_total(flag, n)	((flag) = ( ((flag) & 0xFFFFFFFF0000000F) | ((((u32_t)(n)) <<  4) & 0x00000000FFFFFFF0) ))
#define __packet_set_len(  flag, l)	((flag) = ( ((flag) & 0xFFFF0000FFFFFFFF) | ((((u64_t)(l)) << 32) & 0x0000FFFF00000000) ))
#define __packet_set_tail( flag, l)	((flag) = ( ((flag) & 0x0000FFFFFFFFFFFF) | ((((u64_t)(l)) << 48) & 0xFFFF000000000000) ))

#define __packet_data_addr(p) (char *)((char *)(p) + sizeof(packet_header_t))

typedef xBuffer_p packet_pool_t;
typedef std::map<u64_t,std::pair<char*,u32_t>> map_packet_t;
typedef map_packet_t::iterator				  itor_packet_t;

// ******************************************* 应用层开放调用接口 *******************************************

/*
**
#ifdef __cplusplus
extern "C" {
#endif

// @note：通知接收回调函数
typedef int (*recv_callback_func)(xNetObject* pthis, char const* buf, u64_t bytes);

// @note：通知发送回调函数
typedef int (*send_callback_func)(xNetObject* pthis, char const* buf, ulong_t bytes);

// @note：拆包：将一个数据整体拆分成一个或多个数据包(数据块套上包头)后通知用户回调发送
// @mark：发送端：拆包/分包：将整体数据拆分成一个或多个数据包(数据套上包头) -> 依次通知发送回调(单包发送)
XNET_API int split_packet(char const* buf, u64_t bytes,
						send_callback_func callback_send,
						xNetObject* pthis);

// @note：粘包分离，缝包(前后半包拼接)
// @mark：接收端：并发接收 -> 粘包分离，缝包(前后半包拼接) -> 拼包/组包：去掉各数据包包头并拼接整体数据 -> 通知接收回调(整体接收)
XNET_API int separate_continuous_packet(char const* buf, ulong_t bytes,
									  recv_callback_func callback_recv,
									  xNetObject* pthis);

#ifdef __cplusplus
}
#endif
**
*/

#endif // _XPACKjoint_packetET_H_INCLUDE_
