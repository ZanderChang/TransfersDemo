// Sender.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
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
	if (3 != argc)
	{
		printf("Usage: Sender.exe connect_ip connect_port.\n");
		return 0;
	}

	char tmpBuf[128] = { 0 }; // 接收数据
	char lenBuf[32] = { 0 }; // 发送数据长度
	char dataBuf[] = "Secret from sender."; // 发送数据
	int sendLen = 0;
	int recvLen = 0;

	int port = atoi(argv[2]);
	printf("[+] Connect to: %s:%d\n", argv[1], port);

	// 加载套接字
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
		recvLen = recv(sock, tmpBuf, sizeof(tmpBuf), 0);

		// 发送数据
		// 数据长度
		sprintf(lenBuf, "%d", strlen(dataBuf)+1);
		printf("[+] %s\n", lenBuf);
		sendLen = send(sock, lenBuf, sizeof(lenBuf), 0);
		if (SOCKET_ERROR == sendLen)
		{
			printf("[!] Send Error %d\n", GetLastError());
			return 0;
		}
		printf("[+] SendLen: %d\n", sendLen);

		// 数据
		printf("[+] %s\n", dataBuf);
		sendLen = send(sock, dataBuf, sizeof(dataBuf), 0);
		if (SOCKET_ERROR == sendLen)
		{
			printf("[!] Send Error %d\n", GetLastError());
			return 0;
		}
	}

	memset(lenBuf, 0, sizeof(lenBuf));
	sprintf(lenBuf, "-1");
	printf("[+] %s\n", lenBuf);
	sendLen = send(sock, lenBuf, sizeof(lenBuf), 0);
	if (SOCKET_ERROR == sendLen)
	{
		printf("[!] Send Error %d\n", GetLastError());
		return 0;
	}

	//关闭套接字
	closesocket(sock);
	WSACleanup();//释放初始化Ws2_32.dll所分配的资源。

	return 0;
}