#ifndef INODE_H
#define INODE_H

#include "block.h"

#define FREE_INODE_BLOCK_NUM 1

struct inode *ialloc(void);
struct inode *iget(int inode_num);
void iput(struct inode *in);
void write_inode(struct inode *in);
void read_inode(struct inode *in, int inode_num);
struct inode *find_incore_free(void);
struct inode *find_incore(unsigned int inode_num);
void clear_incore(void);

#define INODE_PTR_COUNT 16
#define MAX_SYS_OPEN_FILES 64

#define INODE_SIZE 64
#define INODE_FIRST_BLOCK 3

#define SIZE_OFFSET 0
#define ID_OFFSET (SIZE_OFFSET + 4)
#define PERMISSIONS_OFFSET (ID_OFFSET + 2)
#define FLAGS_OFFSET (PERMISSIONS_OFFSET + 1)
#define LINK_COUNT_OFFSET (FLAGS_OFFSET + 1)
#define BLOCK_PTR_OFFSET (LINK_COUNT_OFFSET + 1)
#define BYTES_PER_BLOCK_PTR 2

struct inode {
    unsigned int size;
    unsigned short owner_id;
    unsigned char permissions;
    unsigned char flags;
    unsigned char link_count;
    unsigned short block_ptr[INODE_PTR_COUNT];

    unsigned int ref_count;  // in-core only
    unsigned int inode_num;
};

#endif
