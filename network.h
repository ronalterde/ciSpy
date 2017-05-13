#pragma once

#include <string>
#include <stdexcept>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstdlib>
#include <cstring>
#include <cstdint>

namespace network {

class TcpServer {
public:
	TcpServer(uint16_t port);
	std::string receiveClientMsg();

private:
	const int RECEIVE_BUF_LEN{1500};
	const int BACKLOG_LEN_MAX{5};
	int createSocket;
	struct sockaddr_in listenAddress;
	char rxBuffer[1500];
};

}
