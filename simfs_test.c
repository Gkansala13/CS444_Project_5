#include "ctest.h"
#include "image.h"
#include "block.h"
#include "free.h"
#include "inode.h"
#include "mkfs.h"
#include "pack.h"
#include "ls.h"
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

void test_image() {
    // Set up the test environment
    setup();

    // Test opening and closing a valid image file
    CTEST_ASSERT(image_open("test_image", 0) != -1, "expected to open image file");
    CTEST_ASSERT(image_close() != -1, "expected to close image file");

    // Test not being able to open and close an invalid image file
    CTEST_ASSERT(image_open("/test_image", 0) == -1, "expected to not open invalid image file");
    CTEST_ASSERT(image_close() == -1, "expected to not close invalid image file");

    // Tear down the test environment
    teardown();
}

void test_read_write() {
    // Set up the test environment
    setup();

    // Define a block of data to write and a block of data to read
    unsigned char data_to_write[BLOCK_SIZE] = { "Hello world!" };
    unsigned char data_to_read[BLOCK_SIZE] = { 0 };

    // Write the data to block number 9
    bwrite(9, data_to_write);

    // Read the data from block number 9
    bread(9, data_to_read);

    // Check if the data read is the same as the data written
    CTEST_ASSERT(memcmp(data_to_read, data_to_write, BLOCK_SIZE) == 0, "Expected the read and write data to match for a positive block number");

    // Clean up after the test
    teardown();
}

void test_find_and_set_free() {
    // Set up the test environment
    setup();

    unsigned char inode_block[BLOCK_SIZE] = { 0 };
    inode_block[0] = 7;

    // Test finding a free bit
    CTEST_ASSERT(find_free(inode_block) == 3, "Expected to find a free bit");

    set_free(inode_block, 3, 1);
    // Test setting previously free bit
    CTEST_ASSERT(find_free(inode_block) != 3, "Expected to set previously free bit");

    // Reset the inode block
    memset(inode_block, 0, BLOCK_SIZE);
    inode_block[0] = 15;
    set_free(inode_block, 3, 0);
    // Test freeing previously withheld bit
    CTEST_ASSERT(find_free(inode_block) == 3, "Expected to free previously withheld bit");

    // Set the entire inode block to be allocated
    memset(inode_block, 255, BLOCK_SIZE);
    // Test finding a free bit when block is full
    CTEST_ASSERT(find_free(inode_block) == -1, "Expected to find a free bit when block is full");

    // Clean up after the test
    teardown();
}

void test_ialloc() {
    // Set up the test environment
    setup();

    // Create and open the test image
    image_open("test_image", 1);

    // Clear the incore structures
    clear_incore();

    unsigned char inode_block[BLOCK_SIZE] = { 0 };
    bread(1, inode_block);

    // Find the first free bit in the inode block
    int free_bit_num = find_free(inode_block);

    // Allocate an inode
    struct inode *allocated_inode = ialloc();
    int allocated_bit_num = allocated_inode->inode_num;

    // Test if ialloc finds the first free bit
    CTEST_ASSERT(allocated_bit_num == free_bit_num, "Expected ialloc to find first free bit");

    // Test if ialloc sets ref count to 1
    CTEST_ASSERT(allocated_inode->ref_count == 1, "Expected ialloc to sets ref count to 1");

    // Test if ialloc initializes size to 0
    CTEST_ASSERT(allocated_inode->size == 0, "Expected ialloc to initialize size to 0");

    // Test if ialloc initializes owner id to 0
    CTEST_ASSERT(allocated_inode->owner_id == 0, "Expected ialloc to initialize owner id to 0");

    // Test if ialloc initializes permissions to 0
    CTEST_ASSERT(allocated_inode->permissions == 0, "Expected ialloc to initialize permissions to 0");

    // Test if ialloc initializes flags to 0
    CTEST_ASSERT(allocated_inode->flags == 0, "Expected ialloc to initialize flags to 0");

    int non_zero = 0;
    for (int i = 0; i < INODE_PTR_COUNT; i++) {
        if (allocated_inode->block_ptr[i] != 0) {
            non_zero++;
        }
    }
    // Test if ialloc initializes all block pointers to 0
    CTEST_ASSERT(non_zero == 0, "Expected ialloc to initialize all block pointers to 0");

    // Read the inode block again
    bread(1, inode_block);

    // Test if ialloc allocated the previously free bit and wrote to disk
    CTEST_ASSERT(find_free(inode_block) != allocated_bit_num, "Expected ialloc to allocate the previously free bit and write to disk");

    // Set all bits in the inode block to 1 to simulate a full bitmap
    memset(inode_block, 255, BLOCK_SIZE);

    // Write the modified inode block
    bwrite(1, inode_block);

    // Test if ialloc returns null when bitmap is full
    allocated_inode = ialloc();
    CTEST_ASSERT(allocated_inode == NULL, "Expected ialloc to return null when bitmap is full");

    // Close the test image
    image_close();

    // Clean up after the test
    teardown();
}

