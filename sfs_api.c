#include <stdbool.h>
#include "sfs_api.h"
#include "sfs_inode.h"
#include "sfs_info.h"
#include "disk_emu.h"
#include "sfs_bitmap.h"


#define FILE_INACTIVE 0 // Define inactive state
#define FILE_ACTIVE 1 // Define inactive state

#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif


char cur_filename[] = "Chris_file_sys";
SuperBlock cur_sb;                 //initialize super block
i_node cur_ic[I_NODE_LENGTH]; //initialize i cache
dir_entry dir_table[MAX_FILE_NUM];  //initialize directory table
bitmap_entry free_bitmap[NUM_OF_BLOCKS]; //has all blocks in it

//in-memory only
file_descriptor_entry fdt[MAX_FILE_NUM];// 79 files and every one of them has a descriptor
int cur_file = 0;

// Defined numbers 
#define SUPERBLOCK_MAGIC_NUMBER 0xACBD0005

// initializing methods
void initialize_super_block(SuperBlock *sb) {
    sb->magic = SUPERBLOCK_MAGIC_NUMBER;
    sb->block_size = BLOCK_SIZE;
    sb->fs_size = NUM_OF_BLOCKS;
    sb->i_node_table_length = I_NODE_BLENGTH;
    sb->root_dir = 0;
}



void initializeDirectoryTable() {
    for (int index = 0; index < MAX_FILE_NUM; index++) {
        dir_table[index].is_active = 0; // Initialize all entries as inactive
    }
}


//HELPER FUNCTIONS

SuperBlock get_cur_sb(){
    return cur_sb;
}

i_node get_cur_ic(){
    return cur_ic[I_NODE_LENGTH];
}

SuperBlock create_super_block() {
    SuperBlock sb;
    initialize_super_block(&sb);
    return sb;
}

void write_dir_table() {
    char* dir_buffer = malloc(BLOCK_SIZE * I_NODE_BLENGTH);
    if (dir_buffer == NULL) {
        // Handle memory allocation failure
        fprintf(stderr, "Error: Memory allocation failed for directory table buffer.\n");
        return;
    }

    memcpy(dir_buffer, dir_table, sizeof(*dir_buffer) * BLOCK_SIZE * I_NODE_BLENGTH);
    write_blocks(NUM_OF_BLOCKS - I_NODE_BLENGTH - I_NODE_BLENGTH, I_NODE_BLENGTH, dir_buffer);
    
    free(dir_buffer);

}

int file_index_dir (const char* filename) {
    // Check for a valid input filename
    if (filename == NULL) {
        fprintf(stderr, "Error: Invalid filename.\n");
        return -1;
    }

    // Iterate over the directory table to find the filename
    for (int index = 0; index < MAX_FILE_NUM; index++) {
        // Check if the current directory entry is active and its filename matches
        if (dir_table[index].is_active == FILE_ACTIVE &&
            strcmp(filename, dir_table[index].filename) == 0) {
            return index; // Directory index found for the file
        }
    }

    return -1; // Return -1 if the file is not found
}




void ini_ip(indirect_block* ind) {
    // Define INDIRECT_POINTER_SIZE as a constant
    const int INDIRECT_POINTER_SIZE = INDIRECT_POINTER;

    // Allocate memory for indirect pointers
    int* ptr = (int*)calloc(INDIRECT_POINTER_SIZE, sizeof(int));

    if (ptr != NULL) {
        // Initialize the size and pointers
        ind->ptr = ptr;
        ind->size = 0;
    } else {
        fprintf(stderr, "Error: Memory allocation failed for indirect pointers.\n");
        // Handle memory allocation failure here, e.g., by returning an error code or exiting the program.
    }
}




