#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "hamming.h"
#include "hamming.c"

#define PAGE_SIZE 4096
#define BLOCK_SIZE 512
#define SPARE_SIZE 224

// no errors
void test1() {
    printf("\n====== Test 1 Results ======\n");
    uint8_t page[PAGE_SIZE];
    uint8_t spare[SPARE_SIZE] = {0};
    srand(time(NULL));
    for (int i = 0; i < PAGE_SIZE; i++) page[i] = rand() % 256;

    int total_blocks = PAGE_SIZE / BLOCK_SIZE;
    int m = BLOCK_SIZE * 8;
    int r = calc_parity_bits(m);
    int ecc_len = (m + r + 7) / 8;

    clock_t encode_start = clock();
    uint8_t *ecc_array = malloc(total_blocks * ecc_len);
    for (int i = 0; i < total_blocks; i++) {
        int out_len;
        uint8_t *ecc = encode_block(&page[i * BLOCK_SIZE], BLOCK_SIZE, &out_len);
        memcpy(&ecc_array[i * ecc_len], ecc, out_len);
        free(ecc);
    }
    clock_t encode_end = clock();

    clock_t decode_start = clock();
    int corrected_errors = 0;
    uint8_t decoded_block[BLOCK_SIZE];
    for (int i = 0; i < total_blocks; i++) {
        int error_pos = decode_block(&ecc_array[i * ecc_len], m, r, decoded_block);
        if (error_pos > 0) {
            printf("Block %d: corrected error in bit %d\n", i, error_pos);
            corrected_errors++;
        }
        if (memcmp(decoded_block, &page[i * BLOCK_SIZE], BLOCK_SIZE) != 0) {
            printf("AaAAahAAHhHHAHAH");
        }
    }
    clock_t decode_end = clock();

    printf("Errors Corrected:       %d\n", corrected_errors);
    printf("Encoding Time:       %.2f ms\n", (double)(encode_end - encode_start) * 1000 / CLOCKS_PER_SEC);
    printf("Decoding Time:     %.2f ms\n", (double)(decode_end - decode_start) * 1000 / CLOCKS_PER_SEC);
    printf("Static Memory Used:     %zu bytes\n", PAGE_SIZE + (total_blocks * r + 7) / 8);
    printf("Dynamic Memory Used:     %zu bytes\n", total_blocks * (ecc_len + 1));
    printf("============================\n");

    free(ecc_array);
}

// 1 error in 1 block
void test2() {
    printf("\n====== Test 2 Results ======\n");
    uint8_t page[PAGE_SIZE];
    uint8_t spare[SPARE_SIZE] = {0};
    srand(time(NULL));
    for (int i = 0; i < PAGE_SIZE; i++) page[i] = rand() % 256;

    int total_blocks = PAGE_SIZE / BLOCK_SIZE;
    int m = BLOCK_SIZE * 8;
    int r = calc_parity_bits(m);
    int ecc_len = (m + r + 7) / 8;

    clock_t encode_start = clock();
    uint8_t *ecc_array = malloc(total_blocks * ecc_len);
    for (int i = 0; i < total_blocks; i++) {
        int out_len;
        uint8_t *ecc = encode_block(&page[i * BLOCK_SIZE], BLOCK_SIZE, &out_len);
        memcpy(&ecc_array[i * ecc_len], ecc, out_len);
        free(ecc);
    }
    clock_t encode_end = clock();

    int corrupted_block = rand() % total_blocks;
    int bit_pos = rand() % m;
    inject_error_in_data_bit(&ecc_array[corrupted_block * ecc_len], m, r, bit_pos);
    printf("Added error in block %d, bit %d\n", corrupted_block, bit_pos);

    clock_t decode_start = clock();
    int corrected_errors = 0;
    uint8_t decoded_block[BLOCK_SIZE];
    for (int i = 0; i < total_blocks; i++) {
        int error_pos = decode_block(&ecc_array[i * ecc_len], m, r, decoded_block);
        if (error_pos > 0) {
            printf("Block %d: corrected error in bit %d\n", i, error_pos);
            corrected_errors++;
        }
        if (memcmp(decoded_block, &page[i * BLOCK_SIZE], BLOCK_SIZE) != 0) {
            printf("AaAAahAAHhHHAHAH\n");
        }
    }
    clock_t decode_end = clock();

    printf("Errors Corrected:       %d\n", corrected_errors);
    printf("Encoding Time:       %.2f ms\n", (double)(encode_end - encode_start) * 1000 / CLOCKS_PER_SEC);
    printf("Decoding Time:     %.2f ms\n", (double)(decode_end - decode_start) * 1000 / CLOCKS_PER_SEC);
    printf("Static Memory Used:     %zu bytes\n", PAGE_SIZE + (total_blocks * r + 7) / 8);
    printf("Dynamic Memory Used:     %zu bytes\n", total_blocks * (ecc_len + 1));
    printf("============================\n");
    free(ecc_array);
}

// 1 error in each block
void test3() {
    printf("\n====== Test 3 Results ======\n");
    uint8_t page[PAGE_SIZE];
    uint8_t spare[SPARE_SIZE] = {0};
    srand(time(NULL));
    for (int i = 0; i < PAGE_SIZE; i++) page[i] = rand() % 256;

    int total_blocks = PAGE_SIZE / BLOCK_SIZE;
    int m = BLOCK_SIZE * 8;
    int r = calc_parity_bits(m);
    int ecc_len = (m + r + 7) / 8;

    clock_t encode_start = clock();
    uint8_t *ecc_array = malloc(total_blocks * ecc_len);
    for (int i = 0; i < total_blocks; i++) {
        int out_len;
        uint8_t *ecc = encode_block(&page[i * BLOCK_SIZE], BLOCK_SIZE, &out_len);
        memcpy(&ecc_array[i * ecc_len], ecc, out_len);
        free(ecc);
    }
    clock_t encode_end = clock();

    for (int i = 0; i < total_blocks; i++) {
        int bit_pos = rand() % m;
        inject_error_in_data_bit(&ecc_array[i * ecc_len], m, r, bit_pos);
        printf("Added error in block %d, bit %d\n", i, bit_pos);
    }

    clock_t decode_start = clock();
    int corrected_errors = 0;
    uint8_t decoded_block[BLOCK_SIZE];
    for (int i = 0; i < total_blocks; i++) {
        int error_pos = decode_block(&ecc_array[i * ecc_len], m, r, decoded_block);
        if (error_pos > 0) {
            printf("Block %d: corrected error in bit %d\n", i, error_pos);
            corrected_errors++;
        }
        if (memcmp(decoded_block, &page[i * BLOCK_SIZE], BLOCK_SIZE) != 0) {
            printf("AaAAahAAHhHHAHAH\n");
        }
    }
    clock_t decode_end = clock();

    printf("Errors Corrected:       %d\n", corrected_errors);
    printf("Encoding Time:       %.2f ms\n", (double)(encode_end - encode_start) * 1000 / CLOCKS_PER_SEC);
    printf("Decoding Time:     %.2f ms\n", (double)(decode_end - decode_start) * 1000 / CLOCKS_PER_SEC);
    printf("Static Memory Used:     %zu bytes\n", PAGE_SIZE + (total_blocks * r + 7) / 8);
    printf("Dynamic Memory Used:     %zu bytes\n", total_blocks * (ecc_len + 1));
    printf("============================\n");
    free(ecc_array);
}

int main() {
    test1();
    test2();
    test3();
    return 0;
}