#include "network.h"
#include <iostream>

using namespace std;

int main(void) {
	network::TcpServer tcpServer(12345);
	std::string msg = tcpServer.receiveClientMsg();
	cout << msg;

	return 0;
}
