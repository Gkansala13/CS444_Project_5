#include "mkfs.h"
#include "block.h"
#include "image.h"
#include "inode.h"
#include "pack.h"
#include "ls.h"
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

void initialize_blocks(void)
{
    unsigned char zero_block[BLOCK_SIZE * NUMBER_OF_BLOCKS] = { 0 };
    write(image_fd, zero_block, BLOCK_SIZE * NUMBER_OF_BLOCKS);

    for (int i = 0; i < 7; i++)
    {
        alloc();
    }
}

struct inode *create_root_directory(void)
{
    struct inode *root_inode = ialloc();
    int root_block_num = alloc();
    root_inode->flags = DIR_FLAG;
    root_inode->size = DIR_START_SIZE;
    root_inode->block_ptr[0] = root_block_num;

    unsigned char block[BLOCK_SIZE] = { 0 };
    write_u16(block, root_inode->inode_num);
    strcpy((char *)(block + FILENAME_OFFSET), ".");
    write_u16(block + DIR_ENTRY_SIZE, root_inode->inode_num);
    strcpy((char *)(block + DIR_ENTRY_SIZE + FILENAME_OFFSET), "..");
    bwrite(root_block_num, block);

    return root_inode;
}

void mkfs(void)
{
    initialize_blocks();
    struct inode *root_inode = create_root_directory();
    iput(root_inode);
}

struct directory *directory_open(int inode_num)
{
    struct inode *dir_inode = iget(inode_num);
    if (dir_inode == NULL)
    {
        return NULL;
    }

    struct directory *dir = malloc(sizeof(struct directory));
    dir->inode = dir_inode;
    dir->offset = 0;
    return dir;
}

int directory_get(struct directory *dir, struct directory_entry *ent)
{
    unsigned char block[BLOCK_SIZE];

    struct inode *dir_inode = dir->inode;
    int dir_size = dir_inode->size;
    if ((int)dir->offset >= dir_size)
    {
        return -1;
    }

    int data_block_index = dir->offset / BLOCK_SIZE;
    int data_block_num = dir_inode->block_ptr[data_block_index];
    bread(data_block_num, block);

    int offset_in_block = dir->offset % BLOCK_SIZE;
    ent->inode_num = read_u16(block + offset_in_block);
    strcpy(ent->name, (char *)(block + offset_in_block + FILENAME_OFFSET));

    dir->offset += DIR_ENTRY_SIZE;
    return 1;
}

void directory_close(struct directory *dir)
{
    iput(dir->inode);
    free(dir);
}
