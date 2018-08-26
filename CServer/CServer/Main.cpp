// Copyright Freiyer P. Kim, all rights reserved.

#include "stdafx.h"
#include "App.h"

int WINAPI wWinMain(
	_In_		HINSTANCE	hInstance,
	_In_opt_	HINSTANCE	hPrevInstance,
	_In_		LPWSTR		lpCmdLine,
	_In_		int			nCmdShow
)
{
	// Create mutex to detect another instance
	HANDLE ghMutex;
	ghMutex = CreateMutex(
		NULL,
		FALSE,
		L"CServerMutexObject"
	);

	if (ghMutex == NULL || GetLastError() == ERROR_ALREADY_EXISTS)
		// Terminate the program
		return 1;
	
	// Get friendly name of the pc
	wchar_t pc_name_buff[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD length = sizeof(pc_name_buff) / sizeof(wchar_t);
	if (!GetComputerName(pc_name_buff, &length))
	{
		// Terminate the program
		return GetLastError();
	}

	int scrnWidth = GetSystemMetrics(SM_CXSCREEN);
	int scrnHeight = GetSystemMetrics(SM_CYSCREEN);

	// Fill the description table
	AppDesc desc;
	{
		desc.hInstance		= hInstance;
		desc.computerName	= std::wstring(pc_name_buff);
		desc.nCmdShow		= nCmdShow;
		desc.scrnNum		= 1;
		desc.scrnWidth		= &scrnWidth;
		desc.scrnHeight		= &scrnHeight;
	}

	return App::Run(&desc);
}