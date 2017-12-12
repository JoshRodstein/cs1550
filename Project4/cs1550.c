/*
	FUSE: Filesystem in Userspace
	Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>

	This program can be distributed under the terms of the GNU GPL.
	See the file COPYING.


	Modified By: Joshua L. Rodtein
	email: jor94@pitt.edu
	cs1550 University of Pittsburgh
	Project 4 
*/

#define	FUSE_USE_VERSION 26

#include <fuse.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>


//size of a disk block
#define	BLOCK_SIZE 512

//we'll use 8.3 filenames
#define	MAX_FILENAME 8
#define	MAX_EXTENSION 3

//How many files can there be in one directory?
#define MAX_FILES_IN_DIR (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + (MAX_EXTENSION + 1) + sizeof(size_t) + sizeof(long))

//The attribute packed means to not align these things
struct cs1550_directory_entry
{
	int nFiles;	//How many files are in this directory.
				//Needs to be less than MAX_FILES_IN_DIR

	struct cs1550_file_directory
	{
		char fname[MAX_FILENAME + 1];	//filename (plus space for nul)
		char fext[MAX_EXTENSION + 1];	//extension (plus space for nul)
		size_t fsize;					//file size
		long nStartBlock;				//where the first block is on disk
	} __attribute__((packed)) files[MAX_FILES_IN_DIR];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_FILES_IN_DIR * sizeof(struct cs1550_file_directory) - sizeof(int)];
} ;

typedef struct cs1550_root_directory cs1550_root_directory;

#define MAX_DIRS_IN_ROOT (BLOCK_SIZE - sizeof(int)) / ((MAX_FILENAME + 1) + sizeof(long))

struct cs1550_root_directory
{
	int nDirectories;	//How many subdirectories are in the root
						//Needs to be less than MAX_DIRS_IN_ROOT
	struct cs1550_directory
	{
		char dname[MAX_FILENAME + 1];	//directory name (plus space for nul)
		long nStartBlock;				//where the directory block is on disk
	} __attribute__((packed)) directories[MAX_DIRS_IN_ROOT];	//There is an array of these

	//This is some space to get this to be exactly the size of the disk block.
	//Don't use it for anything.  
	char padding[BLOCK_SIZE - MAX_DIRS_IN_ROOT * sizeof(struct cs1550_directory) - sizeof(int)];
} ;


typedef struct cs1550_directory_entry cs1550_directory_entry;

//How much data can one block hold?
#define	MAX_DATA_IN_BLOCK (BLOCK_SIZE - sizeof(long))

struct cs1550_disk_block
{
	//The next disk block, if needed. This is the next pointer in the linked 
	//allocation list
	long nNextBlock;

	//And all the rest of the space in the block can be used for actual data
	//storage.
	char data[MAX_DATA_IN_BLOCK];
};

typedef struct cs1550_disk_block cs1550_disk_block;

static int link_blocks(cs1550_directory_entry , int, long);
static long next_free_block();
static long get_dir_index(char*);
static int get_file(cs1550_directory_entry , cs1550_disk_block *, char *, char*);
static int get_dir(cs1550_directory_entry*, long);
static int change_free();

// accepts sub_dir, previous file index, and new file offset
static int link_blocks(cs1550_directory_entry directory, int index, long offset){
  cs1550_disk_block data;

  printf("\tENTER: link_blocks()\n");

  FILE *fp = fopen(".disk", "rb+");
  if(fp == NULL){
    return -1;
  }

  fseek(fp, directory.files[index].nStartBlock, SEEK_SET);
  fread((void*)&data, sizeof(cs1550_disk_block), 1, fp);
  data.nNextBlock = offset;
  rewind(fp);
  fseek(fp, directory.files[index].nStartBlock, SEEK_SET);
  fwrite((void*)&data, sizeof(cs1550_disk_block), 1, fp);

  printf("\t\tBLock @ directory[%d] w/ offset %ld linked to offset %ld\n", index, directory.files[index].nStartBlock, offset);
  fclose(fp);
  return 0;
}

