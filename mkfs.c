#include "mkfs.h"
#include "block.h"
#include "image.h"

void mkfs(void)
{
    unsigned char zero_block[BLOCK_SIZE] = { 0 };

    for (int i = 0; i < NUMBER_OF_BLOCKS; i++) {
        bwrite(i, zero_block);
    }

    for (int i = 0; i < 7; i++) {
        alloc();
    }
}

