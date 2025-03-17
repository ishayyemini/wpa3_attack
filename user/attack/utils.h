#ifndef DRAGONUTILS_H
#define DRAGONUTILS_H

#endif //DRAGONUTILS_H


int hexCharToInt(unsigned char c);

int get_mac(const char *macAddress, int strict, unsigned char *mac);

const unsigned char *get_src_from_packet(const unsigned char *buf);

const unsigned char *get_bssid_from_packet(const unsigned char *buf);

inline int is_retransmitted(const unsigned char *buf) {
	return buf[1] & 0x08;
}

inline int is_beacon(const unsigned char *buf) {
	return buf[0] == 0x80;
}

inline int is_authentication(const unsigned char *buf, const int len) {
	return len >= 24 + 8 &&
	       buf[0] == 0xb0 && /* Type is Authentication */
	       buf[24] == 0x03 /* Auth type is SAE */;
}

inline int is_status_ok(const unsigned char *buf) {
	return buf[28] == 0x00;
}

inline int is_status_clogged(const unsigned char *buf) {
	return buf[28] == 0x4c;
}

inline int is_status_unsupported(const unsigned char *buf) {
	return buf[28] == 0x4d;
}