// find next free block and sets the pointer
static int change_free(){
  cs1550_disk_block free_ptr;
  
  FILE *fp = fopen(".disk", "rb+");
  fseek(fp, sizeof(cs1550_root_directory), SEEK_SET);
  fread((void*)&free_ptr, sizeof(cs1550_disk_block), 1, fp);
  free_ptr.nNextBlock += BLOCK_SIZE;
  rewind(fp);
  fseek(fp, sizeof(cs1550_root_directory), SEEK_SET);
  fwrite((void*)&free_ptr, sizeof(cs1550_disk_block), 1, fp);
  fclose(fp);

  return 0;
}
// get next free block from free space tracking disk_block
static long next_free_block(){
  long index = -1;
  cs1550_root_directory root;
  cs1550_disk_block free_block;
  long ptr;

  FILE *fp = fopen(".disk", "rb+");
  printf("\tENTER: next_free_block()\n");
  fread((void*)&root, sizeof(cs1550_root_directory), 1, fp);
  fread((void*)&free_block, sizeof(cs1550_disk_block), 1, fp);
  
  if(root.nDirectories == 0){
    free_block.nNextBlock = BLOCK_SIZE * 2;
    rewind(fp);
    fseek(fp, sizeof(cs1550_root_directory), SEEK_SET);
    fwrite((void*)&free_block, sizeof(cs1550_disk_block), 1, fp);
    printf("\t\tFirst Allocation: Initializing free block point to: %ld\n", free_block.nNextBlock);
  } 

  

  index = free_block.nNextBlock;

  fseek(fp, 0, SEEK_END);
  ptr = ftell(fp);
  if(ptr < index){
    printf("END OF FILE\n");
    return -1;
  }
  printf("\tNext Free Block @ offset: %ld\n", index);
  fclose(fp);
  return index;
}

// find directoty, return startBlock of dir in ROOT if found, return -1 if does not exist                                             
static long get_dir_index(char *dirs){
  long found = -1;
  cs1550_root_directory root_dir;

  FILE *fp = fopen(".disk", "rb");
  if(fp == NULL){
    return -1;
  }

  printf("\tENTER: get_dir_index()\n");
  // grab root from disk
  if(fread((void*)&root_dir, sizeof(cs1550_root_directory), 1, fp) < 1){
    return -1;
  }
  fclose(fp);

  int i = 0;
  while(i < MAX_DIRS_IN_ROOT){
    if(strcmp(root_dir.directories[i].dname, dirs) == 0){
      found = root_dir.directories[i].nStartBlock;
      printf("\t\tDirectory is at index %d in ROOT @ OFFSET: %ld\n", i, found);
      break;
    } 
    i++;
  }

  // return nStartBlock of directory if found, -1 if not
  return found;
}

// find cur_dir at passed index and fill struct
static int get_dir(cs1550_directory_entry *cur_dir, long block){
  long found = -1;
  FILE *fp = fopen(".disk", "rb");
  printf("\tEnter: get_dir()\n");
  if(fp == NULL) {
    return -1;
  }
  
  if(fseek(fp, block, SEEK_SET) == -1){
    return -1;
  }
  printf("\t\tSeek to offset: %ld\n", block);
  if(fread(cur_dir, sizeof(cs1550_directory_entry), 1, fp) == 1){
    found = 1;
  }

  fclose(fp);
  return found;

}

