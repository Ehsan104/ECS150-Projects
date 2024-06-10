#include <iostream>
#include <cstring>
#include <string>
#include <vector>
#include <deque>
#include <assert.h>

#include "LocalFileSystem.h"
#include "ufs.h"

using namespace std;

//Helper functions that needed to be implemented: 
bool LocalFileSystem::diskHasSpace(super_t *super, int numInodesNeeded, int numDataBytesNeeded, int numDataBlocksNeeded) {

    /**
     * numDataBytesNeeded is converted to blocks and added to numDataBlocksNeeded
     * Having two separate arguments for data helps for operations that write
     * new data to two separate entities. If you don't need a value
     * you can set the number needed to 0.
     */
    
    // Convert bytes needed to data blocks needed
    numDataBlocksNeeded += (numDataBytesNeeded + UFS_BLOCK_SIZE - 1) / UFS_BLOCK_SIZE;

    // Check for space in the inode bitmap
    if (numInodesNeeded > 0) {
        
        // Load the inode bitmap
        unsigned char inodeBitmap[super->inode_bitmap_len * UFS_BLOCK_SIZE]; readInodeBitmap(super, inodeBitmap);
        
        // Initialize variable to track number of available inodes
        int availableInodes = 0;
        
        // Count how many inodes are available
        for (int i = 0; i < (super->num_inodes); i++) { if (!(inodeBitmap[(i / 8)] & (1 << (i % 8)))) {
            availableInodes++;
        }}
        
        // Return if the number of inodes will suffice
        return availableInodes >= numInodesNeeded;
    }
    
    // Check for space in the data bitmap
    if (numDataBlocksNeeded > 0) {
        
        // Load the inode bitmap
        unsigned char dataBitmap[super->data_bitmap_len * UFS_BLOCK_SIZE]; readDataBitmap(super, dataBitmap);
        
        // Initialize variable to track number of available inodes
        int availableDataBlocks = 0;
        
        // Count how many inodes are available
        for (int i = 0; i < (super->num_data); i++) { if (!(dataBitmap[(i / 8)] & (1 << (i % 8)))) {
            availableDataBlocks++;
        }}
        
        // Return if the number of inodes will suffice
        return availableDataBlocks >= numInodesNeeded;
    }
    
    return true; /* Default Case */

}

void LocalFileSystem::readInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->inode_bitmap_len; i++) {
        disk->readBlock((super->inode_bitmap_addr + i), buffer);
        memcpy((inodeBitmap + (i * UFS_BLOCK_SIZE)), buffer, UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeInodeBitmap(super_t *super, unsigned char *inodeBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->inode_bitmap_len; i++) {
        memcpy(buffer, (inodeBitmap + (i * UFS_BLOCK_SIZE)), UFS_BLOCK_SIZE);
        disk->writeBlock((super->inode_bitmap_addr + i), buffer);
    }
}

void LocalFileSystem::readDataBitmap(super_t *super, unsigned char *dataBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->data_bitmap_len; i++) {
        disk->readBlock((super->data_bitmap_addr + i), buffer);
        memcpy((dataBitmap + (i * UFS_BLOCK_SIZE)), buffer, UFS_BLOCK_SIZE);
    }
}

void LocalFileSystem::writeDataBitmap(super_t *super, unsigned char *dataBitmap) {
    char buffer[UFS_BLOCK_SIZE];
    for (int i = 0; i < super->data_bitmap_len; i++) {
        memcpy(buffer, (dataBitmap + (i * UFS_BLOCK_SIZE)), UFS_BLOCK_SIZE);
        disk->writeBlock((super->data_bitmap_addr + i), buffer);
    }
}

