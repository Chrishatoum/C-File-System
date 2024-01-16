#ifndef SFS_INFO_H
#define SFS_INFO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>


// i-node Table Constants
#define I_NODE_BLENGTH 5
#define I_NODE_LENGTH 60
#define MAX_FILE_NUM I_NODE_LENGTH -1
#define MAX_FILENAME 60

// File System Constants
#define SFS_FILENAME "Chris_file_system"
#define BLOCK_SIZE 1024
#define NUM_OF_BLOCKS 4096


// Pointer Constants
#define INDIRECT_POINTER 30
#define DIRECT_POINTER 12




#endif