// find file index in directory, fill file struct
static int get_file(cs1550_directory_entry dir, cs1550_disk_block *file, char *filename, char* extension){
  long found = -1;
  FILE *fp = fopen(".disk", "rb");
  if(fp == NULL){
    return -1;
  }

  printf("\tENTER: get_file()\n");

  int i = 0;
  while(i < dir.nFiles){
    if(strcmp(dir.files[i].fname, filename) == 0 && strlen(filename) == strlen(dir.files[i].fname)){
      if((extension[0] == '\0') && (dir.files[i].fext[0] == '\0')){
	break;
      } else if ((extension != NULL) && (strcmp(extension, dir.files[i].fext) == 0)){
	break;
      }
    }
    i++;
  }

  if(i < dir.nFiles){
    // found file
    fseek(fp, dir.files[i].nStartBlock, SEEK_SET);
    fread(file, sizeof(cs1550_disk_block), 1, fp);
    found = i; 
  }
  fclose(fp);

  // return offset of file in directory if found, -1 if not                                                                                                                                                 
  return found;
}

/*
 * Called whenever the system wants to know the file attributes, including
 * simply whether the file exists or not. 
 *
 * man -s 2 stat will show the fields of a stat structure
 */
static int cs1550_getattr(const char *path, struct stat *stbuf){
  printf("\nENTER: getattr()\n");
  int res = -ENOENT;
  
  memset(stbuf, 0, sizeof(struct stat));
  
  struct cs1550_directory_entry current_dir;
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  long dir_index;
 
  
  if (strcmp(path, "/") == 0) {
    stbuf->st_mode = S_IFDIR | 0755;
    stbuf->st_nlink = 2;
    res = 0;
  } else {    // verify sub directory
    memset(directory, 0, MAX_FILENAME + 1);
    memset(filename, 0, MAX_FILENAME + 1);
    memset(extension, 0, MAX_EXTENSION + 1);

    sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);
    dir_index = get_dir_index(directory);
    if(dir_index == -1){
      printf("Could not find directory. ");
      return -ENOENT;
      }
    if(dir_index >= 0){
      get_dir(&current_dir, dir_index);
      printf("\t\tGET_DIR() GOT Current_dir w/ %d files\n", current_dir.nFiles);
      if(directory != NULL && filename[0] == '\0'){
	stbuf->st_mode = S_IFDIR | 0755;                                                                                                                                                                
	stbuf->st_nlink = 2;                                                                                                                                                                            
	res = 0; //no error 
      } else {
	int i = 0;
	while(i < current_dir.nFiles){                                                                                                                                                                   
	  printf("\tSearching for files\n");                                                                                                                                                                 	  
	  if(strcmp(current_dir.files[i].fname, filename) == 0 && strcmp(current_dir.files[i].fext, extension) == 0){                                                                                    
	    printf("\t\tfname: %s [    ] filname: %s\n", current_dir.files[i].fname, filename);                                                                                                         
	    stbuf->st_mode = S_IFREG | 0666;                                                                                                                                                            
	    stbuf->st_nlink = 1; //file links                                                                                                                                                      
	    stbuf->st_size = current_dir.files[i].fsize;                                                                                                                                                
	    res = 0; // no error                                                                                                                                                                       
	    break;                                                                                                                                                                                      
	  }                                                                                                                                                                     
	  i++;                                                                                                                                            
	}                                    
      }

    } else {
      res = -ENOENT;
    }
  }
  
  return res;
}


/* 
 * Called whenever the contents of a directory are desired. Could be from an 'ls'
 * or could even be when a user hits TAB to do autocompletion
 */
