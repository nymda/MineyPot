#pragma once
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include "varint.h"
#include "network.h"
#define PORT "25565"

WSADATA wsaData;
SOCKET ListenSocket = INVALID_SOCKET;

char jsonTemplate[] = "{\"description\":{\"text\":\"%s\"},\"players\":{\"max\":%i,\"online\":%i},\"version\":{\"name\":\"%s\",\"protocol\":%i}}";
char jsonFormatted[512] = {};

char pingLogText[] = "%s | Ping from %s";
char connectionLogText[] = "%s | Connection from user %s / %s";

void updateServerParameters(const char* description, int maxPlayers, int currentPlayers, protocolVersion pv) {
	sprintf_s(jsonFormatted, 512, jsonTemplate, description, maxPlayers, currentPlayers, pv.protocolName, pv.protocolID);
}

void logNewConnection(int type, const char* ipAddy, const char* playerName = 0) {

}

bool clientHandler(SOCKET sock) {

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
		printf_s("Login from user: %s\n", clientName);

		closesocket(sock);
		return false;
	}

	//Recieve the clients status request, we dont need to do anything with this either
	byte clientStatusReq[2056];
	if (recv(sock, (char*)clientStatusReq, 2056, 0) == SOCKET_ERROR) {
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
			printf_s("New connection from : %s\n", ip);
			clientHandler(nextClient);
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