bool read_ib(indirect_block* ind, int i_node_num) {
    // Check if the indirect pointer is valid
    if (cur_ic[i_node_num].indirect_pointer == -1) {
        fprintf(stderr, "Error: Invalid indirect pointer.\n");
        return false; // Indicate failure
    }

    // Allocate memory for the indirect buffer
    char* indirect_buffer = malloc(BLOCK_SIZE);

    if (indirect_buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for indirect buffer.\n");
        return false; // Indicate failure
    }

    // Read the indirect block data from disk
    if (read_blocks(cur_ic[i_node_num].indirect_pointer, 1, indirect_buffer) == -1) {
        fprintf(stderr, "Error: Failed to read indirect block from disk.\n");
        free(indirect_buffer);
        return false; // Indicate failure
    }

    // Copy data from the buffer to the indirect_block structure
    memcpy(ind, indirect_buffer, sizeof(int)); // Size copied
    memcpy(ind->ptr, indirect_buffer + sizeof(int), INDIRECT_POINTER * sizeof(int)); // Pointers copied

    // Free the allocated memory for the indirect buffer
    free(indirect_buffer);

    return true; // Indicate success
}


// Essential functions
void mksfs(int fresh) {
    if (fresh == 1) { // Create a new file system from scratch
        // Initialize disk file
        if (init_fresh_disk(SFS_FILENAME, BLOCK_SIZE, NUM_OF_BLOCKS) == -1) {
            printf("Internal Error: mksfs function | Couldn't allocate disk space, external function error!\n");
            return;
        }

        // Create and write the super block
        create_super_block();
        write_blocks(0, 1, &cur_sb);

        // Create and write the inode table
        create_itable();
        write_itable();

        // Initialize and write the directory table
        initializeDirectoryTable();
        write_dir_table();

        // Create and write the free bitmap
        create_bitm();
        write_bitm();
    } else { // An existing file system is in place
        // Read the super block from the first block
        read_blocks(0, 1, &cur_sb);

        // Read the inode table
        read_blocks(1, I_NODE_BLENGTH, &cur_ic);

        // Read the directory table
        read_blocks(NUM_OF_BLOCKS - I_NODE_BLENGTH - I_NODE_BLENGTH, I_NODE_BLENGTH, &dir_table);

        // Read the free bitmap
        read_blocks(NUM_OF_BLOCKS - I_NODE_BLENGTH, I_NODE_BLENGTH, &free_bitmap);
    }
}


int sfs_getnextfilename(char* fname){

// Ensure fname is not a NULL pointer
    if (fname == NULL) {
        return 0;
    }

     if(cur_file >= MAX_FILE_NUM){
        return 0; //no file left in directory
    }

      while (cur_file <= MAX_FILE_NUM) {
       
            // Use strncpy to safely copy the file name
            strcpy(fname, dir_table[cur_file].filename);
         

            cur_file++; // Increment for the next call
            return 1; // Indicate that a file name has been found
        }
        cur_file++; // Move to the next file if the current one is inactive
    
    }

int sfs_getfilesize(const char* path) {
    if (path == NULL) {
        return -1; // Return -1 if the path pointer is NULL
    }

    // Find the file index in the directory table
    int file_index = file_index_dir (path);

    // Check if the file is found in the directory table
    if (file_index != -1) {
        // If there is a file, return the size attribute from the inode structure
        return cur_ic[dir_table[file_index].inode_num].size;
    } else {
        // If the file is not found, return -1
        return -1;
    }
}


