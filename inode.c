#include "inode.h"
#include "block.h"
#include "free.h"
#include "pack.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static struct inode incore[MAX_SYS_OPEN_FILES] = {0};

struct inode *ialloc(void)
{
    unsigned char inode_block[BLOCK_SIZE] = {0};
    int free_bit_num;

    bread(FREE_INODE_BLOCK_NUM, inode_block);
    free_bit_num = find_free(inode_block);

    if (free_bit_num != -1)
    {
        set_free(inode_block, free_bit_num, 1);
        bwrite(FREE_INODE_BLOCK_NUM, inode_block);
        struct inode *incore_node = iget(free_bit_num);
        return incore_node;
    }

    return NULL;
}


struct inode *find_incore_free(void)
{
    for (int i = 0; i < MAX_SYS_OPEN_FILES; i++)
    {
        if (incore[i].ref_count == 0)
        {
            return &incore[i];
        }
    }
    return NULL;
}

struct inode *find_incore(unsigned int inode_num)
{
    for (int i = 0; i < MAX_SYS_OPEN_FILES; i++)
    {
        if (incore[i].ref_count != 0 && incore[i].inode_num == inode_num)
        {
            return &incore[i];
        }
    }
    return NULL;
}

void read_inode_block(unsigned char *inode_block, struct inode *in, int block_offset)
{
    int block_offset_bytes = block_offset * INODE_SIZE;

    in->size = read_u32(inode_block + block_offset_bytes + SIZE_OFFSET);
    in->owner_id = read_u16(inode_block + block_offset_bytes + ID_OFFSET);
    in->permissions = read_u8(inode_block + block_offset_bytes + PERMISSIONS_OFFSET);
    in->flags = read_u8(inode_block + block_offset_bytes + FLAGS_OFFSET);
    in->link_count = read_u8(inode_block + block_offset_bytes + LINK_COUNT_OFFSET);

    for (int i = 0; i < INODE_PTR_COUNT; i++)
    {
        in->block_ptr[i] = read_u16(inode_block + block_offset_bytes + BLOCK_PTR_OFFSET + (BYTES_PER_BLOCK_PTR * i));
    }
}

void read_inode(struct inode *in, int inode_num)
{
    unsigned char inode_block[BLOCK_SIZE] = {0};

    int block_num = inode_num / INODE_SIZE + INODE_FIRST_BLOCK;
    int block_offset = inode_num % INODE_SIZE;

    bread(block_num, inode_block);
    read_inode_block(inode_block, in, block_offset);
}

void write_inode_block(unsigned char *inode_block, struct inode *in, int block_offset)
{
    int block_offset_bytes = block_offset * INODE_SIZE;

    write_u32(inode_block + block_offset_bytes + SIZE_OFFSET, in->size);
    write_u16(inode_block + block_offset_bytes + ID_OFFSET, in->owner_id);
    write_u8(inode_block + block_offset_bytes + PERMISSIONS_OFFSET, in->permissions);
    write_u8(inode_block + block_offset_bytes + FLAGS_OFFSET, in->flags);
    write_u8(inode_block + block_offset_bytes + LINK_COUNT_OFFSET, in->link_count);

    for (int i = 0; i < INODE_PTR_COUNT; i++)
    {
        write_u16(inode_block + block_offset_bytes + BLOCK_PTR_OFFSET + (BYTES_PER_BLOCK_PTR * i), in->block_ptr[i]);
    }
}

void write_inode(struct inode *in)
{
    unsigned char inode_block[BLOCK_SIZE] = {0};

    int inode_num = in->inode_num;
    int block_num = inode_num / INODE_SIZE + INODE_FIRST_BLOCK;
    int block_offset = inode_num % INODE_SIZE;

    bread(block_num, inode_block);
    write_inode_block(inode_block, in, block_offset);
    bwrite(block_num, inode_block);
}

struct inode *iget(int inode_num)
{
    struct inode *incore_node = find_incore(inode_num);
    if (incore_node != NULL)
    {
        incore_node->ref_count++;
        return incore_node;
    }

    struct inode *free_node = find_incore_free();
    if (free_node == NULL)
    {
        return NULL;
    }

    read_inode(free_node, inode_num);
    free_node->ref_count = 1;
    free_node->inode_num = inode_num;
    return free_node;
}

void iput(struct inode *in)
{
    if (in->ref_count == 0)
    {
        return;
    }

    in->ref_count--;
    if (in->ref_count == 0)
    {
        write_inode(in);
    }
}

void clear_incore(void)
{
    memset(incore, 0, sizeof(incore));
}
