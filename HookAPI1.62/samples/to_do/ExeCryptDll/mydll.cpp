// ------------------------------------- //
// �����Ҫʹ�ñ��ļ����벻Ҫɾ����˵��  //
// ------------------------------------- //
//             HOOKAPI ��������          //
//   Copyright 2002 ���ɳ�� Paladin     //
//       www.ProgramSalon.com            //
// ------------------------------------- //

#include "stdafx.h"
#include <stdio.h>

#include "mydll.h"
#include "util.h"
#include "psapi.h"
#include "tlhelp32.h"
#include "ps.h"

HINSTANCE g_hInstance =NULL;

BOOL APIENTRY DllMain( HANDLE hModule, 
                       DWORD  ul_reason_for_call, 
                       LPVOID lpReserved
					 )
{
	g_hInstance =(HINSTANCE)hModule;

    return TRUE;
}

// �ػ�ִ��notepad.exe�ļ������ӣ���explorer�е��winnt\notepad.exe��Ȼ��
// ʹ������������鿴���̣��ͷ����и�notepad2.exe����

DWORD WINAPI myCreateProcessW(
	LPCWSTR lpApplicationName,
	LPWSTR lpCommandLine, 
	LPSECURITY_ATTRIBUTES lpProcessAttributes,
	LPSECURITY_ATTRIBUTES lpThreadAttributes,
	BOOL bInheritHandles,
	DWORD dwCreationFlags,
	LPVOID lpEnvironment,
	LPCWSTR lpCurrentDirectory,
	LPSTARTUPINFOW lpStartupInfo,
	LPPROCESS_INFORMATION lpProcessInformation
)
{
	char cmd[600];
	int len =WideCharToMultiByte( CP_ACP, 0, lpCommandLine, -1, cmd, sizeof(cmd),NULL,NULL); 
	cmd[len] =0;

	WriteLog("CreateProcessW :cmd=%s", cmd);
	strtok(cmd, " ");  //ȥ���ո�
	strupr(cmd);
	if(cmd[0] =='\"')  // ȥ������
	{
		memmove(cmd, &cmd[1], strlen(cmd)-1);
		cmd[strlen(cmd)-2] =0;
	}

	if(strstr(cmd, "NOTEPAD.EXE"))  // �����ļ��������λ��c:\notepad2.exe
	{
		WriteLog("found ok, cmd=%s", cmd);
		CopyFile(cmd, "c:\\notepad2.exe", 0);  //�����ڴ˴�����notepad.exeΪnotepad2.exe
		strcpy(cmd, "c:\\notepad2.exe");
	}
	WCHAR wcmd[600];
	len =MultiByteToWideChar( CP_ACP, 0, cmd, strlen(cmd), wcmd, sizeof(wcmd)); 
	wcmd[len] =0;

	BOOL ifsuccess = CreateProcessW(NULL/*lpApplicationName*/,
		wcmd, lpProcessAttributes,
		lpThreadAttributes, bInheritHandles, dwCreationFlags, lpEnvironment,
		lpCurrentDirectory, lpStartupInfo, lpProcessInformation);
	DWORD err =GetLastError();
	SetLastError(err);
	return (DWORD)ifsuccess;
}

MYAPIINFO myapi_info[] =
{
	{"KERNEL32.DLL", "CreateProcessW(1,2,3,4,5,6,7,8,9,10)", "myCreateProcessW"},
	{NULL,NULL,NULL}
};

MYAPIINFO *GetMyAPIInfo()
{
	return &myapi_info[0];
}
