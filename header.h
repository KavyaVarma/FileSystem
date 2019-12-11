#define FUSE_USE_VERSION 31
#include <fuse.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <errno.h>
#include <sys/time.h>


static void *mine_init (struct fuse_conn_info *conn);
static int mine_getattr (const char *, struct stat *);
static int mine_readdir (const char *, void *, fuse_fill_dir_t, off_t, struct fuse_file_info *);
static int mine_mkdir (const char *, mode_t);
static int mine_rmdir (const char *);
static int mine_rename (const char *, const char *);
static int mine_truncate (const char *, off_t);
static int mine_create (const char *, mode_t, struct fuse_file_info *);
static int mine_open (const char *, struct fuse_file_info *);
static int mine_read (const char *, char *, size_t, off_t, struct fuse_file_info *);
static int mine_write (const char *, const char *, size_t, off_t, struct fuse_file_info *);
static int mine_utimens (const char *, const struct timespec*);
static int mine_access (const char *, int);
static int mine_unlink (const char *);


static struct fuse_operations mine_oper = {
	.init       = mine_init,
	.getattr    = mine_getattr,
	.readdir    = mine_readdir,
	.mkdir		= mine_mkdir,
	.rmdir		= mine_rmdir,
	.rename		= mine_rename,
	.truncate	= mine_truncate,
	.open       = mine_open,
	.create     = mine_create,
	.read       = mine_read,
	.write      = mine_write,
	.utimens    = mine_utimens,
	.access     = mine_access,
	.unlink 	= mine_unlink,
};

typedef struct {
	bool used;                  // valid inode or not
	int id;						// ID for the inode
	size_t size;				// Size of the file
	char *data;					// pointer to data block
	bool directory;				// true if its a directory else false
	int last_accessed;			// Last accessed time
	int last_modified;			// Last modified time
	int link_count; 			// 2 in case its a directory, 1 if its a file
	int children;

} __attribute__((packed, aligned(1))) inode;	//38 bytes

typedef struct {
	int magic;
	size_t blocks;
	size_t iblocks;
	size_t inodes;
} __attribute__((packed, aligned(1))) sblock;

typedef struct {
	int id;			// inode id
	inode *in;      // pointer to inode
}OPEN_FILE_TABLE_ENTRY;

typedef struct{
	char filename[30];
	inode *file_inode;
	int children;
	int used;
}dirent;

#define SBLK_SIZE 16
#define BLK_SIZE 4096

#define N_INODES 50
#define DBLKS_PER_INODE 1
#define DBLKS 50

#define FREEMAP_BLKS 1
#define INODE_BLKS 1

#define FS_SIZE (INODE_BLKS + FREEMAP_BLKS + DBLKS) * BLK_SIZE


#define MAX_NO_OF_OPEN_FILES 10

#define DEBUG

int initialise_inodes(inode* i);
int initialise_freemap(int* map);
inode* next_inode(inode *i);
int next_datablock(int *freemap);
void path_to_inode(const char* path, inode **ino);
void allocate_inode(char *path, inode **ino, bool dir);
void print_inode(inode *i);
dirent* get_dirent(char *path);
dirent* open_dirent(dirent *rel_root);
void get_parent_child(const char *path, dirent **parent, dirent **child);
void set_data(char *dblock);


char *fs;												// The start of the FileSystem in the memory
inode *inodes;											// The start of the inode block
int *freemap;											// The start of the free-map block
char *datablks;											// The start of the data_blocks
OPEN_FILE_TABLE_ENTRY open_table[MAX_NO_OF_OPEN_FILES]; // The open file array

dirent *root; // Address of the block representing the root directory
