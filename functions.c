#include "header.h"

static void *mine_init(struct fuse_conn_info *conn)
{
	printf("Initialisation of The Hague\n");
	return NULL;
}

static int mine_getattr(const char *path , struct stat *stbuf)
{
	printf("\nGetAttr: %s\n", path);

	int result = 0;
	memset(stbuf, 0, sizeof(struct stat));

	inode *ino;
	path_to_inode(path, &ino);

	int directory_flag = -1;
	if(ino!=NULL)
		directory_flag = ino -> directory;
	if (directory_flag == 1) 
	{
		stbuf->st_mode = S_IFDIR | 0777;
		stbuf->st_nlink = 2;
	}
	else if (directory_flag == 0)
	{
		stbuf->st_mode = S_IFREG | 0777;
		stbuf->st_nlink = 1;
		stbuf->st_size = ino -> size;
	}
	else{
		result = -ENOENT;
	}
	return result;
}

static int mine_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi)
{
	printf("\nReadDir: %s\n", path);

	/*(void) offset;
	(void) fi;
	(void) flags;*/

	inode *ino;
	path_to_inode(path, &ino);

	if (ino == NULL){
		return -ENOENT;
	}
	else{
		dirent *temp;
		filler(buf, ".", NULL, 0);
		printf("%s\n", ".");
		filler(buf, "..", NULL, 0);
		printf("%s\n", "..");
		int x;
		if(strcmp(path, "/") == 0){
			temp = (dirent*)(root->file_inode->data);
			x = root->children;
		}
		else{
			temp = (dirent *)(ino -> data);
			x = ino->children;
		}
		while(x!=0 && (temp -> filename != NULL))
		{
			if(temp->used==1)
			{
				x--;
				printf("%s\n", temp->filename);
				filler(buf, temp -> filename, NULL, 0);
			}
			temp++;
		}
	}
	return 0;
}

static int mine_mkdir(const char *path_orig, mode_t mode)
{
	printf("\nMkDir: %s\n", path_orig);

	char *path = (char*)malloc(strlen(path_orig)+1);
	strcpy(path, path_orig);
	if(strcmp(path, "/")==0)
		return -1;
	char *prevtok = strtok(path, "/");
	char *tok = strtok(NULL, "/");
	dirent *rel_root = (dirent*)root;
	while(tok!=NULL)
	{
		int x = rel_root->children;
		rel_root = (dirent*)(rel_root->file_inode->data);
		while(x!=0 && (rel_root -> filename != NULL) && (rel_root->used==0 || strcmp(rel_root -> filename, prevtok) != 0))
		{
			rel_root++;
			if(rel_root->used==1)
				x--;
		}
		if(x==0 || (rel_root->filename==NULL))
			return -1;

		prevtok = tok;
		tok = strtok(NULL, "/");
	}

	if(rel_root==NULL || rel_root->file_inode->directory==false)
		return -1;
	dirent *newentry = open_dirent(rel_root);//((dirent*)rel_root->file_inode->data)+rel_root->children;
	rel_root->children++;
	rel_root->file_inode->children++;
	newentry->children = 0;
	newentry->used = 1;
	allocate_inode(path, &(newentry->file_inode), true);
	//newentry->filename = (char*)malloc(15);
	strcpy(newentry->filename, prevtok);
	return 0;
}

static int mine_rmdir(const char *path)
{
	printf("\nRemoving Directory %s\n", path);
	dirent *parent, *child;
	get_parent_child(path, &parent, &child);
	if(parent==NULL || child==NULL)
		return 1;
	printf("\n%s %s \n", parent->filename, child->filename);
	if(child->children>0)
		return 1;

	parent->children--;
	parent->file_inode->children--;

	child->used = 0;
	child->file_inode->used = false;
	set_data(child->file_inode->data);

	return 0;
}

static int mine_rename(const char *path, const char *buf)
{
	printf("\nRenaming %s %s\n", path, buf);
	dirent *bufparent, *bufchild;	
	get_parent_child(buf, &bufparent, &bufchild);
	mine_unlink(buf);
	dirent *parent, *child;
	get_parent_child(path, &parent, &child);
	strcpy(child->filename, bufchild->filename);
	return 0;
}

static int mine_truncate(const char *path, off_t size)
{
	printf("\nTruncating %s\n", path);
	dirent *parent, *child;
	get_parent_child(path, &parent, &child);
	child->file_inode->size = 0;
	return size;
}

static int mine_create(const char * path_orig, mode_t mode, struct fuse_file_info *fi)
{
	printf("\nCreate : %s\n", path_orig);

	char *path = (char*)malloc(strlen(path_orig)+1);
	strcpy(path, path_orig);
	if(strcmp(path, "/")==0)
		return -1;
	char *prevtok = strtok(path, "/");
	char *tok = strtok(NULL, "/");
	dirent *rel_root = (dirent*)root;
	while(tok!=NULL)
	{
		int x = rel_root->children;
		rel_root = (dirent*)(rel_root->file_inode->data);
		while(x!=0 && (rel_root -> filename != NULL) && (rel_root->used==0 || strcmp(rel_root -> filename, prevtok) != 0))
		{
			rel_root++;
			if(rel_root->used==1)
				x--;
		}
		if(x==0 || (rel_root->filename==NULL))
			return -1;

		prevtok = tok;
		tok = strtok(NULL, "/");
	}

	if(rel_root==NULL || rel_root->file_inode->directory==false)
		return -1;
	dirent *newentry = open_dirent(rel_root);//((dirent*)rel_root->file_inode->data)+rel_root->children;
	rel_root->children++;
	rel_root->file_inode->children++;
	newentry->children = 0;
	newentry->used = 1;
	allocate_inode(path, &(newentry->file_inode), false);
	//newentry->filename = (char*)malloc(15);
	strcpy(newentry->filename, prevtok);
	return 0;
}

static int mine_open(const char *path, struct fuse_file_info *fi)
{
	printf("\nOpening File: %s\n", path);
	inode *ino;
	path_to_inode(path, &ino);
	if(ino == NULL){
		return -1;
	}
	return 0;
}

static int mine_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("\nRead Called, Path: %s, ", path);
	inode *ino;
	path_to_inode(path, &ino);
	size_t len;
	(void) fi;
	if(ino == NULL)
		return -ENOENT;

	len = ino->size;
	if (offset < len) {
		if (offset + size > len)
			size = len - offset;
		memcpy(buf, (ino -> data) + offset, size);
	} else
		size = 0;

	return size;
}

static int mine_write(const char *path, const char *buf, size_t size, off_t offset, struct fuse_file_info *fi)
{
	printf("\nWrite called, Path = %s, buffer: %s, size: %zu, offset: %ld\n", path, buf, size, offset);

	inode* ino;
	path_to_inode(path, &ino);
	if(ino==NULL)
		return -ENOENT;

	memcpy((ino -> data + offset), (buf), size);
	//if(size > (ino->size-offset))
		ino -> size = size + offset;
	printf("inode file size: %lu\n", ino->size);
	return size; 
}

static int mine_utimens (const char *path, const struct timespec *tv)
{
	return 0;
}

static int mine_access (const char *path, int t)
{
	return 0;
}

static int mine_unlink (const char *path)
{
	printf("\nUnlinking %s\n", path);
	dirent *parent, *child;
	get_parent_child(path, &parent, &child);
	if(parent==NULL || child==NULL)
		return 1;
	printf("\n%s %s \n", parent->filename, child->filename);

	parent->children--;
	parent->file_inode->children--;

	child->used = 0;
	child->file_inode->used = false;
	set_data(child->file_inode->data);

	return 0;
}