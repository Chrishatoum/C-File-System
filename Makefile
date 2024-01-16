CFLAGS = -g -ansi -pedantic -Wall -std=gnu99 `pkg-config fuse --cflags --libs`
LDFLAGS = `pkg-config fuse --cflags --libs`

# List of source files
SOURCES = disk_emu.c sfs_api.c sfs_inode.c sfs_bitmap.c sfs_test2.c
# Uncomment one of the following lines as needed
# SOURCES = disk_emu.c sfs_api.c sfs_inode.c sfs_dir.c sfs_test1.c
# SOURCES = disk_emu.c sfs_api.c sfs_inode.c sfs_dir.c sfs_test2.c
# SOURCES = disk_emu.c sfs_api.c sfs_inode.c sfs_dir.c fuse_wrap_old.c
# SOURCES = disk_emu.c sfs_api.c sfs_inode.c sfs_dir.c fuse_wrap_new.c

OBJECTS = $(SOURCES:.c=.o)
EXECUTABLE = sfs

# Define header files here if needed
HEADERS = sfs_api.h

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	gcc $(OBJECTS) $(LDFLAGS) -o $@

%.o: %.c $(HEADERS)
	gcc $(CFLAGS) -c $< -o $@

clean:
	rm -rf *.o *~ $(EXECUTABLE)
