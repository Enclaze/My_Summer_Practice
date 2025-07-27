#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define PAGE_SIZE 4096
#define SPARE_SIZE 224
#define BLOCK_SIZE 512

int calc_parity_bits(int m);

uint8_t* encode_block(uint8_t *data, int len, int *ecc_len_out);

int decode_block(uint8_t *code, int m, int r, uint8_t *data_out);

void inject_error_in_data_bit(uint8_t *ecc_block, int m, int r, int data_bit_index);