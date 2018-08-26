// Copyright Freiyer P. Kim, all rights reserved.

#include "stdafx.h"
#include "App.h"

HWND				App::_mainWnd			= NULL;
std::vector<App *>	App::_wndInstances		= std::vector<App *>();
const AppDesc	*	App::_desc				= nullptr;
const wchar_t		App::_mainClassName[]	= L"CServerMainWindowClass";
const wchar_t		App::_className[]		= L"CServerWindowClass";
const wchar_t		App::_windowName[]		= L"CServer";
NOTIFYICONDATA		App::_nif				= { 0 };
float				App::_dpiX				= 0.0f;
float				App::_dpiY				= 0.0f;
std::map<App::_Mode, std::wstring>
					App::_modeName			= {
	{ App::MODE_TOUCHSCREEN,	L"Touchscreen"	},
	{ App::MODE_TOUCHPAD,		L"Touchpad"		},
};

ID2D1Factory	*	App::_pD2DFactory		= nullptr;
IDWriteFactory	*	App::_pDWriteFactory	= nullptr;

int App::Run(_In_ const AppDesc * desc)
{
#define CHECK(exp)				{\
	OutputDebugStringA(#exp);	\
	OutputDebugStringA("\n");	\
	if(!(exp)) return 1;}

	_desc = desc;

	CHECK(_RegisterClass());
	CHECK(_CreateMainWindow());
	CHECK(_InitializeFactories());
	CHECK(_CreateNotifyIcon());
	CHECK(_LoadSettings() || _GenerateSettings());
	CHECK(InitializeTouchInjection(10, TOUCH_FEEDBACK_NONE));

	// Create a new window in default, make it possible to add multiple windows for multiple scrns.
	// Multiple scrns are not implemented yet, though.
	CHECK(_NewWindow());

	MSG msg = { 0 };
	while (!_wndInstances.empty())
	{
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

	CHECK(_SaveSettings());
	CHECK(_DestroyNotifyIcon());
	return 0;

#undef CHECK
}

bool App::_RegisterClass()
{
	WNDCLASSEX			_wcex;
	_wcex.cbSize = sizeof(_wcex);
	_wcex.style = CS_HREDRAW | CS_VREDRAW;
	_wcex.lpfnWndProc = _MainWindowProc;
	_wcex.cbClsExtra = 0;
	_wcex.cbWndExtra = 0;
	_wcex.hInstance = _desc->hInstance;
	_wcex.hIcon = LoadIcon(_desc->hInstance, MAKEINTRESOURCE(IDI_CSERVER));
	_wcex.hCursor = LoadCursor(NULL, IDC_ARROW);
	_wcex.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	_wcex.lpszMenuName = NULL;
	_wcex.lpszClassName = _mainClassName;
	_wcex.hIconSm = LoadIcon(_desc->hInstance, MAKEINTRESOURCE(IDI_SMALL));
	if (!RegisterClassEx(&_wcex))
		return false;

	_wcex.lpfnWndProc = _WindowProc;
	_wcex.lpszClassName = _className;
	if (!RegisterClassEx(&_wcex))
		return false;

	return true;
}

bool App::_CreateMainWindow()
{
	_mainWnd = CreateWindow(
		_mainClassName, _windowName,
		WS_OVERLAPPEDWINDOW, 0, 0, 0, 0,
		NULL, NULL, _desc->hInstance, NULL
	);
	ShowWindow(_mainWnd, SW_HIDE);
	return _mainWnd != NULL;
}

bool App::_InitializeFactories()
{
#define CHECK(exp)				{\
	OutputDebugStringA(#exp);	\
	OutputDebugStringA("");		\
	if(FAILED(exp))				\
		return false;			}

	CHECK(D2D1CreateFactory(
		D2D1_FACTORY_TYPE_MULTI_THREADED,
		&_pD2DFactory
	));
	_pD2DFactory->GetDesktopDpi(&_dpiX, &_dpiY);

	CHECK(DWriteCreateFactory(
		DWRITE_FACTORY_TYPE_SHARED,
		__uuidof(_pDWriteFactory),
		reinterpret_cast<IUnknown **>(&_pDWriteFactory)
	));
	
	return true;

#undef CHECK
}

bool App::_CreateNotifyIcon()
{
	_nif.cbSize = sizeof(_nif);
	_nif.hWnd = _mainWnd;
	_nif.uID = 200;
	_nif.uVersion = NOTIFYICON_VERSION;
	_nif.uCallbackMessage = CSERVER_NOTIFY;
	_nif.hIcon = LoadIcon(_desc->hInstance, MAKEINTRESOURCE(IDI_SMALL));
	wcscpy_s(_nif.szTip, L"CServer");
	_nif.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
	return Shell_NotifyIcon(NIM_ADD, &_nif);
}

bool App::_DestroyNotifyIcon()
{
	return Shell_NotifyIcon(NIM_DELETE, &_nif);
}

bool App::_LoadSettings()
{
	// TODO
	return true;
}

bool App::_GenerateSettings()
{
	// TODO
	return true;
}

bool App::_SaveSettings()
{
	// TODO
	return true;
}

bool App::_NewWindow()
{
	if (_wndInstances.size() == _desc->scrnNum)
		return false;

	auto app = new App;
	if (!(app->_InitializeNetwork()
		&& app->_InitializeWindow()
		&& app->_InitializeD2D()))
	{
		delete app;
		return false;
	}
	app->OnCreate();
	_wndInstances.push_back(app);
	return true;
}

void App::_DestroyWindow()
{
	for (auto & i : _wndInstances)
	{
		if (i == nullptr)
			continue;
		i->OnDestroy();
		DestroyWindow(i->_hWnd);
		delete i;
		i = nullptr;
	}
	_wndInstances.clear();
}

LRESULT App::_MainWindowProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	switch (iMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		return 1;
	case CSERVER_NOTIFY:
		switch (lParam)
		{
		case WM_LBUTTONDBLCLK:
			_wndInstances.front()->_settingsOpened = true;
			ShowWindow(_wndInstances.front()->_hWnd, SW_SHOW);
			SetFocus(_wndInstances.front()->_hWnd);
			_DestroyNotifyIcon();
			break;
		case WM_RBUTTONUP:
			{
				HMENU hPopupMenu = CreatePopupMenu();
				AppendMenu(hPopupMenu, MF_STRING, CSERVER_EXIT, L"Shutdown Server");
				AppendMenu(hPopupMenu, MF_STRING, CSERVER_OPEN, L"Open Settings");
				POINT pCursor;
				GetCursorPos(&pCursor);
				TrackPopupMenu(hPopupMenu, TPM_LEFTBUTTON | TPM_RIGHTALIGN,
					pCursor.x, pCursor.y, 0, hWnd, NULL);
			}
			break;
		}
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case CSERVER_EXIT:
			if (MessageBox(NULL,
				L"Are you sure to shutdown?", L"Alert",
				MB_YESNO | MB_ICONQUESTION) == IDYES)
				_DestroyWindow();
			break;
		case CSERVER_OPEN:
			_wndInstances.front()->_settingsOpened = true;
			ShowWindow(_wndInstances.front()->_hWnd, SW_SHOW);
			_DestroyNotifyIcon();
			break;
		}
		return 0;
	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

LRESULT App::_WindowProc(HWND hWnd, UINT iMsg, WPARAM wParam, LPARAM lParam)
{
	App * app = reinterpret_cast<App *>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	HMENU menu = GetMenu(hWnd);

	// Check mouse input
	static auto const iMsg2Mouse = [](UINT iMsg) -> MouseCommands { switch (iMsg) {
#define CASE(name) case WM_##name: return MOUSE_##name

		CASE(LBUTTONUP); CASE(LBUTTONDOWN); CASE(LBUTTONDBLCLK);
		CASE(RBUTTONUP); CASE(RBUTTONDOWN); CASE(RBUTTONDBLCLK);

#undef CASE	
	} return MOUSE_UNKNOWN; };
	if (auto cmd = iMsg2Mouse(iMsg); app && cmd != MOUSE_UNKNOWN)
		app->OnMouse(
			GET_X_LPARAM(lParam),
			GET_Y_LPARAM(lParam),
			iMsg2Mouse(iMsg)
		);

	// Handle other events
	switch (iMsg)
	{
	case WM_CREATE:
		SetWindowLongPtr(hWnd, GWLP_USERDATA,
			reinterpret_cast<LONG_PTR>(
				reinterpret_cast<LPCREATESTRUCT>(lParam)
					->lpCreateParams
			)
		);
		EnableMenuItem(menu, ID_SETTINGS_TOUCHSCREENMODE, MF_DISABLED);
		EnableMenuItem(menu, ID_SETTINGS_TOUCHPADMODE, MF_DISABLED);
		EnableMenuItem(menu, ID_SETTINGS_GAMEPADMODE, MF_DISABLED);
		return 0;
	case WM_ERASEBKGND:
		return 1;
	case WM_KEYDOWN:
		if (app)
			app->OnKeyDown(static_cast<UINT8>(wParam));
		return 0;
	case WM_KEYUP:
		if (app)
			app->OnKeyUp(static_cast<UINT8>(wParam));
		return 0;
	case WM_SIZE:
		
		return 0;
	case WM_PAINT:
		if (app)
		{
			if (!app->OnUpdate())
				_DestroyWindow();
			if (!app->OnRender())
				_DestroyWindow();
		}
		return 0;
	case WM_CLOSE:
		ShowWindow(hWnd, SW_HIDE);
		app->_settingsOpened = false;
		_CreateNotifyIcon();
		return 1;
	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		
		}
		return 0;
	}
	return DefWindowProc(hWnd, iMsg, wParam, lParam);
}