int sfs_fopen(char* fname) {
    // Check if the filename exceeds the maximum length
    if (strlen(fname) > MAX_FILENAME) {
        printf("Tried to create a file with a filename exceeding the limit.\n");
        return -1;
    }

    int file_index = file_index_dir(fname);

    if (file_index != -1) { // File already exists
        int i_node_index = dir_table[file_index].inode_num;

        // Check if the file is already open
        for (int i = 0; i < MAX_FILE_NUM; i++) {
            if (fdt[i].open == 1 && fdt[i].inode_num == i_node_index) {
                printf("File already open\n");
                return -1;
            }
        }

        // Find an available entry in the file descriptor table
        int fdt_index = -1;

        for (int i = 0; i < MAX_FILE_NUM; i++) {
            if (fdt[i].open == 0) {
                fdt_index = i;
                break;
            }
        }

        if (fdt_index != -1) {
            // Update file descriptor entry
            fdt[fdt_index].inode_num = i_node_index;
            fdt[fdt_index].open = 1; // Is active
            fdt[fdt_index].rwpointer = cur_ic[i_node_index].size;
            return fdt_index;
        }
    } else { // Create a new file
        int i_node_allocation_num = find_inode();

        if (i_node_allocation_num != -1) {
            // Update i_node entry
            cur_ic[i_node_allocation_num].link_cnt = 1;

            // Find an available entry in the file descriptor table
            int fdt_index = -1;

            for (int i = 0; i < MAX_FILE_NUM; i++) {
                if (fdt[i].open == 0) {
                    fdt_index = i;
                    break;
                }
            }

            if (fdt_index != -1) {
                // Update file descriptor entry
                fdt[fdt_index].inode_num = i_node_allocation_num;
                fdt[fdt_index].open = 1; // Is active
                fdt[fdt_index].rwpointer = 0;

                // Update directory entry (change active state of entry)
                int dir_table_index = -1;

                for (int i = 0; i < MAX_FILE_NUM; i++) {
                    if (dir_table[i].is_active == 0) {
                        dir_table_index = i;
                        break;
                    }
                }

                if (dir_table_index != -1) {
                    dir_table[dir_table_index].is_active = 1;
                    strcpy(dir_table[dir_table_index].filename, fname);
                    dir_table[dir_table_index].inode_num = i_node_allocation_num;

                    // Update inode table, directory table, and return the file descriptor index
                    write_itable();
                    write_dir_table();
                    return fdt_index;
                }
            }
        } else {
            printf("Internal Error: I-NODE limit exceeded!\n");
        }
    }

    return -1; // Return -1 for error or if the file is already open
}




typedef struct {
    int inode;
    int num_bytes_written;
    indirect_block *ind;
    int valid; // This is to indicate if the setup is successful
} WriteSetup;

WriteSetup initial_write_setup(int fileID) {
    WriteSetup setup;
    setup.inode = -1;
    setup.num_bytes_written = 0;
    setup.ind = NULL;
    setup.valid = 1; // Assume valid unless checks fail

    if(fileID >= MAX_FILE_NUM) {
        printf("Invalid file ID!\n");
        setup.valid = 0;
        return setup;
    }
    if(fdt[fileID].open == 0) {
        printf("File not open!\n");
        setup.valid = 0;
        return setup;
    }

    setup.inode = fdt[fileID].inode_num;
    // Additional setup can be added here if needed

    return setup;
}

int determine_block_number_for_write(int fileID, int inode, indirect_block *ind) {
    int rw = fdt[fileID].rwpointer;
    if (rw > DIRECT_POINTER * BLOCK_SIZE) {
        int indirect_block_num = (rw / BLOCK_SIZE) - DIRECT_POINTER;
        return ind->ptr[indirect_block_num];
    } else {
        return cur_ic[inode].direct_pointers[rw / BLOCK_SIZE];
    }
}

int calculate_bytes_to_write(int rwpointer, int length) {
    int last_point = rwpointer % BLOCK_SIZE;
    return (last_point + length > BLOCK_SIZE) ? BLOCK_SIZE - last_point : length;
}
void perform_block_write(int block_num, const char **buf, int num_bytes_to_write, int *num_bytes_written, int inode, int fileID) {
    char *fill_buffer = malloc(BLOCK_SIZE);
    read_blocks(block_num, 1, fill_buffer);
    memcpy(fill_buffer + (fdt[fileID].rwpointer % BLOCK_SIZE), *buf, num_bytes_to_write);
    write_blocks(block_num, 1, fill_buffer);
    free(fill_buffer);

    *buf += num_bytes_to_write;
    *num_bytes_written += num_bytes_to_write;
    cur_ic[inode].size += num_bytes_to_write;
}

void update_file_write_info(int fileID, int num_bytes_written) {
    fdt[fileID].rwpointer += num_bytes_written;
}

indirect_block* initialize_and_update_indirect_block(int fileID, int inode, int block_index) {
    if (block_index >= DIRECT_POINTER) {
        indirect_block *ind = malloc(BLOCK_SIZE);
        if (ind == NULL) {
            // Handle memory allocation error
            return NULL;
        }
        ini_ip(ind);
        read_ib(ind, inode); // Update the indirect block
        return ind;
    }
    return NULL; // No indirect block needed
}


int calculate_required_blocks(int length) {
    int required_blocks = (length + BLOCK_SIZE - 1) / BLOCK_SIZE; // More efficient calculation
    return required_blocks;
}

