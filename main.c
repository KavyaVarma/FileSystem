#include "functions.c"
//#include "header.h"

int main(int argc, char *argv[])
{
	fs = calloc(1, FS_SIZE);

	inodes = (inode *)fs;
	freemap = (int *)(fs + BLK_SIZE);
	datablks = (char *)(fs + 2*BLK_SIZE);

	printf("fs = %p\n", fs);
  	printf("inodes = %p\n", inodes);
  	printf("freemap = %p\n", freemap);
  	printf("datablks = %p\n", datablks);

	initialise_inodes(inodes);
	initialise_freemap(freemap);

	root = (dirent *)datablks;

	strcpy(root -> filename, "ROOT");
	root->used = 0;

	root -> file_inode = next_inode(inodes);
	root -> children = 0;
	(root -> file_inode) -> id = 1;
	(root -> file_inode) -> size = 30;
	(root -> file_inode) -> data = ( datablks + (next_datablock(freemap)*BLK_SIZE));
	(root -> file_inode) -> directory = true;
	(root -> file_inode) -> last_accessed = 0;
	(root -> file_inode) -> last_modified = 0;
	(root -> file_inode) -> link_count = 1;
	printf("Created Root\n");
	mine_mkdir("/tempor", 0);
	mine_mkdir("/tempor/xxx", 0);
	mine_create("/new", 0, 0);
	//mine_readdir("/", 0, 0, 0, 0);
	//mine_readdir("/tempor", 0, 0, 0, 0);
	//mine_readdir("/tempor/xxx", 0, 0, 0, 0);
	//mine_readdir("/abcd", 0, 0, 0, 0);
	mine_write("/new", "HELLO", 5, 0, NULL);
	char* buff = (char*)malloc(6);
	mine_read("/new", buff, 5, 0, NULL);
	printf("%s\n", buff);
	//printf("%d", mine_oper.readdir);
	return fuse_main(argc, argv, &mine_oper, NULL);
}

int initialise_inodes(inode* i){
	for(int x = 0; x < N_INODES; x++){
		(i+x) -> used = false;
	}
	return 0;
}

int initialise_freemap(int* map){
	int x;
	*(map) = 1;
	*(map+1) = 1;
	*(map+2) = 1;	//inodes, bitmap, root directory entry
	for(x = 0; x < DBLKS-1; x++){
		*(map + x + 3) = 0;
	}
	*(map + x) = -1;
	return 0;
}


inode* next_inode(inode *i){
	int x = 0;
	while(x<N_INODES && (i+x)->used==true)
	{
		x++;
	}
	if(x==N_INODES)
		return NULL;
	(i+x)->used = true;
	return i+x;
}

int next_datablock(int *freemap){
	int x = 0;
	while(x<DBLKS && freemap[x+2]==1)
		x++;
	if(x==DBLKS)
		return -1;
	freemap[x+2] = 1;
	return x+2;
}

void path_to_inode(const char* path, inode **ino){

	if(strcmp("/", path) == 0){
		*ino = (root->file_inode);
	}
	else{
		char *path_copy = (char*)malloc(strlen(path)+1);
		strcpy(path_copy, path);
		char *token = strtok(path_copy, "/");
		dirent *temp = root;
    while (token != NULL)
    {
    		int i = 0;
    		int children = temp->children;
    		if((temp -> file_inode) -> directory){
					temp = (dirent *)((temp -> file_inode) -> data);
			while(i<children && (temp -> filename != NULL) && (temp->used==0 || strcmp(temp -> filename, token) != 0))
			{
				temp++;
				if(temp->used==1)
					i++;
			}
			if(i>=children || (temp -> filename) == NULL){
				#ifdef DEBUG
				printf("NO SUCH PATH: %s\n", path);
				#endif
				*ino = NULL;
				return;
			}
			else{
				*ino = (temp -> file_inode);
				}
			}
			token = strtok(NULL, "/");
		}
	}
}

dirent* get_dirent(char *path)
{
	if(strcmp(path, "/")==0)
		return NULL;
	char *prevtok = strtok(path, "/");
	char *tok = strtok(NULL, "/");
	dirent *rel_root = (dirent*)root;
	while(tok!=NULL)
	{
		int x = rel_root->children;
		rel_root = (dirent*)(rel_root->file_inode->data);
		while(x!=0 && (rel_root -> filename != NULL) && (strcmp(rel_root -> filename, prevtok) != 0))
		{
			rel_root++;
			x--;
		}
		if(x==0 || (rel_root->filename==NULL))
			return NULL;

		prevtok = tok;
		tok = strtok(NULL, "/");
	}
	return rel_root;

}

void get_parent_child(const char *newpath, dirent **parent, dirent **child)
{
	char *path = (char*)malloc(strlen(newpath));
	strcpy(path, newpath);
	dirent *in_parent;
	dirent *in_child = root;

	char *prevtok = strtok(path, "/");
	char *tok = strtok(NULL, "/");

	while(prevtok!=NULL)
	{
		in_parent = in_child;
		int x = in_parent->children;
		in_child = (dirent*)(in_parent->file_inode->data);
		while(x!=0 && (in_child -> filename != NULL) && (in_child->used==0 || strcmp(in_child -> filename, prevtok) != 0))
		{
			in_child++;
			if(in_child->used==1)
				x--;
		}
		if(x==0 || (in_child->filename==NULL))
		{
			*parent = NULL;
			*child = NULL;
			return;
		}
		prevtok = tok;
		tok = strtok(NULL, "/");
	}
	*parent = in_parent;
	*child = in_child;
}

dirent* open_dirent(dirent *rel_root)
{
	inode *rel_node = rel_root->file_inode;
	int ch = rel_node->children;
	dirent* next_dir = ((dirent*)rel_root->file_inode->data);
	while(ch!=0)
	{
		if(next_dir->used==0)
			return next_dir;
		ch--;
		next_dir+=1;
	}
	return next_dir;
}

void set_data(char *dblock)
{
	char *pos = (char*)dblock;
	int x = (pos-fs)/BLK_SIZE;
	freemap[x] = 0;
}

void allocate_inode(char *path, inode **ino, bool dir){
	*ino = next_inode(inodes);
	(*ino) -> id = rand() % 10000;
	(*ino) -> size = 0;
	(*ino) -> data = ( datablks + (next_datablock(freemap)*BLK_SIZE));
	(*ino) -> directory = dir;
	(*ino) -> last_accessed = 0;
	(*ino) -> last_modified = 0;
	(*ino) -> link_count = 2;
	(*ino) -> children = 0;
}

void print_inode(inode *i){
	printf("used : %d\n", i -> used);
	printf("id : %d\n", i -> id);
	printf("size : %zu\n", i -> size);
	printf("data : %p\n", i -> data);
	printf("directory : %d\n", i -> directory);
	printf("last_accessed : %d\n", i -> last_accessed);
	printf("last_modified : %d\n", i -> last_modified);
	printf("link_count : %d\n", i -> link_count);
}