void LocalFileSystem::readInodeRegion(super_t *super, inode_t *inodes) {
    
    // Calculate the total size of the inode region in bytes
    int inodeRegionSize = super->num_inodes * sizeof(inode_t);
    char* buffer = new char[inodeRegionSize];
    
    // Read the inode region block by block
    int readSize = 0;
    for (int i = 0; i < super->inode_region_len; i++) {
        int blockSize = std::min(UFS_BLOCK_SIZE, inodeRegionSize - readSize);
        disk->readBlock(super->inode_region_addr + i, buffer + readSize);
        readSize += blockSize;
    }
    
    // Copy the data into the provided inode array
    memcpy(inodes, buffer, inodeRegionSize);
    delete[] buffer;
}

void LocalFileSystem::writeInodeRegion(super_t *super, inode_t *inodes) {
    
    // Calculate the total size of the inode region in bytes
    int inodeRegionSize = super->num_inodes * sizeof(inode_t);
    char* buffer = new char[inodeRegionSize];

    // Copy the inode data into a buffer
    memcpy(buffer, inodes, inodeRegionSize);
    
    // Write the inode region block by block
    int writtenSize = 0;
    for (int i = 0; i < super->inode_region_len; i++) {
        int blockSize = std::min(UFS_BLOCK_SIZE, inodeRegionSize - writtenSize);
        disk->writeBlock(super->inode_region_addr + i, buffer + writtenSize);
        writtenSize += blockSize;
    }   delete[] buffer;
    
}

//All the localFileSystem Stuff:

LocalFileSystem::LocalFileSystem(Disk *disk) { this->disk = disk; }

void LocalFileSystem::readSuperBlock(super_t *super) {
    char buffer[UFS_BLOCK_SIZE];
    disk->readBlock(0, buffer);
    memcpy(super, buffer, sizeof(super_t));
}

int LocalFileSystem::lookup(int parentInodeNumber, string name) {
  /**
   * Lookup an inode.
   *
   * Takes the parent inode number (which should be the inode number
   * of a directory) and looks up the entry name in it. The inode
   * number of name is returned.
   *
   * Success: return inode number of name
   * Failure: return -ENOTFOUND, -EINVALIDINODE.
   * Failure modes: invalid parentInodeNumber, name does not exist.
   */
    
    // Get the parent inode and check if it is valid
    inode_t parent; 
    int EVALUE;

    if ((EVALUE = stat(parentInodeNumber, &parent)) < 0) {  //INVALID CASES
        return EVALUE; 
    }
    
    if (parent.type != UFS_DIRECTORY) { //INVALID CASES
        return -EINVALIDINODE; 
    }

    if (name.length() <= 0|| name.length() >= DIR_ENT_NAME_SIZE) { //INVALID CASES
        return -EINVALIDNAME; 
    }
    
    // Calculate the number of blocks and entries, define variables to store blocks and entries
    dir_ent_t entry; 
    char buffer[UFS_BLOCK_SIZE]; 
    int numEntries = UFS_BLOCK_SIZE / sizeof(dir_ent_t);
    int numBlocks = (parent.size / UFS_BLOCK_SIZE) + (parent.size % UFS_BLOCK_SIZE ? 1 : 0);
    
    // Check each allocated block in direct
    for (int i = 0; i < numBlocks; i++) {
        disk->readBlock(parent.direct[i], buffer);
        
        // For every entry in the block
        for (int j = 0; j < numEntries; j++) {
            memcpy(&entry, (buffer + (j * sizeof(dir_ent_t))), sizeof(dir_ent_t));
            if (entry.inum >= 0 && entry.name == name) {  //If the entry is valid and the name matches, return the inode numebr
                return entry.inum; 
            }
        }
    }
    
    return -ENOTFOUND; 
    
}


