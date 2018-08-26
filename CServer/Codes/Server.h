#pragma once
class Server
{
public:
	Server();
	~Server();

private:

	SOCKET _listenSocket;
	SOCKET _clientSocket;
};

