#include "network.h"

namespace network {

TcpServer::TcpServer(uint16_t port) {
	if ((createSocket = socket (AF_INET, SOCK_STREAM, 0)) == -1)
		throw std::runtime_error("cannot create socket.");

	const int optVal{1};
	setsockopt(createSocket, SOL_SOCKET, SO_REUSEADDR, &optVal, sizeof(int));

	listenAddress.sin_family = AF_INET;
	listenAddress.sin_addr.s_addr = INADDR_ANY;
	listenAddress.sin_port = htons(port);
	if (bind(createSocket, (struct sockaddr *) &listenAddress,
				sizeof (listenAddress)) != 0)
		throw std::runtime_error("bind error: cannot listen on port.");

	if (listen(createSocket, BACKLOG_LEN_MAX) != 0)
		throw std::runtime_error("listen error.");
}

std::string TcpServer::receiveClientMsg() {
	socklen_t addrlen{sizeof(struct sockaddr_in)};
	int rxSocket = accept(createSocket, (struct sockaddr*)&listenAddress,
			&addrlen);
	if (rxSocket <= 0)
		throw std::runtime_error("error on accept()");

	ssize_t size = recv(rxSocket, rxBuffer, RECEIVE_BUF_LEN-1, 0);
	if (size > 0)
		rxBuffer[size] = '\0';

	close(rxSocket);
	std::string out(rxBuffer);
	return out;
}

}
