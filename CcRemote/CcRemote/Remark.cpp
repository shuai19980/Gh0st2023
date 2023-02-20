// Remark.cpp: 实现文件
//

#include "pch.h"
#include "CcRemote.h"
#include "afxdialogex.h"
#include "Remark.h"
#include "CcRemoteDlg.h"


// Remark 对话框

IMPLEMENT_DYNAMIC(Remark, CDialogEx)

Remark::Remark(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_DIALOG2, pParent)
{

}

Remark::~Remark()
{
}

void Remark::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}


BEGIN_MESSAGE_MAP(Remark, CDialogEx)
	ON_BN_CLICKED(IDOK, &Remark::OnBnClickedOk)
END_MESSAGE_MAP()


// Remark 消息处理程序


void Remark::OnBnClickedOk()
{
	// TODO: 在此添加控件通知处理程序代码
	CEdit* premark;
	CString sztmpRemark;
	premark = (CEdit*)GetDlgItem(IDC_EDIT1);
	premark->GetWindowTextA(sztmpRemark);
	CCcRemoteDlg* par = (CCcRemoteDlg*)GetParent();
	par->ChangeRemark(sztmpRemark);
	int rt;
	EndDialog(rt);
}
