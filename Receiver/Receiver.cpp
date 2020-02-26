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

int main(int argc, char* argv[])
{
	int recvLen = 0;

	if (3 != argc)
	{
		printf("Usage: Receiver.exe connect_ip connect_port.\n");
		return 0;
	}

	char lenBuf[32];
	memset(lenBuf, 0, sizeof(lenBuf));
	char* dataBuf = NULL;

	int port = atoi(argv[2]);
	printf("[+] Connect to: %s:%d\n", argv[1], port);

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
	inet_pton(AF_INET, argv[1], &addrSrv.sin_addr);

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
			recvLen = recv(sock, lenBuf, sizeof(lenBuf), 0);
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
				recvLen = recv(sock, dataBuf, dataLen, 0);
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
