#include "varint.h"
#include <windows.h>
#include <stdio.h>

//encodes a varint value from an int and stores the value in its self
void varint::encode(int val) {
	UINT vCopy = (UINT)val;
	this->iValue = val;
	int ePtr = 0;
	memset(this->data, 0, 5);

	while (true) {
		byte cb = (byte)(vCopy & 0b01111111);
		vCopy >>= 7;
		if (vCopy) { cb |= 0b10000000; }
		this->data[ePtr] = cb;
		ePtr++;

		if (ePtr >= 5 || vCopy == 0) {
			this->length = ePtr;
			break;
		}
	}
}

//decodes a varint value from an arbitary byte* and stores the value in its self
int varint::decode(byte* val) {
	int value = 0;
	int ePtr = 0;

	while (true) {
		byte cByte = val[ePtr];
		value |= (cByte & 0x7F) << (ePtr * 7);
		ePtr++;
		if (ePtr > 5) { printf_s("Number is too long\n"); return 0;  }

		if ((cByte & 0x80) != 0x80) {
			memcpy(this->data, val, ePtr);
			break;
		}
	}

	this->length = ePtr;
	return this->iVal();
}

//returns the varints length in bytes
int varint::len() {
	return this->length;
}

//decodes the varints own value and returns it as an int
int varint::iVal() {
	int value = 0;
	int length = 0;
	int ePtr = 0;

	while (true) {
		byte cByte = this->data[ePtr];
		ePtr++;
		value |= (cByte & 0x7F) << (length * 7);

		length += 1;
		if (length > 5) {
			printf_s("Number is too long\n");
		}

		if ((cByte & 0x80) != 0x80) {
			break;
		}
	}

	return value;
}

//returns a pointer to the varints data
byte* varint::pData() {
	return this->data;
}

void printEncoded(byte* encodedNumber, int encodedNumberLength) {
	printf_s("encoded array: ");
	for (int i = 0; i < encodedNumberLength; i++) {
		printf_s("[ 0x%02X (%c) ], ", encodedNumber[i], (char)encodedNumber[i]);
	}
	printf_s("\n");
}
