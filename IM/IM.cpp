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
#include <mswsock.h>

#pragma comment(lib, "Ws2_32.lib")

#define DEFAULT_BUFLEN 512
#define DEFAULT_PORT "27015"

#define RED_BLACK 12
#define BLACK_PINK 192
#define WHITE_BLACK 15

char logs[1024000];
FILE* fp;

struct transferStruct
{
	int type, length;
	char data[1024];
};

void _basic_send_message(SOCKET s, char* buf, int type = 0, int length = 0)
{
	transferStruct data;
	strcpy_s(data.data, sizeof(data.data), buf);
	data.type = type; data.length = length;
	send(s, (const char*)&data, sizeof(data), 0);
}

void _basic_send_file(SOCKET s, const char *buf, int type = 0, int length = 0)
{
	transferStruct data;
	memcpy_s(data.data, 1024, buf, length);
	data.type = type; data.length = length;
	send(s, (const char*)&data, sizeof(data), 0);
}

//void send_message(SOCKET s, std::string buf) {
//	if (buf.length() >= 901) {
//		std::string tmp = buf.substr(0, 900);
//		buf = buf.substr(900);
//		_basic_send_message(s, tmp, 1);
//		while (buf.length() > 1023) {
//			std::string tmp = buf.substr(0, 1023);
//			buf = buf.substr(1023);
//			_basic_send_message(s, tmp, 2);
//		}
//		_basic_send_message(s, buf, 3);
//	}
//	else {
//		_basic_send_message(s, buf);
//	}
//}
void send_message(SOCKET s, char *buf)
{
	_basic_send_message(s, buf);
}
void send_file(SOCKET s, FILE* fp, char* file_name)
{
	_basic_send_message(s, file_name, 5);
	char buffer[1024]; int len = 0;
	while ((len = fread_s(buffer, 1024, 1, 1024, fp))!=0) {
		printf("%d\n", len);
		_basic_send_file(s, buffer, 6, len);
	}
	_basic_send_file(s, "", 7, 0);
}

void addLog(const char* buf)
{
	if (strlen(logs) > 1020000) memset(logs, 0, sizeof(logs));
	strcat_s(logs, 1020000, buf);
}

void output(const char* buf, const int color = 7)
{
	HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	SetConsoleTextAttribute(hConsole, color);
	printf("%s", buf);
	addLog(buf);
}

unsigned __stdcall receive_message(void* args)
{
	SOCKET* sClient = (SOCKET*)args;
	//char* recvData = new char[1024];
	transferStruct data;
	int ret;
	while (1) {
		ret = recv(*sClient, (char*)&data, sizeof(data), 0);
		if (ret < 0) 
		{ 
			output("连接已断开...\n", BLACK_PINK);
			break; 
		}
		if (data.type == 1)
		{
			output(data.data);
		}
		else if (data.type == 2)
		{
			output(data.data);
		}
		else if (data.type == 3)
		{
			output(data.data);
			output("\n");
		}
		else if (data.type == 5)
		{ // file name
			printf("Receive file:%s\n", data.data);
			int ret = fopen_s(&fp, data.data, "wb");
			if (fp == NULL) {
				puts("Cannot write file!");
			}
		}
		else if (data.type == 6)
		{
			printf("Receive file.\n");
			if (fp == NULL) {
				puts("Cannot write file!");
				continue;
			}
			fwrite(data.data, data.length, (size_t)1, fp);
		}
		else if (data.type == 7)
		{
			printf("Receive file end.\n");
			if (fp == NULL) {
				puts("Cannot write file!");
				continue;
			}
			fwrite(data.data, data.length, (size_t)1, fp);
			fclose(fp);
		}
		else
		{
			output(data.data);
			output("\n");
		}
	}
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
	for (ptr = result; ptr != NULL; ptr = ptr->ai_next)
	{
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

	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("Unable to connect to server!\n");
		WSACleanup();
		return 1;
	}

	HANDLE message = (HANDLE)_beginthreadex(NULL, 0, receive_message, &ConnectSocket, 0, 0);

	char sendData[1025] = { 0 };
	getchar(); // clear \n

	while (true)
	{
		memset(sendData, 0, sizeof(sendData));
		gets_s(sendData, 1024);
		// scanf_s("%[^\n]%*c", sendData, 1024);
		if (strcmp(sendData, "/quit") == 0) {
			break;
		}
		else if (strcmp(sendData, "/save") == 0)
		{
			printf("Input save path:");
			char save_path[1025];
			gets_s(save_path, 1024);
			FILE* file;
			int ret = fopen_s(&file, save_path, "w");
			if (ret != 0 || file == NULL) {
				printf("Unable to save to %s, please check.", save_path);
				continue;
			}
			fprintf_s(file, "%s", logs);
			fclose(file);
			printf("Successfully save to %s", save_path);
			continue;
		}
		else if (strcmp(sendData, "/name") == 0)
		{
			printf("Please input your name(max length 100):");
			char name[1025];
			scanf_s("%s", name, 1024);
			getchar();
			_basic_send_message(ConnectSocket, name, 4);
			continue;
		}
		else if (strcmp(sendData, "/file") == 0)
		{
			printf("Please input your file path:");
			char file_path[1025] = { 0 }, file_name[1025] = { 0 }, file[1025] = { 0 };
			FILE* fp;
			char tmp_buf[1024] = { 0 };
			scanf_s("%s", file_path, 1024);
			printf("Please input your file name:");
			scanf_s("%s", file_name, 1024);
			int ret = fopen_s(&fp, file_path, "rb");
			if (ret)
			{
				puts("Open file error!");
				continue;
			}
			send_file(ConnectSocket, fp, file_name);
			fclose(fp);
			getchar();
			continue;
		}
		system("cls");
		puts(logs);
		send_message(ConnectSocket, sendData);
	}

	closesocket(ConnectSocket);
	WSACleanup();

	return 0;
}