static int cs1550_readdir(const char *path, void *buf, fuse_fill_dir_t filler,
			 off_t offset, struct fuse_file_info *fi)
{
  printf("\nENTER: readdir()\n");
  //Since we're building with -Wall (all warnings reported) we need
  //to "use" every parameter, so let's just cast them to void to
  //satisfy the compiler
  (void) offset;
  (void) fi;

  cs1550_root_directory  root;
  cs1550_directory_entry sub_dir;
  char directory[MAX_FILENAME +1];
  char filename[MAX_FILENAME +1];
  char extension[MAX_EXTENSION +1];

  memset(directory,  0,MAX_FILENAME + 1);
  memset(filename, 0,MAX_FILENAME + 1);
  memset(extension,0,MAX_EXTENSION + 1);

  sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);

  // grab root
  FILE *fp = fopen(".disk", "rb+");
  if(fread((void*) &root, sizeof(cs1550_root_directory), 1, fp) < 1){
    printf("Root Read Error");
    fclose(fp);
    return -1;
  }
  fclose(fp);
  if (strcmp(path, "/") == 0){
    printf("\tDirectory is ROOT.. list all sub directories\n");
    //the filler function allows us to add entries to the listing
    //read the fuse.h file for a description (in the ../include dir)
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);

    char directory_name[MAX_FILENAME + 1];
    int i = 0;
    while(i<MAX_DIRS_IN_ROOT){
      strcpy(directory_name, root.directories[i].dname);
      if(directory_name[0] != 0){ 
	filler(buf, directory_name, NULL, 0);
      }
      i++;
    }
  } else {
    printf("\tCurrent Level < %s > is Sub Directory\n", directory);
    /*
    //add the user stuff (subdirs or files)
    //the +1 skips the leading '/' on the filenames
    filler(buf, newpath + 1, NULL, 0);
    */
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    
    long dir_index = get_dir_index(directory);
    
    if(dir_index < 0){
      return -ENOENT;
    }
    
    if(get_dir(&sub_dir, dir_index) != 1){
      return -ENOENT;
    }

    char full_name[MAX_FILENAME + MAX_EXTENSION + 2];
    int j = 0;
    printf("\t\tFind all files in sub directory:%s\n", directory);
    while(j < sub_dir.nFiles){
      if(strlen(sub_dir.files[j].fext) > 0){
	strcpy(full_name, sub_dir.files[j].fname);
	strcat(full_name, ".");
	strcat(full_name, sub_dir.files[j].fext);
      } else {
	strcpy(full_name, sub_dir.files[j].fname);
      }
      filler(buf, full_name, NULL, 0);
      j++;
    }
  }
  return 0;
}

/* 
 * Creates a directory. We can ignore mode since we're not dealing with
 * permissions, as long as getattr returns appropriate ones for us.
 */

static int cs1550_mkdir(const char *path, mode_t mode)
{
  (void) path;
  (void) mode;

  cs1550_directory_entry make_dir;
  cs1550_root_directory root;
  cs1550_disk_block free_ptr;

  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];

  sscanf(path, "/%[^/]/%[^.].%s", directory, filename, extension);

  // if no directory specified, dir is not under root dir
  if(directory == NULL || directory[0] == '\0'){
    return -EPERM;
  }
  // if name beyond 8 MAX_FILENAME (8) chars
  if(strlen(directory) > MAX_FILENAME){
    return -ENAMETOOLONG;
  }
  
  // if directory already exists
  if(get_dir_index(directory) != -1){
    printf("%lu\n", get_dir_index(directory));
    return -EEXIST;
  }

  // grab root from disk
  FILE *fp = fopen(".disk", "rb+");
  if(fread((void*)&root, sizeof(cs1550_root_directory), 1, fp) < 1){
    printf("\nRoot Read Error");
    fclose(fp);
    return -1;
  }
  
  // check for max dirs in root
  if(root.nDirectories >= MAX_DIRS_IN_ROOT){
    printf("\nMemory Allocation Error: Max directories in Root\n");
    fclose(fp);
    return -EPERM;
  }

  fclose(fp);

  // iterate through root directory array to find first empty index
  long block;
  int i = 0;
  while(i<MAX_DIRS_IN_ROOT){
    if(root.directories[i].dname[0]==0){ // empty dir
      break;
    }
    i++;
  }

  // assign directory name, get and assign star block, 
  // increment num of directories in root, initialize new empty directory 
  strcpy(root.directories[i].dname,directory);  // copy new directory name to root index
  root.nDirectories++;                      // increment num of sub directories under root
  block = next_free_block();                // grab next free block
  if(block == -1){
    return -1;
  }
  root.directories[i].nStartBlock = block;  // set in roort as new dirs startBlock
  change_free(); // set NEW next free block as old + BLOCK_SIZE (512 bytes) 
  make_dir.nFiles = 0;                      // initialize num of sub files in enw directory to 0

  // write new directory to disk at correct index
  fp = fopen(".disk", "rb+");
  fwrite((void*)&root, sizeof(cs1550_root_directory), 1, fp);
  rewind(fp);
  fseek(fp, block, SEEK_SET);
  fwrite((void*)&make_dir, sizeof(cs1550_directory_entry), 1, fp);

  fclose(fp);

  int j;
  for(j = 0; j < root.nDirectories; j++){
    printf("\n\nDirectory Block: %ld\n", root.directories[j].nStartBlock);
  }
  printf("\nNext free block: %ld\n", next_free_block());

  

  return 0;
}