void test_alloc() {
    // Set up the test environment
    setup();

    // Create and open the test image
    image_open("test_image", 1);

    int free_bit_num;
    int allocated_bit_num;
    unsigned char data_block[BLOCK_SIZE] = { 0 };
    bread(2, data_block);

    // Test alloc finds first free bit
    free_bit_num = find_free(data_block);
    allocated_bit_num = alloc();
    CTEST_ASSERT(allocated_bit_num == free_bit_num, "Expected alloc to find the first free bit");

    bread(2, data_block);
    // Test alloc allocated the previously free bit
    CTEST_ASSERT(find_free(data_block) != allocated_bit_num, "Expected alloc to allocate the previously free bit");

    // Set the entire data block to be allocated
    memset(data_block, 255, BLOCK_SIZE);
    bwrite(2, data_block);

    allocated_bit_num = alloc();
    // Test alloc returns -1 when block is full
    CTEST_ASSERT(allocated_bit_num == -1, "Expected alloc to return -1 when block is full");

    // Close the test image
    image_close();

    // Clean up after the test
    teardown();
}



void test_mkfs()
{
    image_open("test_image", 1);
    clear_incore();

    unsigned char data_block[BLOCK_SIZE] = { 0 };
    unsigned char initial_block[BLOCK_SIZE] = { 255 };

    mkfs();
    struct inode *root_inode;

    // Verify block bitmap initialization
    bread(2, data_block);
    CTEST_ASSERT(memcmp(data_block, initial_block, BLOCK_SIZE) == 0, "Expected block bitmap to be initialized correctly");

    // Verify inode bitmap initialization
    bread(1, data_block);
    memset(initial_block, 1, 1);
    CTEST_ASSERT(memcmp(data_block, initial_block, BLOCK_SIZE) == 0, "Expected inode bitmap to be initialized correctly");

    // Verify root directory entries
    bread(7, data_block);
    int inode_num = read_u16(data_block);
    CTEST_ASSERT(inode_num == 0, "Expected that the root directory's 1st entry was written to the start of the disk and allocated to the 0th inode");

    const char* current = ".";
    CTEST_ASSERT(strcmp((char*)(data_block + FILENAME_OFFSET), current) == 0, "Expected the root directory's 1st entry to have correct filename");

    inode_num = read_u16(data_block + DIR_ENTRY_SIZE);
    const char* parent = "..";
    CTEST_ASSERT(inode_num == 0, "Expected that root the directory's 2nd entry was written to the second entry location in the disk and allocated 0th inode");
    CTEST_ASSERT(strcmp((char*)(data_block + DIR_ENTRY_SIZE + FILENAME_OFFSET), parent) == 0, "Expected that the root directory's 2nd entry to have correct filename");

    // Verify inode allocation and properties
    struct inode *free_node = find_incore_free();
    CTEST_ASSERT(free_node->inode_num == 0, "Expected that the mkfs correctly freed root directory's inode, 0, from incore");

    root_inode = iget(inode_num);
    CTEST_ASSERT(root_inode->flags == 2, "Expected that root directory is allocated and file flag is directory type");
    CTEST_ASSERT(root_inode->size == DIR_START_SIZE, "Expected the root directory to be initialized with the correct size");
    CTEST_ASSERT(root_inode->block_ptr[0] == 7, "Expected the root directory's inode to point to the 8th data block");

    image_close();
    remove("test_image");
}

