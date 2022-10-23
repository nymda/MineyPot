#pragma once
#include <ws2tcpip.h>
#include <stdio.h>
#include <thread>
#define PORT "25565"

void clientHandler(SOCKET sock);
void listener();
bool init();
void updateServerParameters(const char* description, int maxPlayers, int currentPlayers, const char* protocolName, int protocolVersion);