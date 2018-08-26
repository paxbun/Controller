#pragma once

#include <string>
#include <cstring>
#include <memory>

struct Packet
{
	int magicNumber;
	virtual const char * Next(int * size) = 0;

	Packet(unsigned magicNumber)
		: magicNumber(magicNumber)
	{ }
};

typedef std::shared_ptr<Packet> PacketPtr;

#define STRING_PACKET_MAGIC_NUM 0x53545247

struct StringPacket : public Packet
{
	char	*	content;
	int			contentSize;
	int			nextInvoked;

	StringPacket()
		: Packet(STRING_PACKET_MAGIC_NUM), content(nullptr), contentSize(0),
		nextInvoked(0)
	{ }

	StringPacket(const std::string & str)
		: Packet(STRING_PACKET_MAGIC_NUM), content(new char[str.size() + 1]),
		contentSize((int)str.size()), nextInvoked(0)
	{
		memcpy(content, str.c_str(), str.size());
		content[str.size()] = 0;
	}

	~StringPacket()
	{
		delete[] content;
	}

	virtual const char * Next(int * size) override
	{
		switch (nextInvoked++)
		{
		case 0:
			*size = 4;
			return (char *)&contentSize;
		case 1:
			*size = contentSize;
			return content;
		default:
			return nullptr;
		}
	}
};

typedef std::shared_ptr<StringPacket> StringPacketPtr;

#define TOUCH_PACKET_MAGIC_NUM 0x544F4348

struct TouchPacket : public Packet
{
	int		idx;
	float	x;
	float	y;
	int		down;
	int		nextInvoked;

	TouchPacket(int idx = -1, float x = 0.0f, float y = 0.0f, int down = 0)
		: Packet(TOUCH_PACKET_MAGIC_NUM), idx(idx), x(x), y(y), down(down), nextInvoked(0)
	{}

	virtual const char * Next(int * size) override
	{
		*size = 4;
		switch (nextInvoked++)
		{
		case 0:
			return (char *)&idx;
		case 1:
			return (char *)&x;
		case 2:
			return (char *)&y;
		case 3:
			return (char *)&down;
		default:
			*size = 0;
			return nullptr;
		}
	}
};

typedef std::shared_ptr<TouchPacket> TouchPacketPtr;