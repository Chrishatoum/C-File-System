#ifndef SFS_API_H
#define SFS_API_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "sfs_inode.h"
#include "sfs_bitmap.h"
#include "sfs_info.h"



// SuperBlock Structure
typedef struct {
    int magic;
    int block_size;
    int fs_size;
    int i_node_table_length;
    int root_dir;
    int unused[216];
} SuperBlock;

// Indirect Block Structure
typedef struct {
    int size;
    int* ptr;
} indirect_block;

// Directory Entry Structure
typedef struct {
    char filename[MAX_FILENAME];
    int is_active;
    int inode_num;
} dir_entry; 


// File Descriptor Entry Structure
typedef struct {
    int inode_num;
    int rwpointer;
    int open;
} file_descriptor_entry;



// File System API Functions
void mksfs(int);
int sfs_getnextfilename(char*);
int sfs_getfilesize(const char*);
int sfs_fopen(char*);
int sfs_fclose(int);
int sfs_fwrite(int, const char*, int);
int sfs_fread(int, char*, int);
int sfs_fseek(int, int);
int sfs_remove(char*);



 extern SuperBlock cur_sb;
 extern i_node cur_ic[I_NODE_LENGTH];
 extern dir_entry dir_table[MAX_FILE_NUM];
 extern bitmap_entry free_bitmap[NUM_OF_BLOCKS];


#endif // SFS_API_H
