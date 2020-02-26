// TransferMode1.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "winsock2.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

bool receiverOnline = false; // Receiver是否上线（与Transfer建立连接）
bool senderOffline = false; // Sender是否下线
int listenSenderPort = 0;
int listenReceiverPort = 0;
u_long host = INADDR_ANY;

char* DATA_POINTER = NULL;
int DATA_LENGTH = -1;
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

// sender 通信线程
DWORD WINAPI listenSender(LPVOID lpParamter)
{
	int recvLen = 0;
	char lenBuf[32]; // 用于接收数据长度
	WSADATA wsaData;
	int port = listenSenderPort;
	save2log("listenSender", "[+] Listen Sender Port: %d\n", port);
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		save2log("listenSender", "[!] WSAStartup Error %d\n", GetLastError());
		return 0;
	}

	// 创建用于监听的套接字,即服务端的套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0); // TCP

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port); // 1024以上的端口号
	addrSrv.sin_addr.S_un.S_addr = htonl(host); // 0.0.0.0

	int retVal = bind(sockSrv, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN));
	if (retVal == SOCKET_ERROR)
	{
		save2log("listenSender", "[!] Bind Error %d\n", WSAGetLastError());
		return 0;
	}

	if (listen(sockSrv, 10) == SOCKET_ERROR) // 最大连接要求 10
	{
		save2log("listenSender", "[!] Listen Error %d\n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (true)
	{
		SOCKET sockConn = accept(sockSrv, (SOCKADDR *)&addrClient, &len);
		if (sockConn == SOCKET_ERROR)
		{
			save2log("listenSender", "[!] Accept Error %d\n", WSAGetLastError());
			break;
		}

		//printf("[+] Client IP: %s\n", inet_ntoa(addrClient.sin_addr));
		char ipTmp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addrClient.sin_addr, ipTmp, sizeof(ipTmp));
		save2log("listenSender", "[+] Client IP: %s\n", ipTmp);

		// 发送数据，通知sender是否receiver已上线
		if (!receiverOnline)
		{
			char sendbuf[] = "Receiver Not Online.";
			if (SOCKET_ERROR == send(sockConn, sendbuf, sizeof(sendbuf), 0))
			{
				save2log("listenSender", "[!] Send Error %d\n", GetLastError());
				break;
			}
			continue;
		}
		else
		{
			char sendbuf[] = "Receiver Online.";
			if (SOCKET_ERROR == send(sockConn, sendbuf, sizeof(sendbuf), 0))
			{
				save2log("listenSender", "[!] Send Error %d\n", GetLastError());
				break;
			}
		}
		
		// 从Sender接收数据
		while (true)
		{
			WaitForSingleObject(DATA_LOCK, INFINITE); // 等待锁

			if (!(-1 == DATA_LENGTH && NULL == DATA_POINTER))
			{
				// 如果上次数据仍然存在则直接将其删除
				DATA_LENGTH = -1;
				if (DATA_POINTER)
				{
					free(DATA_POINTER);
				}
				DATA_POINTER = NULL;
			}

			// 接收数据长度
			memset(lenBuf, 0, sizeof(lenBuf));
			recv(sockConn, lenBuf, sizeof(lenBuf), 0);
			DATA_LENGTH = atoi(lenBuf);

			if (DATA_LENGTH == -1) // Sender发送完毕下线
			{
				senderOffline = true;
				ReleaseMutex(DATA_LOCK); // 释放锁
				save2log("listenSender", "[+] Sender Offline\n");
				break;
			}

			// 接收数据
			DATA_POINTER = (char*)malloc(DATA_LENGTH);
			if (!DATA_POINTER)
			{
				save2log("listenSender", "[!] Malloc Error %d\n", GetLastError());
			}
			else
			{
				recvLen = recv(sockConn, DATA_POINTER, DATA_LENGTH, 0);
			}

			save2log("listenSender", "[+] %d, %s\n", DATA_LENGTH, DATA_POINTER);

			ReleaseMutex(DATA_LOCK); // 释放锁
		}

		closesocket(sockConn);
		if (senderOffline)
		{
			break;
		}
	}

	closesocket(sockSrv);
	WSACleanup();
	save2log("listenSender", "[+] Close\n");

	return 0;
}

