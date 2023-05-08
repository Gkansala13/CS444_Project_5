#include "block.h"
#include "image.h"
#include "free.h"
#include <unistd.h>

unsigned char *bread(int block_num, unsigned char *block) {
    off_t byte_offset = (block_num * BLOCK_SIZE);
    lseek(image_fd, byte_offset, SEEK_SET);
    read(image_fd, block, BLOCK_SIZE); 
    return block;
}

void bwrite(int block_num, unsigned char *block) {
    off_t byte_offset = (block_num * BLOCK_SIZE);
    lseek(image_fd, byte_offset, SEEK_SET);
    write(image_fd, block, BLOCK_SIZE);
    return;
}

int alloc(void) {
    unsigned char data_block[BLOCK_SIZE] = { 0 };
    int free_bit_num;
    bread(FREE_DATA_BLOCK_NUM, data_block);
    free_bit_num = find_free(data_block);
    if (free_bit_num != -1) {
        set_free(data_block, free_bit_num, 1);
    }
    bwrite(FREE_DATA_BLOCK_NUM, data_block);
    return free_bit_num;
}
