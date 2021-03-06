// CServer.cpp : Defines the entry point for the application.
//

#include "stdafx.h"
#include "CServer.h"
#define CLASS_NAME L"Application"
#define WNDOW_NAME L"Controller Server"

LRESULT WINAPI WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam);

int APIENTRY wWinMain(
	_In_		HINSTANCE	hInstance,
	_In_opt_	HINSTANCE	hPrevInstance,
	_In_		LPWSTR		lpCmdLine,
	_In_		int			nCmdShow
)
{
	wchar_t my_buffer[MAX_COMPUTERNAME_LENGTH + 1];
	DWORD length;
	GetComputerName(my_buffer, &length);

	WNDCLASSEX wcex = { 0 };
	{
		wcex.cbSize			= sizeof(wcex);
		wcex.style			= CS_HREDRAW | CS_VREDRAW;
		wcex.lpfnWndProc	= WndProc;
		wcex.cbClsExtra		= 0;
		wcex.cbWndExtra		= 0;
		wcex.hInstance		= hInstance;
		wcex.hIcon			= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_CSERVER));
		wcex.hCursor		= LoadCursor(NULL, IDC_ARROW);
		wcex.hbrBackground	= (HBRUSH)GetStockObject(WHITE_BRUSH);
		wcex.lpszMenuName	= NULL;
		wcex.lpszClassName	= CLASS_NAME;
		wcex.hIconSm		= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
	}
	RegisterClassEx(&wcex);

	HWND hWnd = CreateWindow(CLASS_NAME, WNDOW_NAME, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hInstance, NULL);
	
	NOTIFYICONDATA nif = { 0 };
	{
		nif.cbSize				= sizeof(nif);
		nif.hWnd				= hWnd;
		nif.uID					= 200;
		nif.uVersion			= NOTIFYICON_VERSION;
		nif.uCallbackMessage	= WM_USER + 1;
		nif.hIcon				= LoadIcon(hInstance, MAKEINTRESOURCE(IDI_SMALL));
		wcscpy_s(nif.szTip, L"CServer");
		nif.uFlags				= NIF_MESSAGE | NIF_ICON | NIF_TIP;
	}

	Shell_NotifyIcon(NIM_ADD, &nif);
	
	// ShowWindow(hWnd, nCmdShow);

	MSG msg = { 0 };
	while (GetMessage(&msg, NULL, NULL, NULL))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	Shell_NotifyIcon(NIM_DELETE, &nif);

	DEVMODE devmode;

	devmode.dmPelsWidth = 1920;
	devmode.dmPelsHeight = 1080;
	devmode.dmFields = DM_PELSWIDTH | DM_PELSHEIGHT;
	devmode.dmSize = sizeof(DEVMODE);

	long result = ChangeDisplaySettings(&devmode, 0);

	int width = GetSystemMetrics(SM_CXscrn);
	int height = GetSystemMetrics(SM_CYscrn);


	POINTER_TOUCH_INFO contact;
	BOOL bRet = TRUE;

	//
	// assume a maximum of 10 contacts and turn touch feedback off
	//
	InitializeTouchInjection(10, TOUCH_FEEDBACK_NONE);

	//
	// initialize the touch info structure
	//
	memset(&contact, 0, sizeof(POINTER_TOUCH_INFO));

	contact.pointerInfo.pointerType = PT_TOUCH; //we're sending touch input
	contact.pointerInfo.pointerId = 0;          //contact 0

	contact.pointerInfo.ptPixelLocation.x = 640;
	contact.pointerInfo.ptPixelLocation.y = 480;
	contact.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_UPDATE;
	contact.touchFlags = TOUCH_FLAG_NONE;
	contact.touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
	contact.orientation = 0;
	contact.pressure = 32000;

	//
	// set the contact area depending on thickness
	//
	contact.rcContact.top = 0;
	contact.rcContact.bottom = 1080;
	contact.rcContact.left = 0;
	contact.rcContact.right = 1920;

	//
	// inject a touch down
	//
	bRet = InjectTouchInput(1, &contact);

	//
	// if touch down was successfull, send a touch up
	//
	if (bRet) {
		for (clock_t c = clock(), t = clock(); c < t + 5000; c = clock()) {
			while (clock() < c + 1000 / 75);
			contact.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_UPDATE;
			contact.pointerInfo.ptPixelLocation.y = 480 + (double)480 * c / 5000;
			contact.pointerInfo.ptPixelLocation.x = 480 - (double)240 * c / 5000;
			bRet = InjectTouchInput(1, &contact);
			auto i = GetLastError();
		}

		contact.pointerInfo.pointerFlags = POINTER_FLAG_UP;
		InjectTouchInput(1, &contact);
		MessageBox(NULL, L"S", L"", MB_OK);
	}

	return 0;
}

LRESULT WINAPI WndProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_CLOSE:
		PostQuitMessage(0);
		return 0;
	case WM_USER+1:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			ShowWindow(hWnd, SW_SHOW);
			break;
		case WM_RBUTTONDBLCLK:
		{
			HMENU hPopupMenu = CreatePopupMenu();
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, 1, L"Exit");
			InsertMenu(hPopupMenu, 0, MF_BYPOSITION | MF_STRING, 2, L"Play");
			TrackPopupMenu(hPopupMenu, TPM_BOTTOMALIGN | TPM_LEFTALIGN, 0, 0, 0, NULL, NULL);
		}
			break;
		default:
			break;
		}
		return 0;
	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}