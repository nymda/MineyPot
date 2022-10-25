#pragma once
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#include <vector>
#define PORT "25565"

enum class connectionType {
	PING = 0,
	LOGON = 1
};

struct protocolVersion {
	const char* protocolName;
	int protocolID;
};

struct connectionEvent {
	connectionType eventType;
	char eventTime[16];
	char eventIP[16];
	char eventUsername[20];
};

extern std::vector<connectionEvent> conLog;

const static protocolVersion versions[] = {
	{"1.19.2", 760},
	{"1.19.1", 760},
	{"1.19", 759},
	{"1.18.2", 758},
	{"1.18.1", 757},
	{"1.18", 757},
	{"1.17.1", 756},
	{"1.17", 755},
	{"1.16.5", 754},
	{"1.16.4", 754},
	{"1.16.3", 753},
	{"1.16.2", 751},
	{"1.16.1", 736},
	{"1.16", 735},
	{"1.15.2", 578},
	{"1.15.1", 575},
	{"1.15", 573},
	{"1.14.4", 498},
	{"1.14.3", 490},
	{"1.14.2", 485},
	{"1.14.1", 480},
	{"1.13.2", 404},
	{"1.13.1", 401},
	{"1.13", 393},
	{"1.12.2", 340},
	{"1.12.1", 338},
	{"1.12", 335},
	{"1.11.2", 316},
	{"1.11.1", 316},
	{"1.11", 315},
	{"1.10.2", 210},
	{"1.10.1", 210},
	{"1.10", 210},
	{"1.9.4", 110},
	{"1.9.3", 110},
	{"1.9.2", 109},
	{"1.9.1", 108},
	{"1.9", 107},
	{"1.8.9", 47},
	{"1.8.8", 47},
	{"1.8.7", 47},
	{"1.8.6", 47},
	{"1.8.5", 47},
	{"1.8.4", 47},
	{"1.8.3", 47},
	{"1.8.2", 47},
	{"1.8.1", 47},
	{"1.8", 47},
	{"1.7.10", 5},
	{"1.7.9", 5},
	{"1.7.8", 5},
	{"1.7.7", 5},
	{"1.7.6", 5},
	{"1.7.5", 4},
	{"1.7.4", 4},
	{"1.7.3", 4},
	{"1.7.2", 4}
};

const char* getTime();
bool clientHandler(SOCKET sock, char* ipAddy);
void listener();
bool init();
void updateServerParameters(const char* description, int maxPlayers, int currentPlayers, protocolVersion pv);