void test_find_free_incore()
{
    image_open("test_image", 1);
    clear_incore();

    struct inode *free_node = find_incore_free();
    CTEST_ASSERT(free_node->ref_count == 0, "Expected that the empty incore has first element free");

    iget(0);
    struct inode *allocated_node = find_incore_free();
    CTEST_ASSERT(allocated_node != NULL, "Expected that the allocated inode is found");

    clear_incore();
    for (int i = 0; i < MAX_SYS_OPEN_FILES; i++)
    {
        iget(i);
    }
    struct inode *full_node = find_incore_free();
    CTEST_ASSERT(full_node == NULL, "Expected that NULL is returned when incore array is full");

    image_close();
    remove("test_image");
}


void test_find_incore()
{
    image_open("test_image", 1);
    clear_incore();

    // Finding an existing incore inode
    iget(3);
    struct inode *target_inode = find_incore(3);
    CTEST_ASSERT(target_inode != NULL, "Expected the found incore inode to not be NULL");
    CTEST_ASSERT(target_inode->inode_num == 3, "Expected found incore inode's number to match the number passed in");

    // Finding a non-existing incore inode
    clear_incore();
    iget(2);
    iget(4);
    target_inode = find_incore(3);
    CTEST_ASSERT(target_inode == NULL, "Expected that NULL is returned when inode cannot be found");

    // Finding incore inode when incore array is full
    clear_incore();
    for (int i = 0; i < MAX_SYS_OPEN_FILES - 1; i++) // Leave one slot empty
    {
        iget(i);
    }
    target_inode = find_incore(MAX_SYS_OPEN_FILES);
    CTEST_ASSERT(target_inode == NULL, "Expected that NULL is returned when incore array is full");

    image_close();
    remove("test_image");
}


void test_read_inode()
{
    image_open("test_image", 1);
    clear_incore();

    // Set up the inode block with test data
    unsigned char inode_block[BLOCK_SIZE] = { 0 };
    const int inode_offset = 64;
    const unsigned int test_size = 15;  // Updated to unsigned int
    const int test_owner_id = 4;
    const int test_permissions = 3;
    const int test_flags = 2;
    const int test_link_count = 1;
    const int test_block_ptr = 4;

    write_u32(inode_block + inode_offset + SIZE_OFFSET, test_size);
    write_u16(inode_block + inode_offset + ID_OFFSET, test_owner_id);
    write_u8(inode_block + inode_offset + PERMISSIONS_OFFSET, test_permissions);
    write_u8(inode_block + inode_offset + FLAGS_OFFSET, test_flags);
    write_u8(inode_block + inode_offset + LINK_COUNT_OFFSET, test_link_count);
    write_u16(inode_block + inode_offset + BLOCK_PTR_OFFSET + (2 * 1), test_block_ptr);
    bwrite(3, inode_block);

    // Read the inode and perform assertions
    struct inode test_node;
    read_inode(&test_node, 1);
    CTEST_ASSERT(test_node.size == (unsigned int)test_size, "Expected the size attribute to be successfully read from memory");
    CTEST_ASSERT(test_node.owner_id == test_owner_id, "Expected the owner's id attribute to be successfully read from memory");
    CTEST_ASSERT(test_node.permissions == test_permissions, "Expected the permission attribute to be successfully read from memory");
    CTEST_ASSERT(test_node.flags == test_flags, "Expected the flag attribute to be successfully read from memory");
    CTEST_ASSERT(test_node.link_count == test_link_count, "Expected the link count attribute to be successfully read from memory");
    CTEST_ASSERT(test_node.block_ptr[1] == test_block_ptr, "Expected the block pointer attribute to be successfully read from memory");

    image_close();
    remove("test_image");
}