// removes a directory
static int cs1550_rmdir(const char *path)
{
	(void) path;
    return 0;
}

/* 
 * Does the actual creation of a file. Mode and dev can be ignored.
 
*
 */
static int cs1550_mknod(const char *path, mode_t mode, dev_t dev)
{
	(void) mode;
	(void) dev;

	cs1550_root_directory root;
	cs1550_directory_entry sub_dir;
	cs1550_disk_block free_ptr;
	cs1550_disk_block data;

	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];

	memset(directory, 0,MAX_FILENAME + 1);
	memset(filename, 0,MAX_FILENAME + 1);
	memset(extension,0,MAX_EXTENSION + 1);
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
	            
	printf("\tAttempting to create %s.%s in %s\n", filename, extension, directory);

	FILE *fp = fopen(".disk", "rb+");
	if(fread((void*)&root, sizeof(cs1550_root_directory), 1, fp) < 1){
	  printf("\t!!!Root Read Error\n");
	  fclose(fp);
	  return -1;
	}

	fclose(fp);

	if (directory == NULL){
	  printf("\t!!!Cannot create file in root!\n");
	  return -EPERM;
	}

	if(strlen(filename) > MAX_FILENAME || strlen(extension) > MAX_EXTENSION){
	  return -ENAMETOOLONG;
	}

	long dir_index = get_dir_index(directory);
	if(dir_index == -1){
	  printf("!!!\tInvalid path: %s is not a file or directory\n", directory);
	  return -ENOENT;
	}

	// retrieve directory entry at dir_index and fill sub_dir
	if(get_dir(&sub_dir, dir_index) == -1){
	  printf("!!!\tError retrieving directory\n");
	  return -1; 
	}

	// find first free index in files array of sub_dir
	int i = 0;
	while(i < sub_dir.nFiles){
	  if(strcmp(sub_dir.files[i].fname, filename) == 0 && strlen(filename) == strlen(sub_dir.files[i].fname)){
	    if((extension[0] == '\0') && (sub_dir.files[i].fext[0] == '\0')){
	      break;
	    } else if ((extension != NULL) && (strcmp(extension, sub_dir.files[i].fext) == 0)){
	      break;
	    }
	  }
	  i++;
	}

	if (i == sub_dir.nFiles){
	  strcpy(sub_dir.files[i].fname, filename);
	  if(strlen(extension) > 0) {
	    strcpy(sub_dir.files[i].fext, extension);
	  } else {
	    strcpy(sub_dir.files[i].fext, "\0");
	  }
	  sub_dir.files[i].fsize = 0;
	  sub_dir.nFiles = sub_dir.nFiles + 1;
	  printf("\tsub_dir fname: %s.%s\n", sub_dir.files[i].fname, sub_dir.files[i].fext);
	  long block = next_free_block();
	  if(block == -1){
	    return -1;
	  }
	  sub_dir.files[i].nStartBlock = block;
      	  change_free();
	  
	  // if more than 1 file in sub dirextory, link the data blocks
	  if(sub_dir.nFiles > 1){
	    if(link_blocks(sub_dir, i-1, block) == -1){
	      printf("\tError Linking data\n");
	      return -1;
	    }
	  }

	  fp = fopen(".disk", "rb+");  
	  fseek(fp, dir_index, SEEK_SET);
	  fwrite((void*)&sub_dir, sizeof(cs1550_directory_entry), 1, fp);
	  rewind(fp);
	  fseek(fp, sub_dir.files[i].nStartBlock, SEEK_SET);
	  fwrite((void*)&data, sizeof(cs1550_disk_block), 1, fp);
	  fclose(fp);
	  printf("\tlinked succesfully\n");
	} else {
	  return -EEXIST;
	}
       	
	return 0;
}

