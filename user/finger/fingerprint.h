#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include <openssl/ec.h>

#define u8 u_int8_t
#define u16 u_int16_t
#define u32 u_int32_t

#define SAE_MAX_HASH_LEN 64
#define ETH_ALEN 6
#define SAE_PMK_LEN 32
#define SAE_PMKID_LEN 16


struct dh_group {
	const u8 *prime;
	size_t prime_len;
	const u8 *order;
	size_t order_len;
};

struct sae_temporary_data {
	int prime_len;
	int order_len;
	const struct dh_group *dh;
	const struct crypto_bignum *prime;
	const struct crypto_bignum *order;
};

struct sae_data {
	struct sae_temporary_data *tmp;
};

static int sae_derive_pwe_ffc(struct sae_data *sae, const u8 *addr1, const u8 *addr2, const u8 *password,
                              size_t password_len);

struct crypto_bignum *crypto_bignum_init_set(const u8 *buf, size_t len);

#endif //FINGERPRINT_H