int LocalFileSystem::stat(int inodeNumber, inode_t *inode) {
  /**
   * Read an inode.
   *
   * Given an inodeNumber this function will fill in the `inode` struct with
   * the type of the entry and the size of the data, in bytes, and direct blocks.
   *
   * Success: return 0
   * Failure: return -EINVALIDINODE
   * Failure modes: invalid inodeNumber
   */

    // Copy the superblock into our local variable
    super_t super; 
    readSuperBlock(&super);
    
    if (inodeNumber < 0 || inodeNumber >= super.num_inodes) {  //INVALID CASES
        return -EINVALIDINODE; 
    }

    // Get the block number and offset for the inode using the inode number
    int blockNumber = super.inode_region_addr + ((inodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE);
    int offset = (inodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;
    
    // Copy the block from disk into abuffer
    char buffer[UFS_BLOCK_SIZE];
    disk->readBlock(blockNumber, buffer);
    memcpy(inode, buffer + offset, sizeof(inode_t));  // Copy the inode from the buffer into our local inode variable

    return 0;   
}

int LocalFileSystem::read(int inodeNumber, void *buffer, int size) {

    /**
     * Read the contents of a file or directory.
     *
     * Reads up to `size` bytes of data into the buffer from file specified by
     * inodeNumber. The routine should work for either a file or directory;
     * directories should return data in the format specified by dir_ent_t.
     *
     * Success: number of bytes read
     * Failure: -EINVALIDINODE, -EINVALIDSIZE.
     * Failure modes: invalid inodeNumber, invalid size.
     */
    
    // Get the inode using the inodeNumber and return if it is valid
    inode_t inode; 
    int EVALUE;

    if ((EVALUE = stat(inodeNumber, &inode)) < 0) {  //INVALID CASES
        return EVALUE; 
    }

    if (size < 0 || size > inode.size) {  //INVALID CASES
        return -EINVALIDSIZE; 
    }

    char buf[UFS_BLOCK_SIZE]; 
    int bytesRead = 0;

    // Check each entry in the direct table
    for (int i = 0; (i < DIRECT_PTRS) && (size > 0); ++i) {
        disk->readBlock(inode.direct[i], buf);
        int toRead = min(size, UFS_BLOCK_SIZE); // Calculate the number of bytes to read
        memcpy(((char *)buffer + (i * UFS_BLOCK_SIZE)), buf, toRead); // Copy the buffer into the local buffer for the specified bytes
        size -= toRead; bytesRead += toRead; // Decrement from the number of bytesRead and increment to the total bytes read
    }

    return bytesRead; 
}


int LocalFileSystem::create(int parentInodeNumber, int type, string name) {
    /**
     * Makes a file or directory.
     *
     * Makes a file (type == UFS_REGULAR_FILE) or directory (type == UFS_DIRECTORY)
     * in the parent directory specified by parentInodeNumber of name name.
     *
     * Success: return the inode number of the new file or directory
     * Failure: -EINVALIDINODE, -EINVALIDNAME, -EINVALIDTYPE, -ENOTENOUGHSPACE.
     * Failure modes: parentInodeNumber does not exist, or name is too long.
     * If name already exists and is of the correct type, return success, but
     * if the name already exists and is of the wrong type, return an error.
     */
    
    super_t super;
    readSuperBlock(&super);

    if (name.size() > DIR_ENT_NAME_SIZE || name.size() == 0) { //INVALID CASES
        return -EINVALIDNAME;
    }

    inode_t parent;

    if (stat(parentInodeNumber, &parent) == -EINVALIDINODE || parent.type != UFS_DIRECTORY) { //INVALID CASES
        return -EINVALIDINODE;
    }

    int n = parent.size / sizeof(dir_ent_t); // Number of entries in the parent directory
    char buffer[parent.size]; // Buffer to store the parent directory

    read(parentInodeNumber, buffer, parent.size); // Read the parent directory

    for (int i = 0; i < n; i++) { // Check if the name already exists
        dir_ent_t entry;
        memcpy(&entry, buffer + i * sizeof(dir_ent_t), sizeof(dir_ent_t));
        if (strcmp(entry.name, name.c_str()) == 0) {
            inode_t inode;
            if (stat(entry.inum, &inode) == -EINVALIDINODE) { //INVALID CASES
                return -EINVALIDINODE; 
            }

        if (inode.type != type) { //INVALID CASES
            return -EINVALIDTYPE;
        }

        return entry.inum;

        }
    }

    unsigned char inodebitmap[UFS_BLOCK_SIZE * super.inode_bitmap_len]; //Buffer to store the inode bitmap
    readInodeBitmap(&super, inodebitmap); //call to the helper function

    // Find a free inode
    int freeInode = -1;

    for (int i = 0; i < super.num_inodes; ++i) { //Check the inode bitmap for a free inode
        if (((inodebitmap[i / 8] >> (i % 8)) & 1) == 0) {
            freeInode = i;
        break;
        }
    }

    if (freeInode == -1) { //INVALID CASES
        return -ENOTENOUGHSPACE;
    }

    // Determine the number of new data blocks needed
    int neededBlocks = (type == UFS_DIRECTORY) ? 1 : 0; //Number of blocks needed for the new file
    int dirBlock = parent.size / UFS_BLOCK_SIZE; //Parent block
    int offset = parent.size % UFS_BLOCK_SIZE;  //Offset within the parent block

    if (offset == 0) { //If the offset is 0, we need a new block
        neededBlocks++;
    }

    if (offset == 0 && parent.size + UFS_BLOCK_SIZE > MAX_FILE_SIZE) { //INVALID CASES
        return -ENOTENOUGHSPACE;
    }

    unsigned char databitmap[UFS_BLOCK_SIZE * super.data_bitmap_len]; //Buffer to store the data bitmap

    readDataBitmap(&super, databitmap); //call to the helper function

    // Find free data blocks
    vector<int> freeDataBlocks; //Vector to store the free data blocks
    
    for (int i = 0; i < super.num_data && static_cast<int>(freeDataBlocks.size()) < neededBlocks; ++i) { //Check the data bitmap for free data blocks
        if ((databitmap[i / 8] & (1 << (i % 8))) == 0) {
            freeDataBlocks.push_back(i);
        }
    }

    if (static_cast<int>(freeDataBlocks.size()) < neededBlocks) { //INVALID CASES
        return -ENOTENOUGHSPACE;
    }

    if (offset == 0) { //If the offset is 0, we need a new block
        parent.direct[dirBlock] = freeDataBlocks[0] + super.data_region_addr;
    }

    inode_t newInode; 
    newInode.type = type; 
    newInode.size = (type == UFS_DIRECTORY) ? 2 * sizeof(dir_ent_t) : 0; //Size of the new file

    for (int i = 0; i < DIRECT_PTRS; ++i) { //Assign the data blocks to the new inode
        newInode.direct[i] = 0;
    }

    if (type == UFS_DIRECTORY) { //If the new file is a directory
        newInode.direct[0] = freeDataBlocks.back() + super.data_region_addr;
        dir_ent_t entry1;
        entry1.inum = freeInode;
        strcpy(entry1.name, ".");
        dir_ent_t entry2;
        entry2.inum = parentInodeNumber;
        strcpy(entry2.name, "..");

        char toWrite[UFS_BLOCK_SIZE];
        memcpy(toWrite, &entry1, sizeof(dir_ent_t)); //Copy the entries to the buffer
        memcpy(toWrite + sizeof(dir_ent_t), &entry2, sizeof(dir_ent_t)); //Copy the entries to the buffer

        disk->writeBlock(newInode.direct[0], toWrite); //Write the entries to the disk
    }

    for (int block : freeDataBlocks) { //Update the data bitmap
        databitmap[block / 8] |= 1 << (block % 8);
    }

    inodebitmap[freeInode / 8] |= 1 << (freeInode % 8); //Update the inode bitmap

    dir_ent_t newEntry;
    newEntry.inum = freeInode;
    strcpy(newEntry.name, name.c_str());

    char toWritePar[UFS_BLOCK_SIZE];

    disk->readBlock(parent.direct[dirBlock], toWritePar); //Read the parent directory

    memcpy(toWritePar + offset, &newEntry, sizeof(dir_ent_t));

    disk->writeBlock(parent.direct[dirBlock], toWritePar); //Write the new entry to the parent directory

    writeInodeBitmap(&super, inodebitmap); //call to helper function
    writeDataBitmap(&super, databitmap); //call to helper function

    parent.size += sizeof(dir_ent_t); 
    inode_t inodes[super.num_inodes];

    readInodeRegion(&super, inodes); //call to helper function 

    memcpy(&inodes[parentInodeNumber], &parent, sizeof(inode_t));
    memcpy(&inodes[freeInode], &newInode, sizeof(inode_t));

    writeInodeRegion(&super, inodes); //call to helper function 

    return freeInode; //Return the inode number of the new file
}

int LocalFileSystem::write(int inodeNumber, const void *writeBuffer, int size) {
    /**
     * Write the contents of a file.
     *
     * Writes a buffer of size to the file, replacing any content that
     * already exists.
     *
     * Success: number of bytes written
     * Failure: -EINVALIDINODE, -EINVALIDSIZE, -EINVALIDTYPE, -ENOTENOUGHSPACE.
     * Failure modes: invalid inodeNumber, invalid size, not a regular file
     * (because you can't write to directories).
     */

    // Read in the Super block
    super_t super; 
    readSuperBlock(&super);

    // Check if the inodeNumber is Valid
    inode_t inode; 
    int EVALUE;
    if ((EVALUE = stat(inodeNumber, &inode)) < 0) {  //INVALID CASES
        return EVALUE;
    }

    // Check if the Size is Valid
    if (size < 0 || size > MAX_FILE_SIZE) { //INVALID CASES
        return -EINVALIDSIZE; 
    }

    // Check if the entry type is valid (is a directory)
    if (inode.type != UFS_REGULAR_FILE) {  //INVALID CASES
        return -EWRITETODIR; 
    }

    // Check if space already exists in the direct table
    int blocksNeeded = (size / UFS_BLOCK_SIZE) + (size % UFS_BLOCK_SIZE ? 1 : 0), blocksAvailable = 0;

    for (int i = 0; i < DIRECT_PTRS; i++) { //Check the direct table for available blocks
        blocksAvailable += (inode.direct[i] != 0); 
    }

    // If number of blocks needed exceeds number of entries in the direct table, return an error
    if (!diskHasSpace(&super, 0, size, blocksNeeded) || blocksNeeded > DIRECT_PTRS) {  //INVALID CASES
        return -ENOTENOUGHSPACE; 
    }

    // If there are more blocks needed than available, then allocate new blocks
    unsigned char buffer[UFS_BLOCK_SIZE]; 
    if ((blocksNeeded - blocksAvailable)) {
        // Load in the data bitmap
        unsigned char dataBitmap[UFS_BLOCK_SIZE * super.data_bitmap_len];

        for (int i = 0; i < super.data_bitmap_len; i++) { //Read the data bitmap from the disk
            disk->readBlock((super.data_bitmap_addr + i), buffer);
            memcpy((dataBitmap + (i * UFS_BLOCK_SIZE)), buffer, UFS_BLOCK_SIZE);
        }

        for (int i = 0; i < (blocksNeeded - blocksAvailable); i++) { //Allocate new blocks
            int dataBlockNumber = -1;
            for (int j = 0; j < super.num_data; j++) {  //Find a free block
                if (!(dataBitmap[(i / 8)] & (1 << (i % 8)))) {
                    dataBlockNumber = i; dataBitmap[(i / 8)] |= (1 << (i % 8)); break;
                }
            }  
            
            if (dataBlockNumber == -1) { //INVALID CASES
                return -ENOTENOUGHSPACE; 
            }

            // Add the newly allocated block to the direct table
            dataBlockNumber += super.data_region_addr;

            for (int j = 0; j < DIRECT_PTRS; j++) {  //Add the block to the direct table
                if (inode.direct[j] == 0) { 
                    inode.direct[j] = dataBlockNumber; 
                }
            }
        }

        // Write the data bitmap back to disk
        for (int i = 0; i < super.data_bitmap_len; i++) {
            memcpy(buffer, (dataBitmap + (i * UFS_BLOCK_SIZE)), UFS_BLOCK_SIZE);
            disk->writeBlock((super.data_bitmap_addr + i), buffer);
        }
    }

    // Clear the direct table
    memset(buffer, 0, UFS_BLOCK_SIZE);

    for (int i = 0; i < DIRECT_PTRS; i++) { 
        disk->writeBlock(inode.direct[i], buffer); 
    }

    // Write the data in the buffer onto the disk
    int bytesToWrite = size, bytesWritten = 0;
    const unsigned char* dataBuffer = static_cast<const unsigned char*>(writeBuffer);
    for (int i = 0; i < blocksNeeded; i++) {
        // If there is an empty entry in the direct table, skip it
        int blockNum; 
        if ((blockNum = inode.direct[i]) == 0) {
             continue; 
        }

        // Copy the data from the buffer into the data block
        int toWrite = min(bytesToWrite, UFS_BLOCK_SIZE);

        memset(buffer, 0, UFS_BLOCK_SIZE); 

        memcpy(buffer, (dataBuffer + (i * UFS_BLOCK_SIZE)), toWrite); //Copy the data to the buffer

        disk->writeBlock(blockNum, buffer); //Write the data block to the disk

        bytesWritten += toWrite; 

    }   
    
    inode.size = bytesWritten;  //update the size of the inode

    int blockNumber = super.inode_region_addr + ((inodeNumber * sizeof(inode_t)) / UFS_BLOCK_SIZE); //Write the inode back to disk
    int offset = (inodeNumber * sizeof(inode_t)) % UFS_BLOCK_SIZE;

    disk->readBlock(blockNumber, buffer); //Read the block from the disk

    memcpy((buffer + offset), &inode, sizeof(inode_t)); //Copy the inode to the buffer

    disk->writeBlock(blockNumber, buffer); //Write the inode back to the disk

    return bytesWritten; 

}


int LocalFileSystem::unlink(int parentInodeNumber, string name) {
    /**
     * Remove a file or directory.
     *
     * Removes the file or directory name from the directory specified by
     * parentInodeNumber.
     *
     * Success: 0
     * Failure: -EINVALIDINODE, -EDIRNOTEMPTY, -EINVALIDNAME, -ENOTENOUGHSPACE,
     *          -EUNLINKNOTALLOWED
     * Failure modes: parentInodeNumber does not exist, directory is NOT
     * empty, or the name is invalid. Note that the name not existing is NOT
     * a failure by our definition. You can't unlink '.' or '..'
     */

    // Check if the name length is valid (0 < name < DIR_ENT_NAME_SIZE) 
    if (name.length() <= 0 || name.length() >= (DIR_ENT_NAME_SIZE - 1)) { //INVALID CASES
        return -EINVALIDNAME; 
    }

    // Check if user is attempting to unlink an unlinkable file
    if (name == "." || name == "..") { //INVALID CASES
        return -EUNLINKNOTALLOWED; 
    }

    // Load the super block
    super_t super; 
    readSuperBlock(&super); 
    unsigned char blockBuffer[UFS_BLOCK_SIZE];

    // Check if the parent inode number is valid
    inode_t parent, inode;

    if (stat(parentInodeNumber, &parent) < 0) { //INVALID CASES
        return -EINVALIDINODE; 
    }

    // If the parent inode is not a directory, return an error
    if (parent.type != UFS_DIRECTORY) { //INVALID CASES
        return -EINVALIDTYPE; 
    }

    // Get the inode number of the entry by looking up the name
    int inodeNumber = lookup(parentInodeNumber, name);

    // If the entry does not exist, terminate successfully. Otherwise get the inode of the file using the inode number
    if (inodeNumber == -ENOTFOUND) {  
        return 0; 
    }

    if (stat(inodeNumber, &inode) < 0) {  //INVALID CASES
        return -EINVALIDINODE; 
    }

    // If the directory is not empty, return an error
    if (inode.type == UFS_DIRECTORY && (long unsigned int)inode.size > (2 * sizeof(dir_ent_t))) { 
        return -EDIRNOTEMPTY; 
    }

    unsigned char inodeBitmap[UFS_BLOCK_SIZE * super.inode_bitmap_len]; //Load in the inode bitmap

    readInodeBitmap(&super, inodeBitmap); //call to the helper function

    unsigned char dataBitmap[UFS_BLOCK_SIZE * super.data_bitmap_len];  //Load in the data bitmap

    readDataBitmap(&super, dataBitmap); //call to the helper function

    // Remove the data blocks from the data bitmap
    for (int i = 0; i < DIRECT_PTRS; i++) {
        if (inode.direct[i] != 0) {
            int j = inode.direct[i] - super.data_region_addr;
            int byteIndex = (j / 8), bitIndex = (j % 8);
            dataBitmap[byteIndex] &= ~(1 << bitIndex);
            inode.direct[i] = 0;
        }
    }

    inode.size = 0; //Reset the size of the inode

    // Remove the inode from the inode bitmap
    inodeBitmap[(inodeNumber / 8)] &= ~(1 << (inodeNumber % 8));

    // Find and remove the entry in the parent's directory entries
    int position = -1;
    for (int i = 0; i < DIRECT_PTRS; i++) {
        if (parent.direct[i] != 0) {
            disk->readBlock(parent.direct[i], blockBuffer);
            dir_ent_t* entries = reinterpret_cast<dir_ent_t*>(blockBuffer);
            int numEntries = UFS_BLOCK_SIZE / sizeof(dir_ent_t);

            for (int j = 0; j < numEntries; j++) {
                if (entries[j].inum >= 0 && entries[j].name == name) {
                    position = (i * numEntries) + j;
                    entries[j].inum = -1; // Mark the entry as deleted
                    memset(entries[j].name, 0, DIR_ENT_NAME_SIZE);
                    break;
                }
            }
            if (position != -1) {
                disk->writeBlock(parent.direct[i], blockBuffer);
                break;
            }
        }
    }

    if (position == -1) {
         return -ENOTFOUND;
    }

    // Shift entries to fill the gap left by the removed entry
    int numEntries = UFS_BLOCK_SIZE / sizeof(dir_ent_t);

    for (int i = position; (long unsigned int)i < (parent.size / sizeof(dir_ent_t)) - 1; i++) { //Shift the entries to fill the gap
        int currentBlock = i / numEntries;
        int currentOffset = i % numEntries;
        int nextBlock = (i + 1) / numEntries;
        int nextOffset = (i + 1) % numEntries;

        if (currentBlock != nextBlock) {
            disk->readBlock(parent.direct[nextBlock], blockBuffer);
        }

        disk->readBlock(parent.direct[currentBlock], blockBuffer);

        dir_ent_t* entries = reinterpret_cast<dir_ent_t*>(blockBuffer);

        entries[currentOffset] = entries[nextOffset];

        disk->writeBlock(parent.direct[currentBlock], blockBuffer);
    }

    // Adjust the parent size
    parent.size -= sizeof(dir_ent_t);

    // Remove the inode from the inode region
    inode_t inodes[super.num_inodes];

    readInodeRegion(&super, inodes); //call to the helper function

    memset(&inodes[inodeNumber], 0, sizeof(inode_t));

    writeInodeRegion(&super, inodes); //call to the helper function

    writeInodeBitmap(&super, inodeBitmap); //Write the inode bitmap back to disk

    writeDataBitmap(&super, dataBitmap);  //Write the data bitmap back to disk

    inodes[parentInodeNumber] = parent; //Update the parent inode

    writeInodeRegion(&super, inodes); //Write the parent inode back to disk

    return 0; 
    
}
