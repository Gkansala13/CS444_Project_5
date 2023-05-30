#include <fcntl.h>
#include <unistd.h>
#include "image.h"

int image_fd;

int image_open(char *filename, int truncate)
{
    int flags = O_RDWR | O_CREAT | (truncate? O_TRUNC:0);
    
    int image_fd = open(filename, flags, 0600);
    return image_fd;
}


int image_close(void)
{
    return close(image_fd); 
}
