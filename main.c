/*
 * Simultaneous authentication of equals
 * Copyright (c) 2012-2016, Jouni Malinen <j@w1.fi>
 *
 * This software may be distributed under the terms of the BSD license.
 * See README for more details.
 */

#include "hostap-wpa3/src/utils/includes.h"
#include "hostap-wpa3/src/utils/common.h"
#include "hostap-wpa3/src/crypto/crypto.h"
#include "hostap-wpa3/src/crypto/dh_groups.h"
#include "hostap-wpa3/src/common/sae.h"


int main(int argc, char *argv[]) {
	const u8 addr1[6] = {0, 17, 0, 0, 0, 0}; //wlan0 target address 11 base 16 = 17 base 10
	u8 addr2[6] = {0, 51, 0, 0, 0, 0}; //dragontime address 33 base 16 = 51 base 10
	u8 i;
	int j;
	int address_num = 256;
	struct sae_data *mysae = (struct sae_data *) os_zalloc(sizeof(struct sae_data));
	struct sae_temporary_data *mytmp = (struct sae_temporary_data *) os_zalloc(sizeof(struct sae_temporary_data));
	u8 order24[32] = {
		140, 248, 54, 66, 167, 9, 160, 151, 180, 71, 153, 118, 64, 18, 157, 162, 153, 177, 164, 125, 30, 179, 117, 11,
		163, 8, 176, 254, 100, 245, 251, 211
	};
	u8 prime24[256] = {
		135, 168, 230, 29, 180, 182, 102, 60, 255, 187, 209, 156, 101, 25, 89, 153, 140, 238, 246, 8, 102, 13, 208, 242,
		93, 44, 238, 212, 67, 94, 59, 0, 224, 13, 248, 241, 214, 25, 87, 212, 250, 247, 223, 69, 97, 178, 170, 48, 22,
		195, 217, 17, 52, 9, 111, 170, 59, 244, 41, 109, 131, 14, 154, 124, 32, 158, 12, 100, 151, 81, 122, 189, 90,
		138, 157, 48, 107, 207, 103, 237, 145, 249, 230, 114, 91, 71, 88, 192, 34, 224, 177, 239, 66, 117, 191, 123,
		108, 91, 252, 17, 212, 95, 144, 136, 185, 65, 245, 78, 177, 229, 155, 184, 188, 57, 160, 191, 18, 48, 127, 92,
		79, 219, 112, 197, 129, 178, 63, 118, 182, 58, 202, 225, 202, 166, 183, 144, 45, 82, 82, 103, 53, 72, 138, 14,
		241, 60, 109, 154, 81, 191, 164, 171, 58, 216, 52, 119, 150, 82, 77, 142, 246, 161, 103, 181, 164, 24, 37, 217,
		103, 225, 68, 229, 20, 5, 100, 37, 28, 202, 203, 131, 230, 180, 134, 246, 179, 202, 63, 121, 113, 80, 96, 38,
		192, 184, 87, 246, 137, 150, 40, 86, 222, 212, 1, 10, 189, 11, 230, 33, 195, 163, 150, 10, 84, 231, 16, 195,
		117, 242, 99, 117, 215, 1, 65, 3, 164, 181, 67, 48, 193, 152, 175, 18, 97, 22, 210, 39, 110, 17, 113, 95, 105,
		56, 119, 250, 215, 239, 9, 202, 219, 9, 74, 233, 30, 26, 21, 151
	};
	struct dh_group mydh_group = {0, 0, 0, prime24, 256, order24, 32, 0};

	FILE *pas_fd = fopen(argv[1], "r");
	FILE *out_fd = fopen(argv[2], "w");
	char line[256];

	const u8 password[8] = {'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h'}; //61 62 63 64 65 66 67 68 in hex!
	mysae->tmp = mytmp;
	mysae->tmp->dh = &mydh_group;
	mysae->tmp->prime_len = 256; //2048 bits
	//pwe_ffc is redefined, so it can be null
	mysae->tmp->order = crypto_bignum_init_set(order24, 32);
	//mysae->tmp->dh->prime=prime24;
	mysae->tmp->prime = crypto_bignum_init_set(prime24, 256);
	i = 0;
	int iters;

	if (pas_fd != NULL) {
		// Read each line from the file and store it in the
		// 'line' buffer.
		while (fgets(line, sizeof(line), pas_fd)) {
			line[os_strlen(line) - 1] = '\0';
			fprintf(out_fd, "%s,", line);
			// Print each line to the standard output.
			for (j = 0; j < address_num; j++) {
				//2, 1, 2, 1, 1, 1, 1, 2, 3, 1, 2, 3, 1, 1, 1, 1, 9, 1, 1, 2 for all default
				addr2[5] = i;
				iters = sae_derive_pwe_ffc(mysae, addr1, addr2, (u8 *) line, os_strlen(line));
				fprintf(out_fd, " %u", iters);
				if (j < address_num - 1) fprintf(out_fd, ",");
				i++;
			}
			fprintf(out_fd, "\n");
		}

		// Close the file stream once all lines have been
		// read.
		fclose(pas_fd);
	} else {
		// Print an error message to the standard error
		// stream if the file cannot be opened.
		fprintf(stderr, "Unable to open password file!\n");
	}
	return 0;
}