/*
 * Deletes a file
 */
static int cs1550_unlink(const char *path)
{
    (void) path;

    return 0;
}

/* 
 * Read size bytes from file into buf starting from offset
 *
 */
static int cs1550_read(const char *path, char *buf, size_t size, off_t offset,
			  struct fuse_file_info *fi)
{
	(void) fi;

	printf("ENTER: read()\n");

	cs1550_root_directory root;
	cs1550_directory_entry sub_dir;
	cs1550_disk_block file;
 
	char directory[MAX_FILENAME + 1];
	char filename[MAX_FILENAME + 1];
	char extension[MAX_EXTENSION + 1];
	size_t file_size;
	
	memset(directory, 0,MAX_FILENAME + 1);
	memset(filename, 0,MAX_FILENAME + 1);
	memset(extension,0,MAX_EXTENSION + 1);
	
	sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
	
	if((offset > size) || (size <= 0)){                                    
	  return -1;
	}
	
	if(directory == NULL) {
	  return -1;
	}
	if((filename != NULL) && (filename[0] != '\0') && (strlen(filename) < MAX_FILENAME+1)) {
	  if((extension != NULL) && (extension[0] != '\0') && (strlen(extension) >= MAX_EXTENSION+1)) {
	    return -ENAMETOOLONG;
	  }
	} else {
	  return -ENAMETOOLONG;
	}

	//grab root                                                                                                                                                                                              
	FILE *fp = fopen(".disk", "rb+");
	fread((void*)&root, sizeof(cs1550_root_directory), 1, fp);
	fclose(fp);
	
	long directory_offset = get_dir_index(directory);
	if(directory_offset == -1){
	  return -1; 
	}
	
	if(get_dir(&sub_dir, directory_offset) == -1){
	  return -1; 
	}
	
	int fileIndex = get_file(sub_dir, &file, filename, extension);
	if(fileIndex == -1){                                                                                                         
	  return -EEXIST;
	}

	long file_offset = sub_dir.files[fileIndex].nStartBlock; // start of data blocks                                                                                                                       
	file_size = sub_dir.files[fileIndex].fsize;      // initial size of file                                                                                                                         
	int index_to_start = offset/MAX_DATA_IN_BLOCK; // # of times to use block seek loop                                                                                                                        
	long byte = offset % MAX_DATA_IN_BLOCK;
	
	fp = fopen(".disk", "rb+");
	int j = 0;
	while(j < index_to_start){
	  printf("\tseek in file block %d of %d\n", j+1, index_to_start+1);
	  file_offset = file.nNextBlock;
	  if(file_offset <= 0){
	    printf("\t\tError Getting Offset\n: line 714");
	    return -1;
	  }
	  fseek(fp, file_offset, SEEK_SET);
	  if(fread((void*)&file, sizeof(cs1550_disk_block), 1, fp) != 1){
	    printf("\t\t\tError reading file\n");
	    return -1;
	  }
	}
	fclose(fp);
	//printf("\n\tRead: ");
	j = 0;
	int i = byte;
	while(j < file_size){
	  buf[j] = file.data[i];
	  //printf("%c", file.data[i]);
	  j++;
	  i++;
	  if(i == MAX_DATA_IN_BLOCK && j < file_size){
	    fp = fopen(".disk", "rb+");
	    i = 0;
	    file_offset = file.nNextBlock;
	    fseek(fp, file_offset, SEEK_SET);
	    fread((void*)&file, sizeof(cs1550_disk_block), 1, fp);
	    fclose(fp);
	  }
	}
	size = strlen(buf);
	//printf("\n");
	return file_size;
}

