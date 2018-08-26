// Copyright Freiyer P. Kim, all rights reserved.

#include "stdafx.h"
#include "App.h"

#define CHECK_FUTURE(fut) \
	((fut).valid() && ((fut).wait_for(std::chrono::microseconds(1))\
	!= std::future_status::timeout))

bool App::OnCreate()
{
	_subLoopHandle = CreateThread(
		NULL,
		0,
		_SubLoop,
		this,
		NULL,
		&_subLoopId
	);
	_subLoopTerminatorFuture = _subLoopTerminator.get_future();
	return true;
}

#pragma region May_be_unused

void App::OnKeyDown(UINT8 key)
{}

void App::OnKeyUp(UINT8 key)
{}

void App::OnMouse(SHORT x, SHORT y, MouseCommands cmd)
{}

#pragma endregion

bool App::OnUpdate()
{
	// TODO
	return true;
}

bool App::OnRender()
{
	// TODO
	_pRT->BeginDraw();
	_pRT->Clear(D2D1::ColorF(0.0f, 0.2f, 0.4f));
	
	_text = _clientName + L"\n";
	
	if (_established)
	{
		_text.append(_modeName.at(_mode) + L" mode\n");
		if (_curTouch.list[0] && _prevTouch.list[0])
		{
			static auto const draw = [this](TouchPacketPtr prev, TouchPacketPtr cur) {
				if (prev && cur)
				{
					if (prev->down == 2)
						_pRT->DrawLine(
							D2D1::Point2F(prev->x / 2, prev->y / 2),
							D2D1::Point2F(cur->x / 2, cur->y / 2),
							_pBrush
						);
					else
						_pRT->DrawEllipse(
							D2D1::Ellipse(
								D2D1::Point2F(prev->x / 2, prev->y / 2),
								5.0f, 5.0f
							),
							_pBrush,
							2.0f
						);
				}
			};


			DWORD id = -1;
			HANDLE thread = CreateThread(
				NULL,
				0,
				[](LPVOID param) -> DWORD {
					App * _ = reinterpret_cast<App *>(param);
#define m _->
					for (int i = 0; i < 10; i++)
					{
						draw(m _prevTouch.list[i], m _curTouch.list[i]);
						if (!m _curTouch.list[i]) break;
						m _text
							.append(std::to_wstring(i))
							.append(L": ")
							.append(std::to_wstring(m _curTouch.list[i]->x))
							.append(L" ")
							.append(std::to_wstring(m _curTouch.list[i]->y))
							.append(L" ")
							.append(std::to_wstring(m _curTouch.list[i]->down));
					}
#undef m
					return 0;
				},
				this,
				NULL,
				&id
			);
			WaitForSingleObject(thread, 100);
			TerminateThread(thread, 1);
			if (GetExitCodeThread(thread, &id)) {
				if (id == 1) {
					OutputDebugString(L"--");
				}
			}

		}
	}
	else
		_text = L"Waiting for the client...";

	_pRT->DrawTextW(
		_text.c_str(), (UINT32)_text.size(),
		_pFormat,
		D2D1::RectF(50.f, 50.f,
		(float)(_desc->scrnWidth[_idx]), (float)(_desc->scrnHeight[_idx])
		),
		_pBrush
	);

	return SUCCEEDED(_pRT->EndDraw());;
}

void App::OnDestroy()
{
	_DestroyNetwork();
	_subLoopTerminator.set_value();
	TerminateThread(_subLoopHandle, 0);
	CloseHandle(_subLoopHandle);
	_DestroyD2D();
}

void App::_SubLoop()
{
	while (true)
	{
		// Wait until the connection is establised
		while (true)
		{
			if (CHECK_FUTURE(_subLoopTerminatorFuture))
				return;

			if (_Listen())
				if (_HandShake())
					break;

		}

		_established = true;

		// Get input and inject touch
		while (true)
		{
			if (CHECK_FUTURE(_subLoopTerminatorFuture))
				return;

			if (!_HandlePacket())
				break;
		}

		// When the connection is lost
		// initialize and wait until the connection is establised
		_established = false;
		_DestroyNetwork();
		_InitializeNetwork();
	}
}

