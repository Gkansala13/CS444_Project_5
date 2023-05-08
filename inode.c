#include "inode.h"
#include "block.h"
#include "free.h"

int ialloc(void)
{
    unsigned char inode_block[BLOCK_SIZE] = { 0 };

    bread(FREE_INODE_BLOCK_NUM, inode_block);
    int free_bit_num = find_free(inode_block);

    if (free_bit_num != -1) {
        set_free(inode_block, free_bit_num, 1);
        bwrite(FREE_INODE_BLOCK_NUM, inode_block);
        return free_bit_num;
    }

    return -1;
}
