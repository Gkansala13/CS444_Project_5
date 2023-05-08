#include "ctest.h"
#include "image.h"
#include "block.h"
#include "free.h"
#include "inode.h"
#include "mkfs.h"
#include <string.h>

void setup() {
    // Open the test image with write access
    image_open("test_image", 1);
}

void teardown() {
    // Close the test image and remove it from the file system
    image_close();
    remove("test_image");
}

void test_mkfs() {
    // Initialize data block and set initial block to 127
    unsigned char data_block[BLOCK_SIZE] = { 0 };
    unsigned char initial_block[BLOCK_SIZE] = { 127 };

    // Open test image with write access
    image_open("test_image", 1);

    // Create a file system
    mkfs();

    // Read data block from block number 2
    bread(2, data_block);

    // Verify that the data block was initialized correctly
    CTEST_ASSERT(memcmp(data_block, initial_block, BLOCK_SIZE) == 0, "expected initialize memory block correctly");

    // Close the image and delete the test file
    image_close();
    remove("test_image");
}


void test_find_and_set_free() {
    // Initialize an inode bitmap with the first bit set to 1
    unsigned char inode_bitmap[BLOCK_SIZE] = { 0 };
    inode_bitmap[0] = 7; 

    // Test finding a free bit
    CTEST_ASSERT(find_free(inode_bitmap) == 3, "expected find a free bit");

    // Test setting a previously free bit
    set_free(inode_bitmap, 3, 1);
    CTEST_ASSERT(find_free(inode_bitmap) != 3, "expected set previously free bit");

    // Test freeing a previously withheld bit
    inode_bitmap[0] = 15; 
    set_free(inode_bitmap, 3, 0);
    CTEST_ASSERT(find_free(inode_bitmap) == 3, "expected free previously withheld bit");

    // Test not finding a free bit when the block is full
    for (int i = 0; i < BLOCK_SIZE; i++) {
        inode_bitmap[i] = 255; 
    }
    CTEST_ASSERT(find_free(inode_bitmap) == -1, "expected not find a free bit when block is full");
}


void test_image() {
    // Set up the test environment
    setup();

    // Test opening and closing a valid image file
    CTEST_ASSERT(image_open("test_image", 0) != -1, "expected open image file");
    CTEST_ASSERT(image_close() != -1, "expected close image file");

    // Test not being able to open and close an invalid image file
    CTEST_ASSERT(image_open("/test_image", 0) == -1, "expected not open invalid image file");
    CTEST_ASSERT(image_close() == -1, "expected not close invalid image file");

    // Tear down the test environment
    teardown();
}


void test_read_write() {
    setup();

    // Define a block of data to write and a block of data to read
    unsigned char data_to_write[BLOCK_SIZE] = { "Hello, world!" };
    unsigned char data_to_read[BLOCK_SIZE] = { 0 };

    // Write the data to block number 9
    bwrite(9, data_to_write);

    // Read the data from block number 9
    bread(9, data_to_read);

    // Check if the data read is the same as the data written
    CTEST_ASSERT(memcmp(data_to_read, data_to_write, BLOCK_SIZE) == 0, "Expected read and write data to match for a positive block number");

    // Clean up after the test
    teardown();
}


void test_ialloc() {
    // Define a block of inodes and variables to store free and allocated bit numbers
    unsigned char inode_block[BLOCK_SIZE] = { 0 };
    int free_bit_num, allocated_bit_num;

    // Open the test image and read the inode block
    image_open("test_image", 1);
    bread(1, inode_block);

    // Find the first free bit and allocate it
    free_bit_num = find_free(inode_block);
    allocated_bit_num = ialloc();

    // Check if the allocated bit number matches the free bit number
    CTEST_ASSERT(allocated_bit_num == free_bit_num, "Expected to find and allocate the first free bit");

    // Read the inode block again and check if the allocated bit is no longer free
    bread(1, inode_block);
    CTEST_ASSERT(find_free(inode_block) != allocated_bit_num, "Expected to allocate a previously free bit");

    // Fill the inode block with data to mark it as full and try to allocate another bit
    for (int i = 0; i < BLOCK_SIZE; i++) {
        inode_block[i] = 255;
    }
    bwrite(1, inode_block);
    allocated_bit_num = ialloc();

    // Check if the function correctly returns -1 when the block is full
    CTEST_ASSERT(allocated_bit_num == -1, "Expected to return -1 when block is full");

    // Close the test image and remove it
    image_close();
    remove("test_image");
}


void test_alloc() {
    // Define a data block and variables to store free and allocated bit numbers
    unsigned char data_block[BLOCK_SIZE] = { 0 };
    int free_bit_num, allocated_bit_num;

    // Open the test image and read the data block
    image_open("test_image", 1);
    bread(2, data_block);

    // Find the first free bit and allocate it
    free_bit_num = find_free(data_block);
    allocated_bit_num = alloc();

    // Check if the allocated bit number matches the free bit number
    CTEST_ASSERT(allocated_bit_num == free_bit_num, "Expected to find and allocate the first free bit");

    // Read the data block again and check if the allocated bit is no longer free
    bread(2, data_block);
    CTEST_ASSERT(find_free(data_block) != allocated_bit_num, "Expected to allocate a previously free bit");

    // Fill the data block with data to mark it as full and try to allocate another bit
    for (int i = 0; i < BLOCK_SIZE; i++) {
        data_block[i] = 255;
    }
    bwrite(2, data_block);
    allocated_bit_num = alloc();

    // Check if the function correctly returns -1 when the block is full
    CTEST_ASSERT(allocated_bit_num == -1, "Expected to return -1 when block is full");

    // Close the test image and remove it
    image_close();
    remove("test_image");
}


int main() 
{
    CTEST_VERBOSE(1);
    test_image();
    test_mkfs();
    test_read_write();
    test_find_and_set_free();
    test_ialloc();
    test_alloc();
}