void test_write_inode()
{
    image_open("test_image", 1);
    clear_incore();

    // Create a sample inode
    struct inode write_node;
    write_node.inode_num = 2;
    write_node.size = 20;
    write_node.owner_id = 8;
    write_node.permissions = 5;
    write_node.flags = 3;
    write_node.link_count = 2;
    write_node.block_ptr[1] = 6;

    // Write the inode
    write_inode(&write_node);

    // Read the inode
    struct inode read_node;
    read_inode(&read_node, 2);

    // Perform assertions to validate the correctness of the inode attributes
    CTEST_ASSERT(read_node.size == write_node.size, "Expected the correct size attribute");
    CTEST_ASSERT(read_node.owner_id == write_node.owner_id, "Expected the correct owner ID attribute");
    CTEST_ASSERT(read_node.permissions == write_node.permissions, "Expected the correct permissions attribute");
    CTEST_ASSERT(read_node.flags == write_node.flags, "Expected the correct flags attribute");
    CTEST_ASSERT(read_node.link_count == write_node.link_count, "Expected the correct link count attribute");
    CTEST_ASSERT(read_node.block_ptr[1] == write_node.block_ptr[1], "Expected the correct block pointer attribute");

    // Clean up
    image_close();
    remove("test_image");
}


void test_iget()
{
    image_open("test_image", 1);
    clear_incore();

    // Getting a new inode
    struct inode* new_node = iget(3);
    CTEST_ASSERT(new_node != NULL, "Expected the new inode to be returned");
    CTEST_ASSERT(new_node->inode_num == 3, "Expected the inode_num of the returned inode to be correct");
    CTEST_ASSERT(new_node->ref_count == 1, "Expected the ref_count of the returned inode to be 1");

    // Getting an existing inode
    struct inode* existing_node = iget(3);
    CTEST_ASSERT(existing_node == new_node, "Expected the existng inode returns the same inode pointer");
    CTEST_ASSERT(existing_node->ref_count == 2, "Expected the ref_count of the existing inode is incremented");

    // Incore array full with existing inode
    clear_incore();
    for (int i = 0; i < MAX_SYS_OPEN_FILES; i++)
    {
        iget(i);
    }
    existing_node = iget(3);
    CTEST_ASSERT(existing_node != NULL, "Expected the incore array is full to return a valid inode");
    CTEST_ASSERT(existing_node->inode_num == 3, "Expected the inode_num of the existing inode to be correct");

    // Incore array full without existing inode
    struct inode* exceeding_node = iget(MAX_SYS_OPEN_FILES);
    CTEST_ASSERT(exceeding_node == NULL, "Expected the incore array to be full and the inode doesn't exist returns NULL");

    // Clearing incore nodes ref counts to zero
    clear_incore();
    new_node = iget(MAX_SYS_OPEN_FILES);
    CTEST_ASSERT(new_node != NULL, "Expected that clearing incore nodes ref counts to zero allows for getting a new inode");
    CTEST_ASSERT(new_node->inode_num == MAX_SYS_OPEN_FILES, "Expected that the inode_num of the new inode is correct");

    // Clean up
    image_close();
    remove("test_image");
}


