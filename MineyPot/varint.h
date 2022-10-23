#pragma once

#include <windows.h>
#include <stdio.h>

class varint {
private:
	byte data[5];
	int iValue;
	int length;
	void encode(int val);
	int decode(byte* val);

public:
	varint() {
		encode(0);
	}
	
	void operator = (const int V) {
		encode(V);
	}

	void operator = (byte* V) {
		decode(V);
	}

	varint(const int V) {
		encode(V);
	}

	varint(byte* V) {
		decode(V);
	}

	int len();
	byte* pData();
	int iVal();
};

void printEncoded(byte* encodedNumber, int encodedNumberLength);

