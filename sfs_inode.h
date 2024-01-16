#ifndef SFS_INODE_H
#define SFS_INODE_H



#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DIRECT_POINTER_NUM 12

// i-node Structure
typedef struct {
    int mode;
    int link_cnt;
    int size;   
    int direct_pointers[DIRECT_POINTER_NUM];
    int indirect_pointer;
} i_node; 



#endif //SFS_INODE_H