void test_iput()
{
    image_open("test_image", 1);
    clear_incore();

    // Incore array full without existing inode
    for (int i = 0; i < MAX_SYS_OPEN_FILES; i++)
    {
        iget(i);
    }
    struct inode* exceeding_node = iget(MAX_SYS_OPEN_FILES);
    CTEST_ASSERT(exceeding_node == NULL, "Expected that getting an inode when the incore array is full and the inode doesn't exist returns NULL");

    // iput with existing inode
    struct inode* existing_node = find_incore(43);
    CTEST_ASSERT(existing_node != NULL, "Expected that finding an existing inode in the incore array is successful");

    iput(existing_node);
    CTEST_ASSERT(existing_node->ref_count == 0, "Expected that the ref_count of the inode passed to iput is decremented");

    iput(existing_node);
    struct inode* retrieved_node = iget(43);
    CTEST_ASSERT(retrieved_node != NULL && retrieved_node->ref_count == 1, "Expected that the previously released inode is retrieved after calling iput on an incore inode with ref_count 0");

    // Clean up
    image_close();
    remove("test_image");
}


void test_directory_open()
{
    image_open("test_image", 1);
    clear_incore();

    // Open directory with a new inode
    struct inode* new_node = iget(2);
    struct directory* dir = directory_open(2);

    CTEST_ASSERT(dir != NULL, "Expected that directory_open returns a valid directory pointer");
    CTEST_ASSERT(dir->inode == new_node, "Expected that the returned directory has an inode pointer corresponding to the inode number passed in");
    CTEST_ASSERT(dir->offset == 0, "Expected that the returned directory has an offset of 0 initially");

    // Incore array full without existing directory
    clear_incore();
    for (int i = 0; i < MAX_SYS_OPEN_FILES; i++)
    {
        iget(i);
    }
    dir = directory_open(MAX_SYS_OPEN_FILES);
    CTEST_ASSERT(dir == NULL, "Expected that NULL is returned when trying to open a directory that is not incore when the incore array is full");

    // Clean up
    image_close();
    remove("test_image");
}


void test_directory_get()
{
    image_open("test_image", 1);
    clear_incore();
    mkfs();

    // Get first directory entry
    struct directory* dir = directory_open(0);
    struct directory_entry ent;
    int status = directory_get(dir, &ent);

    CTEST_ASSERT(status == 1, "Expected that a successful call to directory_get returns 1");
    CTEST_ASSERT(dir->offset == DIR_ENTRY_SIZE, "Expected that the directory's offset is incremented by the size of an entry after one call");
    CTEST_ASSERT(ent.inode_num == 0 && strcmp(ent.name, ".") == 0, "Expected that the first call returns an entry with inode_num and name matching the directory's first entry");

    // Get second directory entry
    directory_get(dir, &ent);

    CTEST_ASSERT(dir->offset == 2 * DIR_ENTRY_SIZE, "Expected that the directory's offset is incremented by the size of an entry again after another call");
    CTEST_ASSERT(ent.inode_num == 0 && strcmp(ent.name, "..") == 0, "Expected that the second call returns an entry with inode_num and name matching the directory's second entry");

    // Get beyond the last directory entry
    status = directory_get(dir, &ent);
    CTEST_ASSERT(dir->offset == dir->inode->size && status == -1, "Expected that -1 is returned when attempting to get an entry beyond the last entry");

    // Clean up
    image_close();
    remove("test_image");
}


void test_directory_close()
{
    image_open("test_image", 1);
    clear_incore();

    // Close directory
    struct directory* dir = directory_open(0);
    struct inode* existing_node = find_incore(0);
    directory_close(dir);
    struct inode* new_node = find_incore(0);

    CTEST_ASSERT(existing_node != NULL && new_node == NULL, "Expected that the directory's inode was placed incore and removed after the call to directory_close");

    // Clean up
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
    test_find_incore();
    test_find_free_incore();
    test_write_inode();
    test_read_inode();
    test_iput();
    test_iget();
    test_directory_get();
    test_directory_open();
    test_directory_close();
    CTEST_RESULTS();
}