int allocate_new_block() {
    for (int i = 0; i < NUM_OF_BLOCKS; i++) {
        if (free_bitmap[i].is_empty == '1') {
            free_bitmap[i].is_empty = '0';
            return i;
        }
    }
    return -1; // Indicate failure
}
void write_data_to_new_block(const char **buf, int *temp_length, int allocation_num, int *num_bytes_written) {
    int num_bytes_to_write = MIN(*temp_length, BLOCK_SIZE);
    char *write_buf = malloc(BLOCK_SIZE);
    memcpy(write_buf, *buf, num_bytes_to_write);
    write_blocks(allocation_num, 1, write_buf);
    free(write_buf);

    *temp_length -= num_bytes_to_write;
    *buf += num_bytes_to_write;
    *num_bytes_written += num_bytes_to_write;
}
void update_inode_pointers(int inode, indirect_block **ind, int allocation_num, int *indirect_block_num) {
    int pointer_num = cur_ic[inode].size / BLOCK_SIZE;
    if (pointer_num >= DIRECT_POINTER) {
        // Handle indirect pointers
  
        (*ind)->ptr[(*ind)->size++] = allocation_num;
    } else {
        cur_ic[inode].direct_pointers[pointer_num] = allocation_num;
    }
}

indirect_block* allocate_and_init_indirect_block(int inode, int *ind_block_num) {
    // Allocate memory for the indirect block
    indirect_block *ind = malloc(BLOCK_SIZE);
    if (!ind) {
        // Memory allocation failed
        return NULL;
    }

    // Initialize the indirect block
    ini_ip(ind); // Assuming ini_ip initializes the indirect block

    // Find a free block for the indirect block itself
    int allocated_block = -1;
    for (int i = 0; i < NUM_OF_BLOCKS; i++) {
        if (free_bitmap[i].is_empty == '1') {
            allocated_block = i;
            free_bitmap[i].is_empty = '0';
            break;
        }
    }

    if (allocated_block == -1) {
        // No free block found, handle this error
        free(ind);
        return NULL;
    }

    // Update the inode's indirect pointer
    cur_ic[inode].indirect_pointer = allocated_block;
    if (ind_block_num != NULL) {
        *ind_block_num = allocated_block;
    }

    // Optionally, you might want to write the updated indirect block to disk here
    // For example, write_blocks(cur_ic[inode].indirect_pointer, 1, ind);

    return ind;
    
}



    
int sfs_fwrite(int fileID , const char* buf , int length){
     WriteSetup setup = initial_write_setup(fileID);

    if (!setup.valid) {
        return -1; // Early exit if setup is invalid
    }

    indirect_block *ind = setup.ind;
    int inode = setup.inode;
    int num_bytes_written = setup.num_bytes_written;
    int block_index = fdt[fileID].rwpointer / BLOCK_SIZE;
   

    if (fdt[fileID].rwpointer > 0) {
        ind = initialize_and_update_indirect_block(fileID, setup.inode, block_index);
        
        if (block_index >= DIRECT_POINTER && !ind) {
            // Handle the case where indirect block initialization fails
            return -1;
        }

  if (fdt[fileID].rwpointer % BLOCK_SIZE != 0) {
    int block_num = determine_block_number_for_write(fileID, setup.inode, ind);
    int num_bytes_to_write = calculate_bytes_to_write(fdt[fileID].rwpointer, length);

    perform_block_write(block_num, &buf, num_bytes_to_write, &num_bytes_written, setup.inode, fileID);
    update_file_write_info(fileID, num_bytes_to_write);
    length -= num_bytes_to_write;
}

    }
    int num_bytes_to_write = 0;
    int ind_block_num; // Declare ind_block_num



if (length > 0) {
    int required_blocks = (length + BLOCK_SIZE - 1) / BLOCK_SIZE;
    int i = 0;
    int temp_length = length;

    while (i < required_blocks) {
        // Find a free block
        int j = 0;
        int allocation_num = -1;
        while (j < NUM_OF_BLOCKS) {
            if (free_bitmap[j].is_empty == '1') {
                allocation_num = j;
                free_bitmap[j].is_empty = '0';
                break;
            }
            j++;
        }

        if (allocation_num == -1) {
            // Handle the case where no free block is found
            break;
        }

        // Write data to the new block
        int num_bytes_to_write = (temp_length > BLOCK_SIZE) ? BLOCK_SIZE : temp_length;
        char *write_buf = malloc(BLOCK_SIZE);
        memcpy(write_buf, buf, num_bytes_to_write);
        write_blocks(allocation_num, 1, write_buf);
        free(write_buf);

        // Read back the written block for verification (optional)
        char *fill_buffer = malloc(BLOCK_SIZE);
        read_blocks(allocation_num, 1, fill_buffer);
        free(fill_buffer);

        // Update inode pointers
        int pointer_num = cur_ic[inode].size / BLOCK_SIZE;
        if (pointer_num >= DIRECT_POINTER) {
            if (!ind) {
                ind = malloc(BLOCK_SIZE);
                ini_ip(ind);
                int ind_block_num = -1;
                int k = 0;
                while (k < NUM_OF_BLOCKS) {
                    if (free_bitmap[k].is_empty == '1') {
                        ind_block_num = k;
                        free_bitmap[k].is_empty = '0';
                        break;
                    }
                    k++;
                }
                if (ind_block_num == -1) {
                    free(ind);
                    break; // No free block for indirect pointer
                }
                cur_ic[inode].indirect_pointer = ind_block_num;
                char *ind_buffer = malloc(BLOCK_SIZE);
                memcpy(ind_buffer, ind, sizeof(int));
                memcpy(ind_buffer + sizeof(int), ind->ptr, INDIRECT_POINTER * sizeof(int));
                write_blocks(cur_ic[inode].indirect_pointer, 1, ind_buffer);
                free(ind_buffer);
            }
            if (ind->size >= INDIRECT_POINTER) {
                break; // Indirect block size limit exceeded
            }
            ind->ptr[ind->size++] = allocation_num;
        } else {
            cur_ic[inode].direct_pointers[pointer_num] = allocation_num;
        }

        // Update variables for the next iteration
        temp_length -= num_bytes_to_write;
        buf += num_bytes_to_write;
        fdt[fileID].rwpointer += num_bytes_to_write;
        num_bytes_written += num_bytes_to_write;
        cur_ic[inode].size += num_bytes_to_write;
        i++;
    }
}



    if(ind != 0){
            //!!!if there is an indirection block update and write it again
            //printf("Writing updated indirect block to: %d\n",cur_ic[inode].indirect_pointer);
            char *ind_buffer = malloc(BLOCK_SIZE);
            memcpy(ind_buffer, ind, sizeof(int)); //DIFFERENT PROCESS TO WRITE BECAUSE IT IS A STRUCT POINTER!!!!
            memcpy(ind_buffer + sizeof(int), ind->ptr, INDIRECT_POINTER * sizeof(int));
            write_blocks(cur_ic[inode].indirect_pointer, 1, ind_buffer);
            //printf("Written ind size: %d\n", ind->size);

            free(ind_buffer);
            free(ind->ptr);
            free(ind);

            //free just to be safe
    }


    write_itable();
    write_dir_table();
    write_bitm();
    return num_bytes_written;
}