bool App::_Listen()
{
	int iResult = listen(_socket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
		return false;

	SOCKET tmp = accept(_socket, NULL, NULL);
	if (tmp == INVALID_SOCKET)
		return false;
	
	closesocket(_socket);
	_socket = tmp;
	return true;
}

bool App::_HandShake()
{
#define CHECK(exp)				{\
	OutputDebugStringA(#exp);	\
	OutputDebugStringA("\n");	\
	if(!(exp)) return 1;}

	// Exchange device information

	// Send PC name
	StringPacketPtr send(new StringPacket(FromWideChar(_desc->computerName)));
	CHECK(_SendPacket(send));

	// Get client name
	StringPacketPtr recv;
	bool disconnected = false;
	CHECK(recv = std::dynamic_pointer_cast<StringPacket>(_NextPacket(&disconnected)));

	// Get mode
	TouchPacketPtr recv2;
	CHECK(recv2 = std::dynamic_pointer_cast<TouchPacket>(_NextPacket(&disconnected)));

	if (recv == nullptr)
		return false;

	_clientName = ToWideChar(recv->content, recv->contentSize);
	_mode = (_Mode)(recv2->idx);
	return true;
#undef CHECK
}

bool App::_HandlePacket()
{
	TouchPacketPtrList newTouch;
	if (!_GetInput(&newTouch))
		return false;

	if (!_settingsOpened)
		_InjectInput(&newTouch);
	return true;
}

bool App::_GetInput(TouchPacketPtrList * pNewTouch)
{
	auto & newTouch = *pNewTouch;

	if (_ptrTmp) newTouch.list[_ptrTmp->idx] = _ptrTmp;

	PacketPtr packet = nullptr;
	bool disconnected = false;
	bool loop = false;

	while (packet = _NextPacket(&disconnected))
	{
		loop = true;
		if (TouchPacketPtr tPacket = std::dynamic_pointer_cast<TouchPacket>(packet)
			; tPacket)
		{
			if (tPacket->idx < 10 &&
				tPacket->x < _desc->scrnWidth[_idx] && tPacket->x >= 0.0f
				&& tPacket->y < _desc->scrnHeight[_idx] && tPacket->y >= 0.0f)
			{
				if (newTouch.list[tPacket->idx])
				{
					_ptrTmp = tPacket;
					_prevTouch = _curTouch;
					_curTouch = newTouch;
					break;
				}
				newTouch.list[tPacket->idx] = tPacket;
			}
		}
	}
	if (!loop)
	{
		_prevTouch = _curTouch;
		_curTouch = TouchPacketPtrList();
	}

	if (disconnected)
		return false;
	return true;
}

bool App::_InjectInput(TouchPacketPtrList * pNewTouch)
{
	auto & newTouch = *pNewTouch;
	if (_mode == MODE_TOUCHSCREEN) {
		int i = 0;
		for (i = 0; i < 10; i++)
		{
			if (newTouch.list[i])
			{
				_contact[i].pointerInfo.ptPixelLocation.x = (LONG)newTouch.list[i]->x;
				_contact[i].pointerInfo.ptPixelLocation.y = (LONG)newTouch.list[i]->y;
				switch (newTouch.list[i]->down)
				{
				case 0: _contact[i].pointerInfo.pointerFlags
					= POINTER_FLAG_INRANGE | POINTER_FLAG_UP; break;
				case 1: _contact[i].pointerInfo.pointerFlags
					= POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_DOWN; break;
				case 2: _contact[i].pointerInfo.pointerFlags
					= POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_UPDATE; break;
				}
			}
			else
				break;
		}
		if (i == 0)
			return true;
		else
			return InjectTouchInput(i, _contact);
	}
	else {
		POINT point;
		INPUT input;

		if (!GetCursorPos(&point))
			return false;

		bool move = true;
		int i = 0, j = 0;
		for (i = 0; i < 10; i++)
			if (!newTouch.list[i]) break;
		for (j = 0; j < 10; j++)
			if (!_prevTouch.list[j]) break;
		if (i == 0)
			return true;
		else if (i == 1) {
			input.type = INPUT_MOUSE;
			input.mi.time = 0;
			input.mi.dwExtraInfo = 0;
			input.mi.mouseData = 0;
			static int downStep = 0;
			if (newTouch.list[0]->down == 1)
				downStep = 0;
			else
				downStep++;

			move = !((newTouch.list[0]->down == 0) && (downStep <= 5));

			if (move) {
				if (_prevTouch.list[0] && downStep > 5) {
					input.mi.dx = (point.x - _prevTouch.list[0]->x + newTouch.list[0]->x) * 65536 / _desc->scrnWidth[_idx];
					input.mi.dy = (point.y - _prevTouch.list[0]->y + newTouch.list[0]->y) * 65536 / _desc->scrnHeight[_idx];
				}
				else {
					input.mi.dx = point.x * 65536 / _desc->scrnWidth[_idx];
					input.mi.dy = point.y * 65536 / _desc->scrnHeight[_idx];
				}
				input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_MOVE;
				return SendInput(1, &input, sizeof(INPUT)) == 1;
			}
			else {
				downStep = 10;
				input.mi.dx = point.x * 65536 / _desc->scrnWidth[_idx];
				input.mi.dy = point.y * 65536 / _desc->scrnHeight[_idx];
				input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTDOWN;
				if (SendInput(1, &input, sizeof(INPUT)) != 1)
					return false;
				else {
					input.mi.dwFlags = MOUSEEVENTF_ABSOLUTE | MOUSEEVENTF_LEFTUP;
					return SendInput(1, &input, sizeof(INPUT)) != 1;
				}
			}
		}
		else
			return true;
			/*{
			if (i != j)
				return true;

			float dot = 0, dx = 0, dy = 0;
			for (int l = 0; l < i; l++) {
				dx += newTouch.list[i]->x - _prevTouch.list[i]->x;
				dy += newTouch.list[i]->y - _prevTouch.list[i]->y;
			}
			for (int l = 0; l < i - 1; l++) {
				dot +=
					(newTouch.list[i]->x - _prevTouch.list[i]->x)
					* (newTouch.list[i + 1]->x - _prevTouch.list[i + 1]->x)
					+ (newTouch.list[i]->y - _prevTouch.list[i]->y)
					* (newTouch.list[i + 1]->y - _prevTouch.list[i + 1]->y);
			}
			dx /= i; dy /= i;


			if (i == 2) {
				input.type = INPUT_MOUSE;
			}
			else if (i == 3) {

			}
			else if (i == 4) {
				
			}
			else
				return true;
		}*/
	}
}

PacketPtr App::_NextPacket(bool * discon)
{
	static std::map<
		unsigned, std::function<PacketPtr()>
	> const magicNumToPtr{
		{ STRING_PACKET_MAGIC_NUM, [this, discon]() -> PacketPtr {
			int len = 0;
			int res = 0;
			if ((res = recv(_socket, (char *)(&len), 4, 0)) != 4)
			{
				if (res == 0) *discon = true;
				return nullptr;
			}
			StringPacketPtr rtn(new StringPacket);
			rtn->nextInvoked = 0;
			rtn->contentSize = len;
			rtn->content = new char[len + 1];
			rtn->content[len] = 0;

			int tlen = 0, offset = 0;
			while (len && (tlen = recv(_socket,
				offset + rtn->content,
				len, 0)) != SOCKET_ERROR)
			{
				len -= tlen;
				offset += tlen;
			}

			return std::dynamic_pointer_cast<Packet>(rtn);
		}},
		{ TOUCH_PACKET_MAGIC_NUM, [this, discon]() -> PacketPtr {
			int idx = 0;
			int res = 0, down = 0;
			float x = 0.0f, y = 0.0f;
			if ((res = recv(_socket, (char *)(&idx), 4, 0)) != 4)
			{
				if (res == 0) *discon = true;
				return nullptr;
			}
			if ((res = recv(_socket, (char *)(&x), 4, 0)) != 4)
			{
				if (res == 0) *discon = true;
				return nullptr;
			}
			if ((res = recv(_socket, (char *)(&y), 4, 0)) != 4)
			{
				if (res == 0) *discon = true;
				return nullptr;
			}
			if ((res = recv(_socket, (char *)(&down), 4, 0)) != 4)
			{
				if (res == 0) *discon = true;
				return nullptr;
			}

			return std::dynamic_pointer_cast<Packet>(
				TouchPacketPtr(new TouchPacket(idx, x, y, down))
				);
		}},
	};
	int magicNumber = 0;
	int res = 0;
	if ((res = recv(_socket, (char *)(&magicNumber), 4, 0)) != 4)
	{
		if (res == 0) *discon = true;
		return nullptr;
	}

	*discon = false;

	if (auto it = magicNumToPtr.find(magicNumber); it != magicNumToPtr.end())
		return it->second();
	else
		return nullptr;
}

bool App::_SendPacket(PacketPtr packet)
{
	const char * ptr = nullptr;
	int size = 0;

	if (send(_socket,
		(const char *)&(packet->magicNumber),
		4, 0) == SOCKET_ERROR)
		return false;

	while (ptr = packet->Next(&size))
		if (send(_socket, ptr, size, 0) == SOCKET_ERROR)
			return false;

	return true;
}
