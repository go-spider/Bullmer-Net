// Client.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


#include "../xSockClient/xSockClient.h"
#include "../xConfig/xNetConfig.h"
#include "../xUtils/xUtils.h"

int _tmain(int argc, _TCHAR* argv[])
{
	char buf[][200]=
	{
		"1.你好，世界~~~~~~~~~",
		"2.hello,world !",
		"3.你到这里干嘛呢，我都不在 ？",
		"4.谢谢你提醒了我，我现在明白了，要的的啊！",
		"5.一定要记得通知我啊！，不然，嘿嘿~",
		"6.哈哈～",
		"7.是的呢!!!",
		"8.的发发呆!!!",
		"9.和团结统一!!!",
		"10.如火如荼忽然回头!!!",
		"11.研究与克里库特发热人托人涂鸦!!!",
	};
	srand((u32_t)time(NULL));

	xSockClient* sockClient[100];
	int CONN_NUM = 1;
	for (int i=0; i<CONN_NUM; ++i)
	{
		if ( NO_ERROR == xClientSocketPool::shareObject()->connect(str2ipaddr("127.0.0.1"), USER_C2S_PORT/*+i%50*/,sockClient[i]) )
		{
			printf("success...\n");
// 			int idx = rand()%(sizeof(buf)/sizeof(buf[0]));
// 			if( NO_ERROR != sockClient[i]->send(buf[idx],strlen(buf[idx])) )
// 			{
// 				//break;
// 			}
		}
		else
		{
			printf("failed...\n");
		}
		
	}

	int ii =0;
	while( 1 )
	{
// 		if ( NO_ERROR != sockClient[0]->random_regst_test() )
// 		{
// 		//	getchar();
// 		//	break;
// 		}
		
		int idx = rand()%(sizeof(buf)/sizeof(buf[0]));
		if( NO_ERROR != sockClient[rand()%CONN_NUM]->send(buf[idx],strlen(buf[idx])) )
		{
			printf("成功发送个数：%d\n",++ii);
			//break;
		}
		Sleep(10);
	}
	getchar();
	return 0;
}