int sfs_fclose(int fileID) {
    

    if (fdt[fileID].open == 1) {
        // Close the file
        fdt[fileID].open = 0;
        return 0; // File closed successfully
    } 
    
    if (fileID < 0 || fileID >= MAX_FILE_NUM) {
        return -1; // Invalid file number
    }
    else {
        return -1; // File is already closed
    }
}

int sfs_remove(char* file) {
    int removal_index = -1; // Directory index of the file to be removed
    int inode = -1;

    // Find the file to remove and its corresponding inode
    int i = 0;
    while (i < MAX_FILE_NUM && removal_index == -1) {
        if (dir_table[i].is_active == 1 && strcmp(dir_table[i].filename, file) == 0) {
            removal_index = i;
            inode = dir_table[i].inode_num;
        }
        i++;
    }

    // Check if the file was found
    if (removal_index == -1) {
        printf("File not found: %s\n", file);
        return -1;
    }

    // Free data blocks
    int pointer_num = (int)(cur_ic[inode].size / BLOCK_SIZE);
    i = 0;
    while (i < pointer_num) {
        if (i >= DIRECT_POINTER) { // Indirect removal
            if (cur_ic[inode].indirect_pointer != -1) {
                int ind_block_num = cur_ic[inode].indirect_pointer;
                int deallocation_num = cur_ic[inode].indirect_pointer;
                free_bitmap[deallocation_num].is_empty = '1'; // Free the block in indirect pointer
            }
        } else { // Direct pointer removal
            int deallocation_num = cur_ic[inode].direct_pointers[i];
            free_bitmap[deallocation_num].is_empty = '1'; // Free the block in direct pointer
        }
        i++;
    }

    // Reset inode metadata
    cur_ic[inode].size = 0;
    cur_ic[inode].link_cnt = 0;

    // Compact the directory table by removing the file entry
    int dir_size = 0;
    i = 0;
    while (i < MAX_FILE_NUM) {
        if (dir_table[i].is_active == 1) {
            dir_size++;
        }
        i++;
    }

    i = removal_index;
    while (i < dir_size - 1) {
        dir_table[i] = dir_table[i + 1]; // Move entries to fill the gap
        i++;
    }

    // Clear the last entry
    dir_table[dir_size - 1].is_active = 0;

    // Free the memory occupied by the indirect block
    if (cur_ic[inode].indirect_pointer != -1) {
        int deallocation_num = cur_ic[inode].indirect_pointer;
        free_bitmap[deallocation_num].is_empty = '1'; // Free the block
    }

    // Write changes to disk
    write_itable();
    write_dir_table();
    write_bitm();

    return 0;
}

