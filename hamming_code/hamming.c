#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hamming.h"

#define PAGE_SIZE 4096
#define SPARE_SIZE 224
#define BLOCK_SIZE 512

int calc_parity_bits(int m) {
    int r = 0;
    while ((1 << r) < (m + r + 1)) r++;
    return r;
}

uint8_t* encode_block(uint8_t *data, int len, int *ecc_len_out) {
    int m = len * 8;
    int r = calc_parity_bits(m);
    int n = m + r;
    uint8_t *code = calloc((n + 7) / 8, 1);
    if (!code) return NULL;
    int j = 0;

    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) continue;
        int byte = j / 8, bit = j % 8;
        if (data[byte] & (1 << bit))
            code[(i - 1) / 8] |= 1 << ((i - 1) % 8);
        j++;
    }

    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int j = 1; j <= n; j++) {
            if (j & pos) {
                if (code[(j - 1) / 8] & (1 << ((j - 1) % 8))) {
                    parity ^= 1;
                }
            }
        }
        if (parity)
            code[(pos - 1) / 8] |= 1 << ((pos - 1) % 8);
    }
    *ecc_len_out = (n + 7) / 8;
    return code;
}

int decode_block(uint8_t *code, int m, int r, uint8_t *data_out) {
    int n = m + r;
    int error_pos = 0;

    for (int i = 0; i < r; i++) {
        int pos = 1 << i;
        int parity = 0;
        for (int j = 1; j <= n; j++) {
            if (j & pos) {
                if (code[(j - 1) / 8] & (1 << ((j - 1) % 8)))
                    parity ^= 1;
            }
        }
        if (parity)
            error_pos += pos;
    }

    if (error_pos > 0 && error_pos <= n)
        code[(error_pos - 1) / 8] ^= 1 << ((error_pos - 1) % 8);

    int j = 0;
    memset(data_out, 0, m / 8);
    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) continue;
        if (code[(i - 1) / 8] & (1 << ((i - 1) % 8)))
            data_out[j / 8] |= 1 << (j % 8);
        j++;
    }

    return error_pos;
}

void inject_error_in_data_bit(uint8_t *ecc_block, int m, int r, int data_bit_index) {
    int n = m + r;
    for (int i = 1; i <= n; i++) {
        if ((i & (i - 1)) == 0) continue;
        if (i == data_bit_index) {
            ecc_block[(i - 1) / 8] ^= 1 << ((i - 1) % 8);
            return;
        }
    }
    printf("Index %d out of bounds 0..%d\n", data_bit_index, m - 1);
}