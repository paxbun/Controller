// Copyright Freiyer P. Kim, all rights reserved.

#pragma once
#include "stdafx.h"
#include "Packets.h"

struct AppDesc
{
	HINSTANCE		hInstance;
	std::wstring	computerName;
	int				nCmdShow;
	int				scrnNum;
	int	*			scrnWidth;
	int	*			scrnHeight;
};

class App
{
public:
	static int Run(_In_ const AppDesc * desc);
	
	enum MouseCommands
	{
		MOUSE_UNKNOWN,
		MOUSE_LBUTTONDOWN, MOUSE_LBUTTONUP,
		MOUSE_LBUTTONDBLCLK,
		MOUSE_RBUTTONDOWN, MOUSE_RBUTTONUP,
		MOUSE_RBUTTONDBLCLK,
	};

	enum S2CCommands
	{
		S2C_INFO,
	};

	enum C2SCommands
	{
		C2S_CONNECT,
		C2S_DISCONNECT,
		C2S_TOUCH,
	};

private:
	static bool _RegisterClass();
	static bool _CreateMainWindow();
	static bool _InitializeFactories();
	static bool _CreateNotifyIcon();
	static bool _DestroyNotifyIcon();
	static bool _LoadSettings();
	static bool _GenerateSettings();
	static bool _SaveSettings();
	static bool _NewWindow();
	static void _DestroyWindow();

	static HWND					_mainWnd;
	static std::vector<App *>	_wndInstances;
	static const AppDesc *		_desc;
	static const wchar_t		_mainClassName[];
	static const wchar_t		_className[];
	static const wchar_t		_windowName[];
	static NOTIFYICONDATA		_nif;

	static LRESULT _MainWindowProc(
		_In_	HWND	hWnd,
		_In_	UINT	iMsg,
		_In_	WPARAM	wParam,
		_In_	LPARAM	lParam
	);

	static LRESULT _WindowProc(
		_In_	HWND	hWnd,
		_In_	UINT	iMsg,
		_In_	WPARAM	wParam,
		_In_	LPARAM	lParam
	);

	static DWORD _SubLoop(
		_In_	LPVOID	lpParam
	);

	static enum _Mode {
		MODE_TOUCHSCREEN = 1,
		MODE_TOUCHPAD = 2,
	};

	static ID2D1Factory		*	_pD2DFactory;
	static IDWriteFactory	*	_pDWriteFactory;
	static float				_dpiX;
	static float				_dpiY;
	static std::map<_Mode, std::wstring>
								_modeName;

private:

	App();
	~App();
	
	// Window, socket and Direct2D
	HWND	_hWnd;
	size_t	_idx;
	ID2D1HwndRenderTarget	*	_pRT;
	ID2D1SolidColorBrush	*	_pBrush;
	IDWriteTextFormat		*	_pFormat;
	SOCKET						_socket;
	std::wstring				_text;

	bool						_established;
	bool						_settingsOpened;
	std::wstring				_clientName;
	_Mode						_mode;
	

	struct TouchPacketPtrList {
		TouchPacketPtr list[10];

		TouchPacketPtrList()
		{
			for (auto & i : list)
				i = nullptr;
		}

		TouchPacketPtrList(const TouchPacketPtrList & other)
		{
			for (int i = 0; i < 10; i++)
				list[i] = other.list[i];
		}
	};
	
	TouchPacketPtrList	_prevTouch;
	TouchPacketPtrList	_curTouch;
	TouchPacketPtr		_ptrTmp;

	POINTER_TOUCH_INFO	_contact[10];

	HANDLE				_subLoopHandle;
	DWORD				_subLoopId;

	std::promise<void>	_subLoopTerminator;
	std::future<void>	_subLoopTerminatorFuture;

	bool _InitializeWindow();
	bool _InitializeNetwork();
	bool _InitializeD2D();

	void _DestroyNetwork();
	void _DestroyD2D();

public:

	bool OnCreate();
	void OnKeyDown(UINT8 key);
	void OnKeyUp(UINT8 key);
	void OnMouse(SHORT x, SHORT y, MouseCommands cmd);
	bool OnUpdate();
	bool OnRender();
	void OnDestroy();

private:

	void _SubLoop();
	bool _Listen();
	bool _HandShake();
	bool _HandlePacket();
	bool _GetInput(TouchPacketPtrList *);
	bool _InjectInput(TouchPacketPtrList *);
	PacketPtr _NextPacket(bool *);
	bool _SendPacket(PacketPtr packet);
};

#define CSERVER_NOTIFY	WM_USER + 1
#define CSERVER_EXIT	WM_USER + 2
#define CSERVER_OPEN	WM_USER + 3

#define CSERVER_PORT	"19754"

template<typename T>
inline void SafeRelease(T ** intf)
{
	if (*intf)
	{
		(*intf)->Release();
		*intf = NULL;
	}
}

std::wstring ToWideChar(const char * str, int len);
std::wstring ToWideChar(const std::string & str);
std::string FromWideChar(const wchar_t * str, int len);
std::string FromWideChar(const std::wstring & str);