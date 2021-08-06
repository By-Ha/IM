#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#define DEBUG
// windows.h 内含的 winsock.h 与 winsock2.h 冲突, 增加此3行以避免

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <string>
#include <iostream>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define RED_BLACK 12
#define BLACK_PINK 192
#define WHITE_BLACK 15

std::string logs;

struct transferStruct {
	int type; // 0 message 1 set username
	char data[1024];
};

void _basic_send_message(SOCKET s, std::string buf, int type = 0) {
	transferStruct data;
	strcpy_s(data.data, sizeof(data.data), buf.c_str());
	data.type = type;
	send(s, (const char*)&data, sizeof(data), 0);
}

void send_message(SOCKET s, std::string buf) {
	if (buf.length() >= 1023) {
		std::string tmp = buf.substr(0, 900);
		buf = buf.substr(901);
		_basic_send_message(s, tmp, 1);
		while (buf.length() > 1023) {
			std::string tmp = buf.substr(0, 1023);
			buf = buf.substr(1024);
			_basic_send_message(s, tmp, 2);
		}
		_basic_send_message(s, buf, 3);
	}
	else {
		_basic_send_message(s, buf);
	}
}

void output(const char* buf, const int color=7) {
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	printf("%s", buf);
	logs += buf;
}

unsigned __stdcall receive_message(void* args) {
	SOCKET* sClient = (SOCKET*)args;
	//char* recvData = new char[1024];
	transferStruct data;
	int ret;
	while (1) {
		ret = recv(*sClient, (char*)&data, sizeof(data), 0);
		if (ret < 0) { output("连接已断开...\n", BLACK_PINK); break; }
		if (data.type == 1) {
			output(data.data);
		}
		else if (data.type == 2) {
			output(data.data);
		}
		else if (data.type == 3) {
			output(data.data);
			output("\n");
		}
		else {
			output(data.data);
			output("\n");
		}
	}
	// delete[] recvData;
	closesocket(*sClient);
	exit(0);
	return 0;
}

int main(int argc, char** argv) {
	WSADATA wsaData;
	const char* sendbuf = "This is a test message.";
	char remote[512] = "localhost";
	int iResult;
	int recvbuflen = DEFAULT_BUFLEN;

	// Initialize Winsock
	iResult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	/*
	int WSAStartup(
	  WORD      wVersionRequired,   什么鬼版本？
	  LPWSADATA lpWSAData           指向WSADATA的指针
	);
	*/

	if (iResult != 0) {
		printf("WSAStartup failed: %d\n", iResult);
		return 1;
	}

	// 以上为初始化Socket,服务端与客户端一致

	struct addrinfo* result = NULL, * ptr = NULL, hints;

	ZeroMemory(&hints, sizeof(hints));	// memset(xxx, 0, sizeof(xxx))
	hints.ai_family = AF_UNSPEC;		// address family ipv4/ipv6...
	hints.ai_socktype = SOCK_STREAM;	// socket type
	hints.ai_protocol = IPPROTO_TCP;	// protocol type

	// Resolve the server address and port (DNS)
	// 从参数传入的访问地址，默认端口，网络协议，传出结果

	puts("Please input server ip:");
	scanf_s("%s", remote, 510);

	iResult = getaddrinfo(remote, DEFAULT_PORT, &hints, &result);

	if (iResult != 0) {
		printf("getaddrinfo failed: %d\n", iResult);
		WSACleanup();
		return 1;
	}

	SOCKET ConnectSocket = INVALID_SOCKET;
	// Attempt to connect to the first address returned by
	// the call to getaddrinfo
	ptr = result;

	// Create a SOCKET for connecting to server
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next) {
		// ai_next 下一个DNS解析的IP地址, 如 www.baidu.com -> xxx.xxx.xx.xx yyy.yyy.yy.yy
		ConnectSocket = socket(ptr->ai_family, ptr->ai_socktype, ptr->ai_protocol);
		if (ConnectSocket == INVALID_SOCKET) {
			printf("Error at socket(): %ld\n", WSAGetLastError());

			WSACleanup();
			// 与WSASTARTUP共同使用,释放对dll的绑定
			return 1;
		}

		// Connect to server.
		iResult = connect(ConnectSocket, ptr->ai_addr, (int)ptr->ai_addrlen);
		if (iResult == SOCKET_ERROR) {
			closesocket(ConnectSocket);
			ConnectSocket = INVALID_SOCKET;
			continue;
		}
		break;
	}

	freeaddrinfo(result);
	// result 内结构储存空间都由malloc获取 使用此函数释放

	if (ConnectSocket == INVALID_SOCKET) {
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	HANDLE message = (HANDLE)_beginthreadex(NULL, 0, receive_message, &ConnectSocket, 0, 0);

	std::string sendData;
	while (true)
	{
		sendData = "";
		std::cin >> sendData;
		if (sendData == "quit") break;
		else if (sendData == "save") {
			puts("Input save path:");
			std::string save_path;
			std::cin >> save_path;
			FILE* file;
			int ret = fopen_s(&file, save_path.c_str(), "w");
			if (ret != 0 || file == NULL) {
				printf("Unable to save to %s, please check.", save_path.c_str());
				continue;
			}
			fprintf_s(file, "%s", logs.c_str());
			fclose(file);
			printf("Successfully save to %s", save_path.c_str());
			continue;
		}
		system("cls");
		std::cout << logs;
		send_message(ConnectSocket, sendData.c_str());
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}