int sfs_fread(int fileID, char* buf, int length) {
    if (fileID >= MAX_FILE_NUM || length < 0 || fdt[fileID].open == 0) {
        // Check for invalid fileID, negative length, or closed file
        printf("Invalid file or length\n");
        return -1;
    }

    int num_bytes_read = 0;
    int inode = fdt[fileID].inode_num;
    int data_block_place = (int)(fdt[fileID].rwpointer / BLOCK_SIZE);
    int num_bytes_left_to_read = length;
    indirect_block* ind = NULL;

    if (length > cur_ic[inode].size) {
        // If length is greater than the file size, read up to the file size
        num_bytes_left_to_read = cur_ic[inode].size;
    }

    for (; num_bytes_left_to_read > 0; data_block_place++) {
        int num_bytes_being_read = 0;
        int read_block_num;

        if (data_block_place >= DIRECT_POINTER) {
            // Indirect pointers are needed
            if (ind == NULL) {
                ind = malloc(BLOCK_SIZE);
                ini_ip(ind);
                read_ib(ind, inode);
            }
            read_block_num = ind->ptr[data_block_place - DIRECT_POINTER];
        } else {
            // Use direct pointers
            read_block_num = cur_ic[inode].direct_pointers[data_block_place];
        }

        if (fdt[fileID].rwpointer % BLOCK_SIZE + num_bytes_left_to_read > BLOCK_SIZE) {
            num_bytes_being_read = BLOCK_SIZE - fdt[fileID].rwpointer % BLOCK_SIZE;
        } else {
            num_bytes_being_read = num_bytes_left_to_read;
        }

        // Read file block
        char* read_buffer = malloc(BLOCK_SIZE);
        read_blocks(read_block_num, 1, read_buffer);
        memcpy(buf + num_bytes_read, read_buffer + fdt[fileID].rwpointer % BLOCK_SIZE, num_bytes_being_read);
        free(read_buffer);

        // Update pointers and variables
        num_bytes_left_to_read -= num_bytes_being_read;
        num_bytes_read += num_bytes_being_read;
        fdt[fileID].rwpointer += num_bytes_being_read;
    }

    if (ind != NULL) {
        free(ind->ptr);
        free(ind);
    }

    return num_bytes_read;
}



int sfs_fseek(int fileID, int loc){
    //straightforward pointer mover
    int fdt_index = fileID;
    if(fdt_index >= 0){
        fdt[fdt_index].rwpointer = loc;
        return 0;
    }
    else{
        return -1;
    }

} //moves file pointer to location






