// TransferMode2.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "winsock2.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

bool transferMode1Offline = false; // 模式一的中转是否下线
char* ipFrom = NULL;
char* ipTo = NULL;
int portFrom = 0;
int portTo = 0;

char* DATA_POINTER = NULL;
int DATA_LENGTH = 0;
HANDLE DATA_LOCK; // 数据锁

void save2log(const char* prefix, const char* format, ...)
{
	va_list args;
	printf("[%s] ", prefix);
	va_start(args, format);
	vprintf(format, args);
	va_end(args);
	return;
}

// 主动连接模式一的中转，接收数据
DWORD WINAPI listenFrom(LPVOID lpParamter)
{
	int recvLen = 0;

	char lenBuf[32];
	memset(lenBuf, 0, sizeof(lenBuf));
	char* dataBuf = NULL;

	int port = portFrom;
	printf("[From] [+] Connect to: %s:%d\n", ipFrom, port);

	// 加载套接字
	WSADATA wsaData;
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("[From] [!] WSAStartup Error %d\n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);// 端口号
	//addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");// IP地址
	inet_pton(AF_INET, ipFrom, &addrSrv.sin_addr);

	// 创建套接字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == sock)
	{
		printf("[From] [!] Socket Error %d\n", WSAGetLastError());
		return 0;
	}

	// 向服务器发出连接请求
	if (INVALID_SOCKET == connect(sock, (struct  sockaddr*)&addrSrv, sizeof(addrSrv)))
	{
		printf("[From] [!] Connect Error %d\n", WSAGetLastError());
		return 0;
	}
	else
	{
		// 接收数据
		while (true)
		{
			WaitForSingleObject(DATA_LOCK, INFINITE); // 等待锁
			// 数据长度
			recvLen = recv(sock, lenBuf, sizeof(lenBuf), 0);
			DATA_LENGTH = atoi(lenBuf);
			printf("[From] [+] %d\n", DATA_LENGTH);
			if (-1 == DATA_LENGTH)
			{
				transferMode1Offline = true;
				ReleaseMutex(DATA_LOCK); // 释放锁
				break;
			}

			// 数据
			DATA_POINTER = (char*)malloc(DATA_LENGTH);
			if (DATA_POINTER)
			{
				recvLen = recv(sock, DATA_POINTER, DATA_LENGTH, 0);
				printf("[From] [+] %s\n", DATA_POINTER);
			}
			else
			{
				printf("[From] [!] malloc %d\n", GetLastError());
			}
			ReleaseMutex(DATA_LOCK); // 释放锁
		}
	}

	//关闭套接字
	closesocket(sock);
	WSACleanup();//释放初始化Ws2_32.dll所分配的资源。
	printf("[+] Close listenFrom.\n");

	return 0;
}

// 主动连接 Receiver 并传递数据
DWORD WINAPI listenTo(LPVOID lpParamter)
{
	int recvLen = 0;

	char lenBuf[32];
	memset(lenBuf, 0, sizeof(lenBuf));
	char* dataBuf = NULL;

	int port = portTo;
	printf("[To] [+] Connect to: %s:%d\n", ipTo, port);

	//加载套接字
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("[To] [!] WSAStartup Error %d\n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	inet_pton(AF_INET, ipTo, &addrSrv.sin_addr);

	// 创建套接字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == sock)
	{
		printf("[To] [!] Socket Error %d\n", WSAGetLastError());
		return 0;
	}

	// 连接 Receiver
	if (INVALID_SOCKET == connect(sock, (struct sockaddr*)&addrSrv, sizeof(addrSrv)))
	{
		printf("[To] [!] Connect Error %d\n", WSAGetLastError());
		return 0;
	}
	else
	{
		// 发送数据
		while (true)
		{
			WaitForSingleObject(DATA_LOCK, INFINITE); // 等待锁

			if (0 == DATA_LENGTH && NULL == DATA_POINTER) // 无数据到达
			{
				Sleep(2000);
			}
			else
			{
				// 数据长度
				memset(lenBuf, 0, sizeof(lenBuf));
				sprintf(lenBuf, "%d", DATA_LENGTH);
				recvLen = send(sock, lenBuf, sizeof(lenBuf), 0);
				printf("[To] [+] Send %d\n", DATA_LENGTH);
				if (-1 == DATA_LENGTH)
				{
					ReleaseMutex(DATA_LOCK); // 释放锁
					break;
				}

				// 数据
				recvLen = send(sock, DATA_POINTER, DATA_LENGTH, 0);
				printf("[To] [+] Send %s %d\n", DATA_POINTER, recvLen);

				free(DATA_POINTER);
				DATA_POINTER = NULL;
				DATA_LENGTH = 0;
			}
			
			ReleaseMutex(DATA_LOCK); // 释放锁
		}
	}

	//关闭套接字
	closesocket(sock);
	WSACleanup();//释放初始化Ws2_32.dll所分配的资源。
	printf("[+] Close listenTo.\n");

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 5)
	{
		save2log("", "Usage: TransferMode2.exe listen_ip_from listen_port_from listen_ip_to listen_port_to.\n");
		return 0;
	}

	ipFrom = argv[1];
	portFrom = atoi(argv[2]);
	ipTo = argv[3];
	portTo = atoi(argv[4]);
	
	save2log("", "[+] Transfer data from %s:%d to %s:%d.\n", ipFrom, portFrom, ipTo, portTo);

	DATA_LOCK = CreateMutex(NULL, false, NULL);
	if (NULL == DATA_LOCK)
	{
		save2log("", "[!] CreateMutex Error %d\n", GetLastError());
		return 0;
	}

	HANDLE fromThread = CreateThread(NULL, 0, listenFrom, NULL, 0, NULL);
	HANDLE toThread = CreateThread(NULL, 0, listenTo, NULL, 0, NULL);

	CloseHandle(fromThread);
	CloseHandle(toThread);

	while (true)
	{
	}

	return 0;
}
