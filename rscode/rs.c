#include "rs.h"
#include "rs.c"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DATA_SHARDS 128
#define PARITY_SHARDS 7
#define TOTAL_SHARDS (DATA_SHARDS + PARITY_SHARDS)
#define BLOCK_SIZE 32

void print_block(unsigned char *block, int block_size, const char *label, int block_num) {
    printf("%s (block %d): ", label, block_num);
    for (int j = 0; j < block_size; j++) {
        printf("%02x ", block[j]);
    }
    printf("\n");
}

double get_time_ms(struct timespec start, struct timespec end) {
    return (end.tv_sec - start.tv_sec) * 1000.0 + (end.tv_nsec - start.tv_nsec) / 1000000.0;
}

int init_shards(unsigned char ***shards, unsigned char **original_data, size_t *dynamic_memory) {
    *shards = malloc(TOTAL_SHARDS * sizeof(unsigned char*));
    if (*shards == NULL) {
        fprintf(stderr, "Failed to allocate memory for shards\n");
        return 1;
    }
    *dynamic_memory = TOTAL_SHARDS * sizeof(unsigned char*);
    for (int i = 0; i < TOTAL_SHARDS; i++) {
        (*shards)[i] = malloc(BLOCK_SIZE);
        if ((*shards)[i] == NULL) {
            fprintf(stderr, "Failed to allocate memory for shard %d\n", i);
            for (int j = 0; j < i; j++) free((*shards)[j]);
            free(*shards);
            return 1;
        }
        *dynamic_memory += BLOCK_SIZE;
    }

    *original_data = malloc(DATA_SHARDS * BLOCK_SIZE);
    if (*original_data == NULL) {
        fprintf(stderr, "Failed to allocate memory for original_data\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free((*shards)[i]);
        free(*shards);
        return 1;
    }
    *dynamic_memory += DATA_SHARDS * BLOCK_SIZE;

    for (int i = 0; i < DATA_SHARDS; i++) {
        for (int j = 0; j < BLOCK_SIZE; j++) {
            (*shards)[i][j] = (unsigned char)(i * BLOCK_SIZE + j);
            (*original_data)[i * BLOCK_SIZE + j] = (*shards)[i][j];
        }
    }
    return 0;
}

void test_no_errors() {
    printf("\n=== Test 1: No Errors ===\n");

    reed_solomon *rs = reed_solomon_new(DATA_SHARDS, PARITY_SHARDS);
    if (rs == NULL) {
        fprintf(stderr, "Failed to create reed_solomon\n");
        return;
    }

    size_t static_memory = DATA_SHARDS * BLOCK_SIZE + PARITY_SHARDS * BLOCK_SIZE;
    printf("Static memory: %zu bytes\n", static_memory);

    size_t dynamic_memory = 0;
    unsigned char **shards = NULL;
    unsigned char *original_data = NULL;
    if (init_shards(&shards, &original_data, &dynamic_memory)) {
        reed_solomon_release(rs);
        return;
    }

    unsigned char *zilch = calloc(TOTAL_SHARDS, 1);
    if (zilch == NULL) {
        fprintf(stderr, "Failed to allocate memory for zilch\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
        free(shards);
        free(original_data);
        reed_solomon_release(rs);
        return;
    }
    dynamic_memory += TOTAL_SHARDS;

    dynamic_memory += DATA_SHARDS * TOTAL_SHARDS + PARITY_SHARDS * DATA_SHARDS;
    printf("Dynamic memory: %zu bytes\n", dynamic_memory);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    unsigned char **data_blocks = shards;
    unsigned char **fec_blocks = &shards[DATA_SHARDS];
    int ret = reed_solomon_encode(rs, data_blocks, fec_blocks, BLOCK_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double encode_time = get_time_ms(start, end);
    printf("Encoding time: %.3f ms\n", encode_time * BLOCK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Encoding failed\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
        free(shards);
        free(original_data);
        free(zilch);
        reed_solomon_release(rs);
        return;
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    ret = reed_solomon_reconstruct(rs, shards, zilch, TOTAL_SHARDS, BLOCK_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double reconstruct_time = get_time_ms(start, end);
    printf("Reconstruction time: %.3f ms\n", reconstruct_time * BLOCK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Reconstruction failed\n");
    } else {
        printf("Reconstruction successful\n");
        int errors = 0;
        for (int i = 0; i < DATA_SHARDS; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (shards[i][j] != original_data[i * BLOCK_SIZE + j]) {
                    errors++;
                    printf("Error in block %d, byte %d: expected %02x, got %02x\n",
                           i, j, original_data[i * BLOCK_SIZE + j], shards[i][j]);
                }
            }
        }
        printf(errors == 0 ? "All data recovered correctly\n" : "Found %d errors in recovered data\n", errors);
    }

    for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
    free(shards);
    free(original_data);
    free(zilch);
    reed_solomon_release(rs);
}

void test_erasures() {
    printf("\n=== Test 2: Erasures ===\n");

    reed_solomon *rs = reed_solomon_new(DATA_SHARDS, PARITY_SHARDS);
    if (rs == NULL) {
        fprintf(stderr, "Failed to create reed_solomon\n");
        return;
    }

    size_t static_memory = DATA_SHARDS * BLOCK_SIZE + PARITY_SHARDS * BLOCK_SIZE;
    printf("Static memory: %zu bytes\n", static_memory);

    size_t dynamic_memory = 0;
    unsigned char **shards = NULL;
    unsigned char *original_data = NULL;
    if (init_shards(&shards, &original_data, &dynamic_memory)) {
        reed_solomon_release(rs);
        return;
    }

    unsigned char *zilch = calloc(TOTAL_SHARDS, 1);
    if (zilch == NULL) {
        fprintf(stderr, "Failed to allocate memory for zilch\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
        free(shards);
        free(original_data);
        reed_solomon_release(rs);
        return;
    }
    dynamic_memory += TOTAL_SHARDS;

    dynamic_memory += DATA_SHARDS * TOTAL_SHARDS + PARITY_SHARDS * DATA_SHARDS;
    printf("Dynamic memory: %zu bytes\n", dynamic_memory);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    unsigned char **data_blocks = shards;
    unsigned char **fec_blocks = &shards[DATA_SHARDS];
    int ret = reed_solomon_encode(rs, data_blocks, fec_blocks, BLOCK_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double encode_time = get_time_ms(start, end);
    printf("Encoding time: %.3f ms\n", encode_time * BLOCK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Encoding failed\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
        free(shards);
        free(original_data);
        free(zilch);
        reed_solomon_release(rs);
        return;
    }

    int num_error_blocks = 3;
    printf("Simulating %d erasures (whole blocks set to zero):\n", num_error_blocks);
    for (int i = 0; i < num_error_blocks && i < DATA_SHARDS; i++) {
        print_block(shards[i], BLOCK_SIZE, "Block before erasure", i);
        memset(shards[i], 0, BLOCK_SIZE);
        zilch[i] = 1;
        print_block(shards[i], BLOCK_SIZE, "Block after erasure", i);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    ret = reed_solomon_reconstruct(rs, shards, zilch, TOTAL_SHARDS, BLOCK_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double reconstruct_time = get_time_ms(start, end);
    printf("Reconstruction time: %.3f ms\n", reconstruct_time * BLOCK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Reconstruction failed\n");
    } else {
        printf("Reconstruction successful\n");
        int errors = 0;
        for (int i = 0; i < DATA_SHARDS; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (shards[i][j] != original_data[i * BLOCK_SIZE + j]) {
                    errors++;
                    printf("Error in block %d, byte %d: expected %02x, got %02x\n",
                           i, j, original_data[i * BLOCK_SIZE + j], shards[i][j]);
                }
            }
            if (i < num_error_blocks) {
                print_block(shards[i], BLOCK_SIZE, "Recovered block", i);
            }
        }
        printf(errors == 0 ? "All data recovered correctly\n" : "Found %d errors in recovered data\n", errors);
    }

    for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
    free(shards);
    free(original_data);
    free(zilch);
    reed_solomon_release(rs);
}

void test_random_errors() {
    printf("\n=== Test 3: Random Errors ===\n");

    srand(time(NULL));

    reed_solomon *rs = reed_solomon_new(DATA_SHARDS, PARITY_SHARDS);
    if (rs == NULL) {
        fprintf(stderr, "Failed to create reed_solomon\n");
        return;
    }

    size_t static_memory = DATA_SHARDS * BLOCK_SIZE + PARITY_SHARDS * BLOCK_SIZE;
    printf("Static memory: %zu bytes\n", static_memory);

    size_t dynamic_memory = 0;
    unsigned char **shards = NULL;
    unsigned char *original_data = NULL;
    if (init_shards(&shards, &original_data, &dynamic_memory)) {
        reed_solomon_release(rs);
        return;
    }

    unsigned char *zilch = calloc(TOTAL_SHARDS, 1);
    if (zilch == NULL) {
        fprintf(stderr, "Failed to allocate memory for zilch\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
        free(shards);
        free(original_data);
        reed_solomon_release(rs);
        return;
    }
    dynamic_memory += TOTAL_SHARDS;

    dynamic_memory += DATA_SHARDS * TOTAL_SHARDS + PARITY_SHARDS * DATA_SHARDS;
    printf("Dynamic memory: %zu bytes\n", dynamic_memory);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);
    unsigned char **data_blocks = shards;
    unsigned char **fec_blocks = &shards[DATA_SHARDS];
    int ret = reed_solomon_encode(rs, data_blocks, fec_blocks, BLOCK_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double encode_time = get_time_ms(start, end);
    printf("Encoding time: %.3f ms\n", encode_time * BLOCK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Encoding failed\n");
        for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
        free(shards);
        free(original_data);
        free(zilch);
        reed_solomon_release(rs);
        return;
    }

    int num_error_blocks = 3;
    int errors_per_block = 5;
    printf("Simulating %d random errors in %d blocks:\n", errors_per_block, num_error_blocks);
    for (int i = 0; i < num_error_blocks && i < DATA_SHARDS; i++) {
        print_block(shards[i], BLOCK_SIZE, "Block before errors", i);
        for (int e = 0; e < errors_per_block; e++) {
            int byte_idx = rand() % BLOCK_SIZE;
            unsigned char original = shards[i][byte_idx];
            shards[i][byte_idx] = (unsigned char)(rand() % 256);
            printf("Block %d, byte %d: changed from %02x to %02x\n", i, byte_idx, original, shards[i][byte_idx]);
        }
        zilch[i] = 1;
        print_block(shards[i], BLOCK_SIZE, "Block after errors", i);
    }

    clock_gettime(CLOCK_MONOTONIC, &start);
    ret = reed_solomon_reconstruct(rs, shards, zilch, TOTAL_SHARDS, BLOCK_SIZE);
    clock_gettime(CLOCK_MONOTONIC, &end);
    double reconstruct_time = get_time_ms(start, end);
    printf("Reconstruction time: %.3f ms\n", reconstruct_time * BLOCK_SIZE);
    if (ret != 0) {
        fprintf(stderr, "Reconstruction failed\n");
    } else {
        printf("Reconstruction successful\n");
        int errors = 0;
        for (int i = 0; i < DATA_SHARDS; i++) {
            for (int j = 0; j < BLOCK_SIZE; j++) {
                if (shards[i][j] != original_data[i * BLOCK_SIZE + j]) {
                    errors++;
                    printf("Error in block %d, byte %d: expected %02x, got %02x\n",
                           i, j, original_data[i * BLOCK_SIZE + j], shards[i][j]);
                }
            }
            if (i < num_error_blocks) {
                print_block(shards[i], BLOCK_SIZE, "Recovered block", i);
            }
        }
        printf(errors == 0 ? "All data recovered correctly\n" : "Found %d errors in recovered data\n", errors);
    }

    for (int i = 0; i < TOTAL_SHARDS; i++) free(shards[i]);
    free(shards);
    free(original_data);
    free(zilch);
    reed_solomon_release(rs);
}

int main() {
    fec_init();

    test_no_errors();
    test_erasures();
    test_random_errors();

    return 0;
}
