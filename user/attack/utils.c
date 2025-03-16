#include <openssl/ec.h>

#include "utils.h"

int hexCharToInt(unsigned char c) {
	static int table_created = 0;
	static int table[256];

	int i;

	if (table_created == 0) {
		for (i = 0; i < 256; i++) {
			switch ((unsigned char) i) {
				case '0':
					table[i] = 0;
					break;
				case '1':
					table[i] = 1;
					break;
				case '2':
					table[i] = 2;
					break;
				case '3':
					table[i] = 3;
					break;
				case '4':
					table[i] = 4;
					break;
				case '5':
					table[i] = 5;
					break;
				case '6':
					table[i] = 6;
					break;
				case '7':
					table[i] = 7;
					break;
				case '8':
					table[i] = 8;
					break;
				case '9':
					table[i] = 9;
					break;
				case 'A':
				case 'a':
					table[i] = 10;
					break;
				case 'B':
				case 'b':
					table[i] = 11;
					break;
				case 'C':
				case 'c':
					table[i] = 12;
					break;
				case 'D':
				case 'd':
					table[i] = 13;
					break;
				case 'E':
				case 'e':
					table[i] = 14;
					break;
				case 'F':
				case 'f':
					table[i] = 15;
					break;
				default:
					table[i] = -1;
			}
		}

		table_created = 1;
	}

	return table[c];
}

int getmac(const char *macAddress, const int strict, unsigned char *mac) {
	char byte[3];
	int i, nbElem, n;

	if (macAddress == NULL) return 1;

	/* Minimum length */
	if ((int) strlen(macAddress) < 12) return 1;

	memset(mac, 0, 6);
	byte[2] = 0;
	i = nbElem = 0;

	while (macAddress[i] != 0) {
		if (macAddress[i] == '\n' || macAddress[i] == '\r') break;

		byte[0] = macAddress[i];
		byte[1] = macAddress[i + 1];

		if (sscanf(byte, "%x", &n) != 1 && strlen(byte) == 2) return 1;

		if (hexCharToInt(byte[1]) < 0) return 1;

		mac[nbElem] = n;

		i += 2;
		nbElem++;

		if (macAddress[i] == ':' || macAddress[i] == '-'
		    || macAddress[i] == '_')
			i++;
	}

	if ((strict && nbElem != 6) || (!strict && nbElem > 6)) return 1;

	return 0;
}
