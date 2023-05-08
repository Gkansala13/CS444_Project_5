#include "free.h"
#include "block.h"

void set_free(unsigned char *block, int num, int set)
{
    int byte_num = num / 8;
    int bit_num = num % 8;
    
    block[byte_num] = set ? (block[byte_num] | (1 << bit_num)) : (block[byte_num] & ~(1 << bit_num));
}


int find_free(unsigned char *block)
{
    for(int i=0; i < BLOCK_SIZE; i++)
    {
        if(block[i] != 0xff)
        {
            int bit_num = __builtin_ctz(~block[i]);
            return (i * 8) + bit_num;
        }
    }
    return -1;
}

