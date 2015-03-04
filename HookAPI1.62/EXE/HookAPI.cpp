// ------------------------------------- //
// �����Ҫʹ�ñ��ļ����벻Ҫɾ����˵��  //
// ------------------------------------- //
//             HOOKAPI v1.4              //
//   Copyright 2002 ���ɳ�� Paladin     //
//       www.ProgramSalon.com            //
// ------------------------------------- //

#include <windows.h>
#include "resource.h"

#define MsgBox(msg) MessageBox(NULL, msg, "HookAPI - www.programsalon.com", MB_OK)

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam);

typedef int (WINAPI *FuncHookOneProcess2)(HWND hwndNotify, char *exe_name);
typedef int (WINAPI *FuncUnhookOneProcess2)(char *exe_name);
typedef int (WINAPI *FuncHookAllProcess)();
typedef int (WINAPI *FuncUnhookAllProcess)();

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WriteProfileString("HookAPI", "exe_name", "HookAPI.exe");

	int isNT =false;
	OSVERSIONINFO VersionInfo;
	VersionInfo.dwOSVersionInfoSize =sizeof(OSVERSIONINFO);

	if(!GetVersionEx(&VersionInfo))
		return false;

	if(VersionInfo.dwPlatformId ==VER_PLATFORM_WIN32_NT)
		isNT =true;

	HINSTANCE hLib;
	if(isNT)
		hLib =LoadLibrary("HookAPINT.dll");
	else
		hLib =LoadLibrary("HookAPI9x.dll");
	if(hLib ==NULL)
	{
		MessageBox(NULL, "LoadLibrary HookAPIxx.dll failed", "HookAPI.exe", MB_OK);
		return false;
	}

	FuncHookOneProcess2 HookOneProcess2 =(FuncHookOneProcess2)GetProcAddress(hLib, "HookOneProcess2");
	FuncUnhookOneProcess2 UnhookOneProcess2 =(FuncUnhookOneProcess2)GetProcAddress(hLib, "UnhookOneProcess2");
	FuncHookAllProcess HookAllProcess =(FuncHookAllProcess)GetProcAddress(hLib, "HookAllProcess");
	FuncUnhookAllProcess UnhookAllProcess =(FuncUnhookAllProcess)GetProcAddress(hLib, "UnhookAllProcess");

	if(HookAllProcess ==NULL)
	{
		MsgBox("HookAllProcess ==NULL");
		FreeLibrary(hLib);
		return 0;
	}
	
	if(UnhookAllProcess ==NULL)
	{
		MsgBox("UnhookAllProcess ==NULL");
		FreeLibrary(hLib);
		return 0;
	}
	if(HookAllProcess() <0)
	{
		MsgBox("HookAllProcesses error!");
		UnhookAllProcess();
		FreeLibrary(hLib);
		return 0;
	}
	// ����ֻHookһ���������еĳ�������ĺ����Ѿ���1.6�汾��ȡ���������Ǻ����ã������Ժ�汾�л��ǻ��ṩ
/*
	if(HookOneProcess2(NULL, "ship.exe") <0)
	{
		MsgBox("HookOneProcess failed!");
		return  0;
	}
*/
	/// HookAllProcesses2������޷��ػ�Ctrl-Alt_Del��ʱ���е�����������ȳ�������ʹ�õġ�
	/*if(HookAllProcesses2(1000) <0)
	{
		MsgBox("HookAllProcesses error!");

		return 0;
	}*/
	/*char temp[200], temp2[20], temp3[256];

	char *p =(char *)0x7FCEDCF1;//(char *)funcSHFileOperationA;

	wsprintf(temp, "func:%x", p);
	wsprintf(temp2, "%x,%x,%x,%x", *p&0xFF, *(p+1)&0xFF, *(p+2), *(p+3), *(p+4));
	wsprintf(temp3, "[%s], %s", temp, temp2);
	MessageBox(NULL, temp3, "ok", MB_OK);*/

	DialogBox(hInstance, MAKEINTRESOURCE(IDD_MAIN), NULL, MainDlgProc);

	UnhookAllProcess();
	FreeLibrary(hLib);

	return 0;
}

BOOL CALLBACK MainDlgProc(HWND hDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch(msg)
	{
	case WM_COMMAND:
		switch(LOWORD(wParam))
		{
		case IDOK:
			EndDialog(hDlg, IDOK);
			break;
		case ID_HIDE:
			ShowWindow(hDlg, SW_HIDE);
			break;
		}
	default:
		break;
	}

	return FALSE;
}
