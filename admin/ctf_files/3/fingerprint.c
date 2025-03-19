#include <stdlib.h>
#include <stdio.h>

#include "fingerprint.h"

// Process a list of passwords and generate a fingerprint for each of them
int main(int argc, char *argv[]) {
    if (argc != 3) {
        exit(1);
    }

    // 1. Open password dictionary and output csv
    FILE *pas_fd = fopen(argv[1], "r");
    FILE *out_fd = fopen(argv[2], "w");

    // 2. Calculate two addresses
    u8 addr1[6] = {/* TODO */};
    u8 addr2[6] = {/* TODO */};

    // 3. Define DH group, with prime, prime_len, order and order_len
    struct dh_group *my_dh_group = calloc(1, sizeof(struct dh_group));
    my_dh_group->prime = /* TODO */;
    my_dh_group->prime_len = /* TODO */;
    my_dh_group->order = /* TODO */;
    my_dh_group->order_len = /* TODO */;

    // 4. Define SAE temporary data, with prime_len, order, prime and dh
    struct sae_temporary_data *my_tmp = calloc(1, sizeof(struct sae_temporary_data));
    my_tmp->order_len = my_dh_group->order_len;
    my_tmp->prime_len = my_dh_group->prime_len;
    my_tmp->order = crypto_bignum_init_set(my_dh_group->order, my_dh_group->order_len);
    my_tmp->prime = crypto_bignum_init_set(my_dh_group->prime, my_dh_group->prime_len);
    my_tmp->dh = my_dh_group;

    // 5. Define SAE data, with tmp
    struct sae_data *my_sae = calloc(1, sizeof(struct sae_data));
    my_sae->tmp = my_tmp;

    // 6. For each password from the dictionary:
    //    Generate a fingerprint, consisting of the iterations amount of sae_derive_pwe_ffc (stored in "counter")
    //    for each MAC address
    /* TODO */
}

