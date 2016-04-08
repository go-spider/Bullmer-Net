/*
* @note		：服务器参数配置
* @author	：Andy.Ro
* @email	：Qwuloo@qq.com
* @date		：2014-06
*/
#ifndef _XNET_CONFIG_H_INCLUDE_
#define _XNET_CONFIG_H_INCLUDE_

#define localhost 127.0.0.1


// 登录服监听客户端连接端口范围
#define LOGIN_C2L_PORT			5000
#define LOGIN_C2L_PORT_SCOPE	1000
// 登录服连接中央服ip地址及端口(作为客户端)
#define LOGIN_L2M_ADDR			"127.0.0.1"
#define LOGIN_L2M_PORT			MAST_P2M_PORT
#define LOGIN_L2M_PORT_SCOPE	MAST_P2M_PORT_SCOPE

// 用户服监听客户端连接端口范围
#define USER_C2S_PORT			7000
#define USER_C2S_PORT_SCOPE		1
// 用户服连接平台服ip地址及端口(作为客户端)
#define USER_S2P_ADDR			"127.0.0.1"
#define USER_S2P_PORT			PLAT_S2P_PORT
#define USER_S2P_PORT_SCOPE		PLAT_S2P_PORT_SCOPE

// 平台服监听用户服连接端口范围
#define PLAT_S2P_PORT			8000
#define PLAT_S2P_PORT_SCOPE		1
// 平台服连接中央服ip地址及端口(作为客户端)
#define PLAT_P2M_ADDR			"127.0.0.1"
#define PLAT_P2M_PORT			MAST_P2M_PORT
#define PLAT_P2M_PORT_SCOPE		MAST_P2M_PORT_SCOPE

// 中央服监听平台服连接端口范围
#define MAST_P2M_PORT			9000
#define MAST_P2M_PORT_SCOPE		1

#endif // _XNET_CONFIG_H_INCLUDE_