#ifndef MKFS_H
#define MKFS_H

#define FILENAME_OFFSET 2
#define DIR_FLAG 2
#define DIR_ENTRY_SIZE 32
#define UNKNOWN_FLAG 0
#define NUMBER_OF_BLOCKS 1024
#define DIR_START_SIZE (DIR_ENTRY_SIZE * 2)
#define FILE_FLAG 1

struct directory {
    struct inode *inode;
    unsigned int offset;
};

struct directory_entry {
    unsigned int inode_num;
    char name[16];
};

void mkfs(void);
struct directory *directory_open(int inode_num);
int directory_get(struct directory *dir, struct directory_entry *ent);
void directory_close(struct directory *dir);

#endif
