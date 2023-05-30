#include "ls.h"
#include "mkfs.h"
#include <stdio.h>

void ls(void)
{
    struct directory *dir = directory_open(0);

    if (dir == NULL) return;

    struct directory_entry ent;

    while (directory_get(dir, &ent) != -1) printf("%d %s\n", ent.inode_num, ent.name);

    directory_close(dir);
}