/* 
 * Write size bytes from buf into file starting from offset
 *
 */
static int cs1550_write(const char *path, const char *buf, size_t size, 
			off_t offset, struct fuse_file_info *fi)
{
  printf("ENTER: write()\n");
  //check to make sure path exists   [check]                                                                                       
  //check that size is > 0                                                                                                         
  //check that offset is <= to the file size                                                                                       
  //write data                                                                                                                     
  //set size 
  
  (void) fi;
  cs1550_root_directory root;
  cs1550_directory_entry sub_dir;
  cs1550_disk_block file;
  
  char directory[MAX_FILENAME + 1];
  char filename[MAX_FILENAME + 1];
  char extension[MAX_EXTENSION + 1];
  size_t file_size;
  
  memset(directory, 0,MAX_FILENAME + 1);
  memset(filename, 0,MAX_FILENAME + 1);
  memset(extension,0,MAX_EXTENSION + 1);
  
  sscanf(path,"/%[^/]/%[^.].%s",directory,filename,extension);
               
  if(size <= 0){ 
    return -1;
  }
  
  if(directory == NULL) {
    return -1;
  }
  
  if((filename != NULL) && (filename[0] != '\0') && (strlen(filename) < MAX_FILENAME + 1)) {
    if((extension != NULL) && (extension[0] != '\0') && (strlen(extension) >= MAX_EXTENSION+1)) {
      return -ENAMETOOLONG;
    }
  } else {
    return -ENAMETOOLONG;
  }
  
  //grab root                                                                                                                                                               
  FILE *fp = fopen(".disk", "rb+");                                                                                                                                          
  fread((void*)&root, sizeof(cs1550_root_directory), 1, fp);                                                                                                                
  fclose(fp);           
  
  // if directory does not exist 
  long directory_offset = get_dir_index(directory);
  if(directory_offset == -1){
    return -ENOENT; 
  } 

  if(get_dir(&sub_dir, directory_offset) == 0) {
    return -EISDIR; 
  }

  int fileIndex = get_file(sub_dir, &file, filename, extension);
  if(fileIndex == -1){ 
    return -EEXIST;
  }
  
  long file_offset = sub_dir.files[fileIndex].nStartBlock; // start of data blocks
  file_size = sub_dir.files[fileIndex].fsize;      // initial size of file
  sub_dir.files[fileIndex].fsize += size;               // go ahead and update file size to the size to include data to be written
  if(offset > file_size){
    return -EFBIG;
  }

  // Write the updated directory to the .disk file before we do anything
  fp = fopen(".disk", "rb+");
  fseek(fp, directory_offset, SEEK_SET);
  fwrite((void*)&sub_dir, sizeof(cs1550_directory_entry), 1, fp);
  fclose(fp);
  
  int index_to_start = offset/MAX_DATA_IN_BLOCK; // # of times to use block seek loop                                                                                                              
  long byte = offset % MAX_DATA_IN_BLOCK;                      
  
  fp = fopen(".disk", "rb+");                                                                                                                                                                     
  int j = 0;                                                                                                                                                                                  
  while(j < index_to_start){                                                                                                                                                             
    //printf("\tseek in file block %d of %d\n", j+1, index_to_start+1);                                                                                                                     
    file_offset = file.nNextBlock;                                                                                                                                                   
    if(file_offset <= 0){                                                                                                                                                 
      printf("\t\tError Getting Offset\n: line 714");                                                                                                  
      return -1;                                                                                                                               
    }                                                                                                                                                                                         
    fseek(fp, file_offset, SEEK_SET);                                                                                                                                                        
    if(fread((void*)&file, sizeof(cs1550_disk_block), 1, fp) != 1){                                                                                                                               
      printf("\t\t\tError reading file\n");                                                                                                                                                    
      return -1;                                                 
    }
  }
  fclose(fp);
  
  j = 0;
  int i = byte;
  //printf("\n\tWrote: ");
  while(j < size){
    file.data[i] = buf[j];
    //printf("%c", file.data[i]); 
    j++;
    i++;
    if(i == MAX_DATA_IN_BLOCK && j < size){
      long freeBlock = next_free_block();
      if(freeBlock == -1){
	return -1;
      }
      file.nNextBlock = freeBlock;
      fp = fopen(".disk", "rb+");
      i = 0;
      fseek(fp, file_offset, SEEK_SET);
      file_offset = file.nNextBlock;
      fwrite((void*)&file, sizeof(cs1550_disk_block), 1, fp);
      fseek(fp, file_offset, SEEK_SET);
      fread((void*)&file, sizeof(cs1550_disk_block), 1, fp);
      fclose(fp);
      change_free();
    } else if(j == size) {
      fp = fopen(".disk", "rb+");
      fseek(fp, file_offset, SEEK_SET);
      fwrite((void*)&file, sizeof(cs1550_disk_block), 1, fp);
      fclose(fp);
      
    }
  }
  //size = strlen(buf);
  printf("\n");
  
  //printf("\n\n\t\t\WROTE SUCCESFULLY\n");
  //check to make sure path exists   [check]
  //check that size is > 0
  //check that offset is <= to the file size
  //write data
  //set size (should be same as input) and return, or error
  
  return size;
}


