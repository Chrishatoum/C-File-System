#include "sfs_api.h"
#include "sfs_inode.h"
#include "sfs_info.h"
#include "disk_emu.h"
#include "sfs_bitmap.h"

void create_bitm() {
    // Initialize the free bitmap
    for (int i = 0; i < NUM_OF_BLOCKS; i++) {
        if (i < I_NODE_BLENGTH + 1 || i >= NUM_OF_BLOCKS - 10) {
            free_bitmap[i].is_empty = '0'; // Set block as full
        } else {
            free_bitmap[i].is_empty = '1'; // Set block as empty (data block)
        }
    }
}

void write_bitm() {
    // Calculate the size of the bitmap buffer
    size_t bitmap_buffer_size = sizeof(free_bitmap);

    // Attempt to allocate memory for the bitmap buffer
    char* bitmap_buffer = malloc(bitmap_buffer_size);

    if (bitmap_buffer == NULL) {
        fprintf(stderr, "Error: Memory allocation failed for bitmap buffer.\n");
        return;
    }

    // Copy the free_bitmap data to the buffer
    memcpy(bitmap_buffer, &free_bitmap, bitmap_buffer_size);

    // Write the free bitmap blocks to disk
    if (write_blocks(NUM_OF_BLOCKS - I_NODE_BLENGTH, I_NODE_BLENGTH, bitmap_buffer) == -1) {
        // Handle the case where write_blocks fails
        fprintf(stderr, "Error: Failed to write free bitmap to disk.\n");
    } else {
        // Successful write, print a message
        printf("Successfully wrote the free bitmap to disk.\n");
    }

    // Free the allocated memory for the bitmap buffer
    free(bitmap_buffer);
}
