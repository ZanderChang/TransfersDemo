// Receiver.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include "winsock2.h"
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

using namespace std;

int mode;
char lenBuf[32];
char* dataBuf = NULL;
int port;
char* ip;

int runInMode1()
{
	//加载套接字
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("[!] WSAStartup Error %d\n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);// 端口号
	//addrSrv.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");// IP地址
	inet_pton(AF_INET, ip, &addrSrv.sin_addr);

	// 创建套接字
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (SOCKET_ERROR == sock)
	{
		printf("Socket Error %d\n", WSAGetLastError());
		return 0;
	}

	// 向服务器发出连接请求
	if (INVALID_SOCKET == connect(sock, (struct  sockaddr*)&addrSrv, sizeof(addrSrv)))
	{
		printf("Connect Error %d\n", WSAGetLastError());
		return 0;
	}
	else
	{
		// 接收数据
		while (true)
		{
			// 数据长度
			recv(sock, lenBuf, sizeof(lenBuf), 0);
			int dataLen = atoi(lenBuf);
			printf("[+] %d\n", dataLen);
			if (-1 == dataLen)
			{
				break;
			}

			// 数据
			dataBuf = (char*)malloc(dataLen);
			if (dataBuf)
			{
				recv(sock, dataBuf, dataLen, 0);
				printf("[+] %s\n", dataBuf);
				free(dataBuf);
			}
			else
			{
				printf("[!] malloc %d\n", GetLastError());
			}
		}
	}

	//关闭套接字
	closesocket(sock);
	WSACleanup();//释放初始化Ws2_32.dll所分配的资源。

	return 0;
}

int runInMode2()
{
	//加载套接字
	WSADATA wsaData;

	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("[!] WSAStartup Error %d\n", WSAGetLastError());
		return 0;
	}

	// 创建用于监听的套接字,即服务端的套接字
	SOCKET sockSrv = socket(AF_INET, SOCK_STREAM, 0); // TCP

	SOCKADDR_IN addrSrv;
	addrSrv.sin_family = AF_INET;
	addrSrv.sin_port = htons(port);
	inet_pton(AF_INET, ip, &addrSrv.sin_addr);

	int retVal = bind(sockSrv, (LPSOCKADDR)&addrSrv, sizeof(SOCKADDR_IN));
	if (retVal == SOCKET_ERROR)
	{
		printf("[!] Bind Error %d\n", WSAGetLastError());
		return 0;
	}

	if (listen(sockSrv, 10) == SOCKET_ERROR) // 最大连接要求 10
	{
		printf("[!] Listen Error %d\n", WSAGetLastError());
		return 0;
	}

	SOCKADDR_IN addrClient;
	int len = sizeof(SOCKADDR);
	SOCKET sockConn = accept(sockSrv, (SOCKADDR *)&addrClient, &len);
	if (sockConn == SOCKET_ERROR)
	{
		printf("[!] Accept Error %d\n", WSAGetLastError());
		return 0;
	}

	char ipTmp[INET_ADDRSTRLEN];
	inet_ntop(AF_INET, &addrClient.sin_addr, ipTmp, sizeof(ipTmp));
	printf("[+] TransferMode2 IP: %s\n", ipTmp);

	int DATA_LENGTH = 0;
	char* DATA_POINTER = NULL;
	int recvLen = 0;

	// 接收数据
	while (true)
	{
		memset(lenBuf, 0, sizeof(lenBuf));
		recvLen = recv(sockConn, lenBuf, sizeof(lenBuf), 0);
		DATA_LENGTH = atoi(lenBuf);
		printf("[+] %d\n", DATA_LENGTH);
		if (-1 == DATA_LENGTH)
		{
			break;
		}

		DATA_POINTER = (char*)malloc(DATA_LENGTH);
		recvLen = recv(sockConn, DATA_POINTER, DATA_LENGTH, 0);
		printf("[+] %s\n", DATA_POINTER);

		free(DATA_POINTER);
	}

	//关闭套接字
	closesocket(sockConn);
	closesocket(sockSrv);
	WSACleanup();//释放初始化Ws2_32.dll所分配的资源。
	printf("[+] Close Receiver Mode2\n");

	return 0;
}

int main(int argc, char* argv[])
{
	int recvLen = 0;

	if (4 != argc)
	{
		printf("Usage: Receiver.exe mode connect_ip/listen_ip connect_port.\n");
		return 0;
	}

	mode = atoi(argv[1]);
	port = atoi(argv[3]);
	ip = argv[2];
	printf("[+] Connect to: %s:%d in Mode %d\n", ip, port, mode);

	if (1 == mode)
	{
		runInMode1();
	}
	else
	{
		runInMode2();
	}

	return 0;
}
