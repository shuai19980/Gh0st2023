#pragma once
#include "afxdialogex.h"


// Remark 对话框

class Remark : public CDialogEx
{
	DECLARE_DYNAMIC(Remark)

public:
	Remark(CWnd* pParent = nullptr);   // 标准构造函数
	virtual ~Remark();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_DIALOG2 };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedOk();
	afx_msg void OnOnline32895();
};
