/*********** symlink.c ***************/

// Function declarations ****************************************
int my_symlink(MINODE* pmip, char* old_name, char* _basename);
int is_link(MINODE* mip);
int readlink(char* pathname, char name[]);
// **************************************************************

extern int getino(char *pathname);
extern int my_creat(MINODE* pmip, char* _basename);
extern int balloc(int dev); //same as ialloc but for the block bitmap

int symlink_pathname(char* pathname){

    int oino, pino;
    char second_pathname[128], new_basename[128], temp[128], dirname[128], old_basename[60];

    MINODE* omip, *pmip;

    sscanf(pathname, "%s %s", temp, second_pathname);  //seperate the two pathnames
    strcpy(pathname, temp);

    printf("pathname1: %s\npathname2: %s\n", pathname, second_pathname);

    oino = getino(pathname); //get inode number for old file

    if(!oino){ //check old file exsits
        printf("symlink_pathname : %s is not a valid path\nsymlink unsuccessful\n", pathname);
        return -1;
    }
    
    omip = iget(dev, oino); //get the inode from old file

    tokenize(pathname);
    strcpy(old_basename, name[n - 1]); //get the old file nme for later


    if(getino(second_pathname)){ //if second pathname has an inode, that means new file already exists
        printf("symlink_pathname: %s already exists\nsymlink unsuccessful\n", second_pathname);
        return -1;

    }
    //tokenize second pathname
    tokenize(second_pathname);
    
    //determine to start at root or cwd
    if(second_pathname[0] == '/'){
            strcpy(dirname, "/"); //dirname is root
        }else{  //dirname is cwd
            strcpy(dirname, "");
    }

    //build the dirname
    for(int i = 0; i < (n-1); i++){
        strcat(dirname, name[i]);

        if((i+1) < (n-1)){
            strcat(dirname, "/");
        }
    }
    

    strcpy(new_basename, name[n - 1]); //get the _basename from path

    printf("dirname: %s\nbasename: %s\n", dirname, new_basename);

    if(strlen(dirname)){ //if a dirname was given
        pino = getino(dirname); //get the inode number for the parent directory

        if(!pino){ //dirname does not exist
            printf("symlink unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory
    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }

    
    // AT THIS POINT IN THE CODE: parent directory found, old file inode found, and new file name identified
    // all potential errors accounted for: safe to run my symlink
    return my_symlink(pmip, old_basename, new_basename); //link file

}


int my_symlink(MINODE* pmip, char* old_name, char* _basename){
    if(my_creat(pmip, _basename)){
        return -1;
    }

    MINODE* mip;
    int ino, blk;
    char buf[BLKSIZE], name[60];

    ino = getino(_basename); //get the inode number for the new created file
    mip = iget(dev, ino); // set mip to new created file (get its inode)

    mip->INODE.i_mode = 0xA1FF; //lnk mode

    blk = balloc(dev); // get the newly allocated block number

    if(!blk){ //if new data block could not be assigned
        printf("my_symlink : WARNING, blk could not be allocated\n");
        return -1;
    }

    mip->INODE.i_block[0] = blk; //assign new datablock
    
    get_block(dev, blk, buf); // go to that data block
    memcpy(buf, old_name, strlen(old_name)); //put old name into data block
    put_block(dev, blk, buf); // write data block back to disk

    mip->INODE.i_size = strlen(old_name); //file size is the length of the linked file name
    
    mip->dirty = 1; //mark as dirty
    iput(mip); //write inode back to disk

    pmip->dirty = 1; // pmip is dirty
    iput(pmip); //write parent inode back to disk

    return 0;

}

int call_readlink(char* pathname){

    char name[60];
    int size;

    size = readlink(pathname, name);

    if(size == -1){
        return -1;
    }
    printf("\n---- READLINK RESULTS ----\nFilename: %s\nSize: %d\n\n", name, size);


    return 0;

}
int readlink(char* pathname, char name[]){

    int ino, file_size;
    MINODE* mip;
    char buf[BLKSIZE];

    ino = getino(pathname); //get inode number of link
    mip = iget(dev, ino); //put inode in mip

    if(is_link(mip) != 0){ //file is not a link
        printf("readlink : %s is not a lnk type\nreadlink unsucessful\n", pathname);
        return -1;
    }

    get_block(dev, mip->INODE.i_block[0], buf); //get data block that has file name

    strncpy(name, buf, mip->INODE.i_size); //since size is the name str len, read that many chars from data block
    name[mip->INODE.i_size] = 0;

    file_size = mip->INODE.i_size;

    iput(mip);

    return file_size;
    
}


/*****************************************************
*
*  Name:    is_link
*  Made by: Reagan
*  Details: Returns 0 if mip is a link
*
*****************************************************/
int is_link(MINODE* mip){
  // CHECK IF mip IS A LINK ***************************
  INODE f_inode;
  f_inode = mip->INODE;
  u16 mode = f_inode.i_mode;                    // 16 bits mode
  char mode_bits[16];
  mode_bits[16] = '\0';

  // this loop stores the u16 mode as binary (as a string) - but in reverse order
  for(int i = 0; i < 16; i++){
    int bit;
    bit = (mode >> i) & 1;
    //printf("%d\n", bit);
    if(bit == 1){
      mode_bits[i] = '1';
    }else{
      mode_bits[i] = '0';
    }
  }
  
  if(strcmp(mode_bits + 12, "0001") == 0){ // 1000 -> is a REG file, 
    //printf("ls: Can't ls a non-directory.\n");
    iput(mip); //derefrence useless minode to ref file
    return -1;
  }else if(strcmp(mode_bits + 12, "0010") == 0){ // 0100 -> is a DIR,  
    //printf("ls: Can't ls a non-directory.\n");
    iput(mip); //derefrence useless minode to ref file
    return -1;
  }
  //************************************************************

  return 0;
}