DWORD App::_SubLoop(LPVOID lpParam)
{
	if (lpParam) {
		reinterpret_cast<App *>(lpParam)->_SubLoop();
	}
	return 0;
}

App::App()
	: _hWnd(NULL), _idx(_wndInstances.size()),
	_pRT(nullptr), _pBrush(nullptr), _pFormat(nullptr),
	_socket(INVALID_SOCKET), _settingsOpened(false)
{
	memset(_contact, 0, sizeof(POINTER_TOUCH_INFO) * 10);
	for (int i = 0; i < 10; i++)
	{
		_contact[i].pointerInfo.pointerType = PT_TOUCH;
		_contact[i].pointerInfo.pointerId = i;
		_contact[i].touchFlags = TOUCH_FLAG_NONE;
		_contact[i].touchMask = TOUCH_MASK_CONTACTAREA | TOUCH_MASK_ORIENTATION | TOUCH_MASK_PRESSURE;
		_contact[i].orientation = 0;
		_contact[i].pressure = 32000;
		_contact[i].rcContact.top = 0;
		_contact[i].rcContact.bottom = _desc->scrnHeight[_idx];
		_contact[i].rcContact.left = 0;
		_contact[i].rcContact.right = _desc->scrnWidth[_idx];
	}
}

App::~App()
{
}

