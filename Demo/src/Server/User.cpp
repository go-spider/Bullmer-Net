#include "stdafx.h"

#include "../../../xNet/xMacro.h"
#include "../../../User/UserServer.h"
#include "../../../xConfig/xNetConfig.h"

int _tmain(int argc, _TCHAR* argv[])
{
	printf("------- 用户服 --------\n");

	// 监听客户端
	C2S_Server C2SService;
	xReactor::listenTCP(&C2SService, USER_C2S_PORT, USER_C2S_PORT_SCOPE);
	
	// 连接平台服
	S2P_Server S2PService;
	xReactor::connectTCP(&S2PService, USER_S2P_ADDR, USER_S2P_PORT);

	xReactor::poll(1);
	
	return 0;
}