/******************************************************************************
 *
 *  DO NOT MODIFY ANYTHING BELOW THIS LINE
 *
 *****************************************************************************/

/*
 * truncate is called when a new file is created (with a 0 size) or when an
 * existing file is made shorter. We're not handling deleting files or 
 * truncating existing ones, so all we need to do here is to initialize
 * the appropriate directory entry.
 *
 */
static int cs1550_truncate(const char *path, off_t size)
{
	(void) path;
	(void) size;

    return 0;
}


/* 
 * Called when we open a file
 *
 */
static int cs1550_open(const char *path, struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;
    /*
        //if we can't find the desired file, return an error
        return -ENOENT;
    */

    //It's not really necessary for this project to anything in open

    /* We're not going to worry about permissions for this project, but 
	   if we were and we don't have them to the file we should return an error

        return -EACCES;
    */

    return 0; //success!
}

/*
 * Called when close is called on a file descriptor, but because it might
 * have been dup'ed, this isn't a guarantee we won't ever need the file 
 * again. For us, return success simply to avoid the unimplemented error
 * in the debug log.
 */
static int cs1550_flush (const char *path , struct fuse_file_info *fi)
{
	(void) path;
	(void) fi;

	return 0; //success!
}


//register our new functions as the implementations of the syscalls
static struct fuse_operations hello_oper = {
    .getattr	= cs1550_getattr,
    .readdir	= cs1550_readdir,

    .mkdir	= cs1550_mkdir,
	.rmdir = cs1550_rmdir,
    .read	= cs1550_read,
    .write	= cs1550_write,
	.mknod	= cs1550_mknod,
	.unlink = cs1550_unlink,
	.truncate = cs1550_truncate,
	.flush = cs1550_flush,
	.open	= cs1550_open,
};

//Don't change this.
int main(int argc, char *argv[])
{
	return fuse_main(argc, argv, &hello_oper, NULL);
}