bool App::_InitializeWindow()
{
	RECT rect;
	{
		rect.left = 0;
		rect.top = 0;
		rect.right = _desc->scrnWidth[_idx] / 2;
		rect.bottom = _desc->scrnHeight[_idx] / 2;
	}
	AdjustWindowRect(&rect, WS_OVERLAPPEDWINDOW, TRUE);

	_hWnd = CreateWindowEx(
		NULL,
		_className, _windowName, WS_OVERLAPPED | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		rect.right - rect.left,
		rect.bottom - rect.top,
		NULL,
		LoadMenu(NULL, MAKEINTRESOURCE(IDR_MENU)),
		_desc->hInstance, this
	);
	ShowWindow(_hWnd, SW_HIDE);
	return _hWnd != NULL;
}

bool App::_InitializeNetwork()
{
#define CHECK(exp, dsr, act)		\
	{								\
		OutputDebugStringA(#exp);	\
		OutputDebugStringA("\n");	\
		if((exp) dsr) {				\
			act; return false;		\
		}							\
	}								\

	WSADATA wsaData;

	addrinfo * result = NULL;
	addrinfo hints;

	CHECK(WSAStartup(MAKEWORD(2, 2), &wsaData), != 0, {});

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;
	
	CHECK(
		getaddrinfo(NULL, CSERVER_PORT, &hints, &result),
		!= 0, { WSACleanup(); }
	);
	
	CHECK(
		_socket = socket(result->ai_family, result->ai_socktype, result->ai_protocol),
		== INVALID_SOCKET, { freeaddrinfo(result); WSACleanup(); }
	);

	CHECK(
		bind(_socket, result->ai_addr, (int)result->ai_addrlen),
		== SOCKET_ERROR, { freeaddrinfo(result); closesocket(_socket); WSACleanup(); }
	);

	freeaddrinfo(result);
	
	return true;

#undef CHECK
}

bool App::_InitializeD2D()
{
#define CHECK(exp)				{\
	OutputDebugStringA(#exp);	\
	OutputDebugStringA("");		\
	if(FAILED(exp))				\
		return false;			}
	
	CHECK(_pD2DFactory->CreateHwndRenderTarget(
		D2D1::RenderTargetProperties(),
		D2D1::HwndRenderTargetProperties(
			_hWnd,
			D2D1::SizeU(
				(UINT32)(_desc->scrnWidth[_idx] / 2 * _dpiX / 96.0f),
				(UINT32)(_desc->scrnHeight[_idx] / 2 * _dpiY / 96.0f)
			)
		),
		&_pRT
	));

	CHECK(_pRT->CreateSolidColorBrush(
		D2D1::ColorF(1.0f, 1.0f, 1.0f),
		&_pBrush
	));

	CHECK(_pDWriteFactory->CreateTextFormat(
		L"Segoe UI Light",
		NULL,
		DWRITE_FONT_WEIGHT_NORMAL,
		DWRITE_FONT_STYLE_NORMAL,
		DWRITE_FONT_STRETCH_NORMAL,
		16,
		L"",	//Locale
		&_pFormat
	));

	return true;

#undef CHECK
}

void App::_DestroyNetwork()
{
	if (_socket != INVALID_SOCKET)
		closesocket(_socket);
	WSACleanup();
}

void App::_DestroyD2D()
{
	SafeRelease(&_pFormat);
	SafeRelease(&_pDWriteFactory);
	SafeRelease(&_pBrush);
	SafeRelease(&_pRT);
	SafeRelease(&_pD2DFactory);
}

std::wstring ToWideChar(const char * str, int len)
{
	if (len == 0)
		return std::wstring();
	int num = MultiByteToWideChar(
		CP_UTF8, 0, str, len, NULL, 0
	);
	std::wstring rtn(num, 0);
	MultiByteToWideChar(
		CP_UTF8, 0, str, len, &rtn[0], num
	);
	return rtn;
}

std::wstring ToWideChar(const std::string & str)
{
	return ToWideChar(str.c_str(), (int)str.size());
}

std::string FromWideChar(const wchar_t * str, int len)
{
	if (len == 0)
		return std::string();
	int num = WideCharToMultiByte(
		CP_UTF8, 0, str, len, NULL, 0, NULL, NULL
	);
	std::string rtn(num, 0);
	WideCharToMultiByte(
		CP_UTF8, 0, str, len, &rtn[0], num, NULL, NULL
	);
	return rtn;
}

std::string FromWideChar(const std::wstring & str)
{
	return FromWideChar(str.c_str(), (int)str.size());
}
