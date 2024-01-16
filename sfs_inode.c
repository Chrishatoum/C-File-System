
#include "sfs_api.h"
#include "sfs_inode.h"
#include "sfs_info.h"
#include "disk_emu.h"
#include "sfs_bitmap.h"


void initialize_inode( i_node *ic, int link_cnt, int size) {
    ic->link_cnt = link_cnt;
    ic->size = size;
}

void create_itable() {
    initialize_inode(&cur_ic[0], 1, 0);
    for (int i = 1; i < I_NODE_LENGTH; i++) {
        initialize_inode(&cur_ic[i], 0, 0);
    }
}

void write_itable() {
    // Calculate the size of the inode table buffer
    size_t ic_buffer_size = sizeof(cur_ic);

    // Check if the buffer size matches the expected size to be written
    if (ic_buffer_size != BLOCK_SIZE * I_NODE_BLENGTH) {
        
        return;
    }

    // Write the inode table directly to the blocks
    if (write_blocks(1, I_NODE_BLENGTH, &cur_ic) == -1) {
        // Handle the case where write_blocks fails
        fprintf(stderr, "Error: Failed to write inode table to disk.\n");
    }
}


int find_inode() {
    int first_available = -1; // Initialize to an invalid value

    for (int i = 0; i < I_NODE_LENGTH; i++) {
        if (cur_ic[i].link_cnt == 0) {
            return i; // Return the first available inode found
        }

        if (first_available == -1 && cur_ic[i].link_cnt < cur_ic[first_available].link_cnt) {
            first_available = i; // Update the first available to the inode with the lowest link count
        }
    }

    if (first_available != -1) {
        return first_available; // If no available inode found, return the one with the lowest link count
    }

    return -1; // No available inode found
}

