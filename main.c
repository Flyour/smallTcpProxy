#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#define DEFAULT_LOCAL_PORT 8898
#define DEFAULT_REMOTE_PORT 5900
#define BUFFER_SIZE 71680

typedef struct {
	SOCKET AcceptSocket;
	SOCKET ConnectSocket;
}threadArg;

int handleClient(threadArg* arg)
{
	SOCKET AcceptSocket = arg->AcceptSocket;
	SOCKET ConnectSocket = arg->ConnectSocket;
	char recvBuf[BUFFER_SIZE] = { 0 };
	int iResult = 0;
	int oResult = 0;
	//printf("accept socket %ld\n", (int)AcceptSocket);
	//printf("connect socket %ld\n", (int)ConnectSocket);
	while (1)
	{
		iResult = recv(AcceptSocket, recvBuf, BUFFER_SIZE, 0);
		if (iResult > 0)
		{
			printf("client -> @: %d bytes\n", iResult);
			oResult = send(ConnectSocket, recvBuf, iResult, 0);
			if (oResult == SOCKET_ERROR) {
				printf("send to server failed: %d\n", WSAGetLastError());
				return 1;
			}
			else
				printf("@ -> server: %d bytes\n", oResult);
		}
		else if (iResult == 0)
			printf("Handle Client: Connection closed\n");
		else
			printf("recv from client failed: %d\n", WSAGetLastError());
	}
}

int handleServer(threadArg* arg)
{
	SOCKET AcceptSocket = arg->AcceptSocket;
	SOCKET ConnectSocket = arg->ConnectSocket;
	char recvBuf[BUFFER_SIZE] = { 0 };
	int iResult = 0;
	int oResult = 0;
	while (1)
	{
		iResult = recv(ConnectSocket, recvBuf, BUFFER_SIZE, 0);
		if (iResult > 0)
		{
			printf("server -> @: %d bytes\n", iResult);
			oResult = send(AcceptSocket, recvBuf, iResult, 0);
			if (oResult == SOCKET_ERROR) {
				printf("send to client failed: %d\n", WSAGetLastError());
				return 1;
			}
			else
				printf("@ -> client: %d bytes\n", oResult);
		}
		else if (iResult == 0)
			printf("Handle Server: Connection closed\n");
		else
			printf("recv from server failed: %d\n", WSAGetLastError());
	}
}


int main(int argc, char* argv[])
{
	WORD sockVersion = MAKEWORD(2, 2);
	WSADATA wsaData;
	SOCKET ListenSocket = INVALID_SOCKET;
	SOCKET ConnectSocket = INVALID_SOCKET;
	struct sockaddr_in LocalAddr;
	struct sockaddr_in RemoteAddr;

	// init
	int	iResult = WSAStartup(sockVersion, &wsaData);
	if (iResult != NO_ERROR)
	{
		printf("WSAStartup failed with error: %d\n", iResult); 
		return 1;
	}

	// init listen socket
	ListenSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ListenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	LocalAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, "127.0.0.1", &LocalAddr.sin_addr.s_addr);
	LocalAddr.sin_port = htons(DEFAULT_LOCAL_PORT);

	// init connect socket
	ConnectSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (ConnectSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		return 1;
	}

	RemoteAddr.sin_family = AF_INET;
	InetPtonA(AF_INET, "192.168.33.130", &RemoteAddr.sin_addr.s_addr);
	RemoteAddr.sin_port = htons(DEFAULT_REMOTE_PORT);

	// 完成双向连接
	if (bind(ListenSocket, (SOCKADDR*)&LocalAddr, sizeof(LocalAddr)) == SOCKET_ERROR) {
		printf("bind failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	if (listen(ListenSocket, 1) == SOCKET_ERROR) {
		printf("listen failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	// 与客户端连接
	SOCKET AcceptSocket = accept(ListenSocket, NULL, NULL);
	if (AcceptSocket == INVALID_SOCKET) {
		printf("accept from failed with error: %ld\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}
	else
		printf("Client connected \n");

	// 与服务端连接
	iResult = connect(ConnectSocket, (SOCKADDR*)&RemoteAddr, sizeof(RemoteAddr));
	if (iResult == SOCKET_ERROR) {
		printf("connect to server failed with error: %d\n", WSAGetLastError());
		closesocket(ConnectSocket);
		WSACleanup();
		return 1;
	}
	else
		printf("Server connected \n");

	threadArg* arg = malloc(sizeof(threadArg));
	arg->AcceptSocket = AcceptSocket;
	arg->ConnectSocket = ConnectSocket;
	HANDLE hThreadClient = CreateThread(NULL, 0, handleClient, arg, 0, NULL);
	HANDLE hThreadServer = CreateThread(NULL, 0, handleServer, arg, 0, NULL);

	WaitForSingleObject(hThreadServer, INFINITE);
	WaitForSingleObject(hThreadServer, INFINITE);
	CloseHandle(hThreadClient);
	CloseHandle(hThreadServer);

	closesocket(ConnectSocket);
	closesocket(AcceptSocket);
	WSACleanup();

	return 0;
}
