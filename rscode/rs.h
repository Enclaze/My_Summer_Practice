#ifndef __RS_H_
#define __RS_H_

#ifndef DATA_SHARDS_MAX
#define DATA_SHARDS_MAX (255)
#endif

#ifndef RS_MALLOC
#define RS_MALLOC(x)    malloc(x)
#endif

#ifndef RS_FREE
#define RS_FREE(x)      free(x)
#endif

#ifndef RS_CALLOC
#define RS_CALLOC(n, x) calloc(n, x)
#endif

typedef struct _reed_solomon {
    int data_shards;
    int parity_shards;
    int shards;
    unsigned char* m;
    unsigned char* parity;
} reed_solomon;

void fec_init(void);

reed_solomon* reed_solomon_new(int data_shards, int parity_shards);
void reed_solomon_release(reed_solomon* rs);

int reed_solomon_encode(reed_solomon* rs,
        unsigned char** data_blocks,
        unsigned char** fec_blocks,
        int block_size);

int reed_solomon_decode(reed_solomon* rs,
        unsigned char **data_blocks,
        int block_size,
        unsigned char **dec_fec_blocks,
        unsigned int *fec_block_nos,
        unsigned int *erased_blocks,
        int nr_fec_blocks);

int reed_solomon_encode2(reed_solomon* rs, unsigned char** shards, int nr_shards, int block_size);

int reed_solomon_reconstruct(reed_solomon* rs, unsigned char** shards, unsigned char* marks, int nr_shards, int block_size);
#endif