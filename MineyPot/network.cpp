#pragma once
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <vector>
#include "varint.h"
#include "network.h"
#define PORT "25565"

WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET;

char jsonTemplate[] = "{\"description\":{\"text\":\"%s\"},\"players\":{\"max\":%i,\"online\":%i},\"version\":{\"name\":\"%s\",\"protocol\":%i}}";
char jsonFormatted[512] = {}; //yes 512 bytes is completely arbitary and is way too big but we dont live in the 80s anymore

char pingLogText[] = "%s | Ping from %s";
char connectionLogText[] = "%s | Logon from %s (%s)";

void updateServerParameters(const char* description, int maxPlayers, int currentPlayers, protocolVersion pv) {
	sprintf_s(jsonFormatted, 512, jsonTemplate, description, maxPlayers, currentPlayers, pv.protocolName, pv.protocolID);
}

void logNewConnection(int type, char* ipAddy, char* playerName = nullptr) {
	char* newLog = new char[128];
	const char* time = getTime();
	if (type == 0) {
		sprintf_s(newLog, 128, pingLogText, time, ipAddy);
		int stringActualLength = strlen(newLog);
		char* newLogSized = new char[stringActualLength + 1];
		sprintf_s(newLogSized, stringActualLength + 1, newLog);
		delete[] newLog;
		conLog.push_back(newLogSized);
	}
	else if(type == 1 && playerName != 0) {
		sprintf_s(newLog, 128, connectionLogText,time, ipAddy, playerName);
		int stringActualLength = strlen(newLog);
		char* newLogSized = new char[stringActualLength + 1];
		sprintf_s(newLogSized, stringActualLength + 1, newLog);
		delete[] newLog;
		conLog.push_back(newLogSized);
	}
	delete[] time;
	return;
}

bool clientHandler(SOCKET sock, char* ipAddy) {

	//Recieve the clients handshake, we dont need to read this but will test for a legacy or modern client
	byte clientHandshake[1024];
	int handshakeSize = recv(sock, (char*)clientHandshake, 1024, 0);
	if(handshakeSize == SOCKET_ERROR){
		closesocket(sock);
		return false;
	}

	if (clientHandshake[0] == 0xFE) {
		//Client has sent a legacy request, will emplement this later
		printf_s("Legacy client\n");
		closesocket(sock);
		return false;
	}

	if (clientHandshake[handshakeSize - 1] == 0x02) {
		//client is attempting to connect to the server

		//get client infomation, playername, etc
		byte clientData[1024];
		int clientDataSize = recv(sock, (char*)clientData, 1024, 0);

		int cdptr = 0;
		varint packetLength = clientData + cdptr;
		cdptr += packetLength.len();

		varint packetID = clientData + cdptr;
		cdptr += packetID.len();

		varint playerNameLength = clientData + cdptr;
		cdptr += playerNameLength.len();

		char* clientName = new char[playerNameLength.iVal() + 1];
		ZeroMemory(clientName, playerNameLength.iVal() + 1);
		memcpy(clientName, (char*)(clientData + cdptr), playerNameLength.iVal());

		logNewConnection(1, ipAddy, clientName);

		closesocket(sock);
		return false;
	}

	//Recieve the clients status request, we dont need to do anything with this either
	byte clientStatusReq[1024];
	if (recv(sock, (char*)clientStatusReq, 1024, 0) == SOCKET_ERROR) {
		closesocket(sock);
		return false;
	}

	//This is a lazy way to do packet formatting, i dont care
	//response is as follows
	//VARINT: packet size
	//VARINT: packet ID (0x00)
	//CHAR[]: json data

	varint ID = 0x00;
	varint StrLen = strlen(jsonFormatted);
	varint pktLength = ID.len() + StrLen.len() + strlen(jsonFormatted);
	int totalPacketSize = pktLength.len() + pktLength.iVal();
	byte* packetContainer = (byte*)malloc(totalPacketSize);
	if (!packetContainer) {
		closesocket(sock);
		return false;
	}

	int cptr = 0;
	memcpy(packetContainer + cptr, pktLength.pData(), pktLength.len());
	cptr += pktLength.len();
	memcpy(packetContainer + cptr, ID.pData(), ID.len());
	cptr += ID.len();
	memcpy(packetContainer + cptr, StrLen.pData(), StrLen.len());
	cptr += StrLen.len();
	memcpy(packetContainer + cptr, jsonFormatted, strlen(jsonFormatted));
	cptr += strlen(jsonFormatted);

	//send off the server info and free the created packet
	if(send(sock, (char*)packetContainer, totalPacketSize, 0) == SOCKET_ERROR) {
		closesocket(sock);
		return false;
	};
	free(packetContainer);

	//Recieve and retransmit client ping
	byte clientPing[128];
	int pingPtr = 0;
	if(recv(sock, (char*)clientPing, 128, 0) == SOCKET_ERROR) {
		closesocket(sock);
		return false;
	};
	if(send(sock, (char*)clientPing, 128, 0) == SOCKET_ERROR) {
		closesocket(sock);
		return false;
	};

	logNewConnection(0, ipAddy);

	closesocket(sock);
	return true;
}

void listener() {
	while (true) {
		SOCKET nextClient = INVALID_SOCKET;
		nextClient = INVALID_SOCKET;

		SOCKADDR_IN addr;
		int addrlen = sizeof(addr);
		nextClient = accept(ListenSocket, (SOCKADDR*)&addr, &addrlen);

		if (nextClient != INVALID_SOCKET) {
			char ip[32] = {};
			inet_ntop(AF_INET, &addr.sin_addr, (PSTR)&ip, 32);
			clientHandler(nextClient, ip);
		}
		else {
			printf_s("Invalid socket");
		}
	}
}

bool init() {

	//generic winsock setup code
	int iresult = WSAStartup(MAKEWORD(2, 2), &wsaData);
	if (iresult != 0) {
		printf_s("WSAStartup failed: %d\n", iresult);
		WSACleanup();
		return false;
	}

	addrinfo* result = NULL;
	addrinfo* ptr = NULL;
	addrinfo hints;

	ZeroMemory(&hints, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_PASSIVE;

	iresult = getaddrinfo(NULL, PORT, &hints, &result);
	if (iresult != 0) {
		wprintf_s(L"getaddrinfo failed: %d\n", iresult);
		WSACleanup();
		return false;
	}

	ListenSocket = socket(result->ai_family, result->ai_socktype, result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		wprintf_s(L"ListenSocket creation failed: %d\n", iresult);
		WSACleanup();
		return false;
	}

	iresult = bind(ListenSocket, result->ai_addr, (int)result->ai_addrlen);
	if (iresult != 0) {
		wprintf_s(L"bind failed: %d\n", iresult);
		WSACleanup();
		return false;
	}

	if (listen(ListenSocket, SOMAXCONN) == SOCKET_ERROR) {
		wprintf_s(L"listen failed: %d\n", iresult);
		WSACleanup();
		return false;
	}

	//start listening for new clients
	std::thread tListener(listener);
	tListener.detach();
}

const char* getTime() {
	char* timeBuffer = new char[16];
	time_t now = time(0);
	struct tm tstruct;
	localtime_s(&tstruct, &now);
	strftime(timeBuffer, 16, "%I:%M:%S", &tstruct);
	return timeBuffer;
}