// receiver 通信线程
DWORD WINAPI listenReceiver(LPVOID lpParamter)
{
	char lenBuf[32]; // 用于发送数据长度
	WSADATA wsaData;
	int port = listenReceiverPort;
	save2log("listenReceiver", "[+] Listen Receiver Port: %d\n", port);
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		save2log("listenReceiver", "[!] WSAStartup Error %d\n", GetLastError());
		return 0;
	}

	// 创建用于监听的套接字,即服务端的套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0); // TCP

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port); // 1024以上的端口号
	addrSrv.sin_addr.S_un.S_addr = htonl(host); // 0.0.0.0

	int retVal = bind(sockSrv, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN));
	if (retVal == SOCKET_ERROR)
	{
		save2log("listenReceiver", "[!] Bind Error %d\n", WSAGetLastError());
		return 0;
	}

	if (listen(sockSrv, 10) == SOCKET_ERROR) // 最大连接要求 10
	{
		save2log("listenReceiver", "[!] Listen Error %d\n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);

	while (true)
	{
		SOCKET sockConn = accept(sockSrv, (SOCKADDR *)&addrClient, &len);
		if (sockConn == SOCKET_ERROR)
		{
			save2log("listenReceiver", "[!] Accept Error %d\n", WSAGetLastError());
			break;
		}

		//printf("[+] Client IP: %s\n", inet_ntoa(addrClient.sin_addr));
		char ipTmp[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &addrClient.sin_addr, ipTmp, sizeof(ipTmp));
		save2log("listenReceiver", "[+] Client IP: %s\n", ipTmp);

		receiverOnline = true; // receiver已上线

		// 向Receiver发送数据
		while (true)
		{
			WaitForSingleObject(DATA_LOCK, INFINITE); // 等待锁

			if (senderOffline)
			{
				memset(lenBuf, 0, sizeof(lenBuf));
				sprintf(lenBuf, "-1");
				if (SOCKET_ERROR == send(sockConn, lenBuf, sizeof(lenBuf), 0))
				{
					save2log("listenReceiver", "[!] Send Error %d\n", GetLastError());
					break;
				}
				save2log("listenReceiver", "[+] Sender Offline\n");
				ReleaseMutex(DATA_LOCK); // 释放锁
				break;
			}

			if (-1 == DATA_LENGTH && NULL == DATA_POINTER)
			{
				Sleep(2000);
			}
			else
			{
				// 发送数据长度
				memset(lenBuf, 0, sizeof(lenBuf));
				sprintf(lenBuf, "%d", DATA_LENGTH);
				if (SOCKET_ERROR == send(sockConn, lenBuf, sizeof(lenBuf), 0))
				{
					save2log("listenReceiver", "[!] Send Error %d\n", GetLastError());
					break;
				}

				// 发送数据
				if (SOCKET_ERROR == send(sockConn, DATA_POINTER, DATA_LENGTH, 0))
				{
					save2log("listenReceiver", "[!] Send Error %d\n", GetLastError());
					break;
				}

				// 重置数据
				free(DATA_POINTER);
				DATA_POINTER = NULL;
				DATA_LENGTH = -1;
			}

			ReleaseMutex(DATA_LOCK); // 释放锁
		}

		closesocket(sockConn);
		if (senderOffline)
		{
			break;
		}
	}

	closesocket(sockSrv);
	WSACleanup();
	save2log("listenReceiver", "[+] Close\n");

	return 0;
}

int main(int argc, char* argv[])
{
	if (argc != 3)
	{
		save2log("", "Usage: TransferMode1.exe listen_port_from listen_port_to.\n");
		return 0;
	}

	listenSenderPort = atoi(argv[1]);
	listenReceiverPort = atoi(argv[2]);
	save2log("", "[+] Transfer data from %d to %d.\n", listenSenderPort, listenReceiverPort);

	DATA_LOCK = CreateMutex(NULL, false, NULL);
	if (NULL == DATA_LOCK)
	{
		save2log("", "[!] CreateMutex Error %d\n", GetLastError());
		return 0;
	}

	HANDLE senderThread = CreateThread(NULL, 0, listenSender, NULL, 0, NULL);
	HANDLE receiverThread = CreateThread(NULL, 0, listenReceiver, NULL, 0, NULL);

	CloseHandle(senderThread);
	CloseHandle(receiverThread);

	while (true)
	{
	}

	return 0;
}

