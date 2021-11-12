/************* mkdir_creat.c file **************/

// Function declarations for mkdir & creat implementation ***
int tst_bit(char *buf, int bit); // in Chapter 11.3.1
int set_bit(char *buf, int bit);// in Chapter 11.3.1
int my_mkdir(MINODE* mip, char* _basename);
int init_dir(int dblk, int pino, int ino);
int enter_name(MINODE* pip, int ino, char* name);
int my_creat(MINODE* pmip, char* _basename);
int create_inode(INODE* ip, int bno); //from book pg 334
// **********************************************************

extern int put_block(int dev, int blk, char *buf);
extern int tokenize(char *pathname);
extern int getino(char *pathname);
extern MINODE *iget(int dev, int ino);
extern int search(MINODE *mip, char *name);
extern MINODE *iget(int dev, int ino);




extern int n;
extern char *name[64];
extern int nblocks, ninodes, bmap, imap;

/*****************************************************
*
*  Name:    is_dir
*  Made by: Reagan
*  Details: Returns 0 if mip is a directory
*
*****************************************************/
int is_dir(MINODE* mip){
  // CHECK IF mip IS A DIRECTORY ***************************
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
  
  if(strcmp(mode_bits + 12, "0001") == 0){ // 1000 -> is a REG file,  CANT CD TO REF FILE
    //printf("ls: Can't ls a non-directory.\n");
    iput(mip); //derefrence useless minode to ref file
    return -1;
  }else if(strcmp(mode_bits + 12, "0101") == 0){ // 0100 -> is a DIR,  OKAY
    //printf("ls: Can't ls a non-directory.\n");
    iput(mip); //derefrence useless minode to ref file
    return -1;
  }
  //************************************************************

  return 0;
}

/*****************************************************
*
*  Name:    ialloc
*  Made by: KC
*  Details: Goes the the inode bitmap block and 
*           allocates a new inode for use
*
*****************************************************/
int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){ //returns the bit value (if there is no bit there)
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        return i+1;
    }
  }
  return 0;
}

/*****************************************************
*
*  Name:    balloc
*  Made by: Reagan
*  Details: Goes the the block bitmap block and allocates 
*           a new block in the disk for use.
*
*****************************************************/
int balloc(int dev){ //same as ialloc but for the block bitmap

  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, bmap, buf);

  for (i=0; i < nblocks; i++){
    if (tst_bit(buf, i)==0){ //returns the bit value (if there is no bit there)
        set_bit(buf, i);
        put_block(dev, bmap, buf);
        printf("allocated block = %d\n", i+1); // bits count from 0; ino from 1
        return i+1;
    }
  }

    return 0;
}

/*****************************************************
*
*  Name:    test bit
*  Made by: Reagan
*  Details: Returns the bit at that point in the block 
*           (1 or 0)
*
*****************************************************/
int tst_bit(char *buf, int bit){ // in Chapter 11.3.1

    int i,j;

    i = bit / 8; // what n byte in buf this bit belongs to (8 bits in a byte)
    j = bit % 8; // which nth bit this bit belongs to in the i'th byte.

    /* example of return

        i = 10, j = 3
        buf[10] & (1 << 3)
        0001 1100 & 0000 0001 << 3
        0001 1100 & 0000 1000
        0000 1000
        8
        8 != 0; returns 1
        ** Thus can only be 0 if there is not a 1 bit at byte i, bit j
    */
    if(buf[i] & (1 << j) != 0){
        return 1;
    }
    return 0;
}

/*****************************************************
*
*  Name:    set bit
*  Made by: Reagan
*  Details: Sets the bit at that location in the 
*           block to a 1
*
*****************************************************/
int set_bit(char *buf, int bit){ // in Chapter 11.3.1

    int i,j;

    i = bit / 8; // what n byte in buf this bit belongs to (8 bits in a byte)
    j = bit % 8; // which nth bit this bit belongs to in the i'th byte.


    /* example of return

        i = 10, j = 3
        buf[10] |= (1 << 3)
        0001 0100 |= 0000 0001 << 3
        0001 0100 |= 0000 1000 (sets bit to 1 if it exists in either operand)
        0001 1100
        
        Places a 1 bit in the 3rd bit of this byte (before: 0001 0100, after: 0001 1100)
    */

    buf[i] |= (1 << j); // set byte i's, j bit to 1

    return 0;
}

/*****************************************************
*
*  Name:    creat_pathname
*  Made by: Reagan Kelley
*  Details: Takes a given pathname, and makes sure 
*           that it can be used to creat a new file. 
*           If all checks are made, my_creat is called.
* 
*****************************************************/
int creat_pathname(char* pathname){

    //divide pathname into dirname and basename
    int from_cwd, pino;
    char dirname[128], _basename[128], temp[255];
    MINODE* pmip;
    

    if(getino(pathname) > 0){ //if given pathname already exists
        printf("creat_pathname : %s already exists\ncreat unsuccessful\n", pathname);
        return -1;
    }

    if(getino(pathname) == -1){ //if second pathname is invalid
        printf("creat_pathname: %s is not a valid pathname\ncreat unsuccessful\n", pathname);
        return -1;

    }

    if(strlen(pathname) == 0 || (strlen(pathname) == 1 && pathname[0] == '/')){ //if no pathname given or pathname is '/'

        return -1;
    }
    tokenize(pathname);
    
    //determine to start at root or cwd
    if(pathname[0] == '/'){
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
    

    strcpy(_basename, name[n - 1]); //get the _basename from path

    printf("dirname: %s\nbasename: %s\n", dirname, _basename);

    if(strlen(dirname)){ //if a dirname was given
        pino = getino(dirname); //get the inode number for the parent directory

        if(!pino){ //dirname does not exist
            printf("creat unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory
    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }

    if(strlen(dirname) && is_dir(pmip) != 0){ //pmip is not a directory (only check if not cwd)
        printf("dirname is not a directory\ncreat unsuccessful\n");
        return -1;
    }
    
    //all checks made, safe to creat


    return my_creat(pmip, _basename);
}

/*****************************************************
*
*  Name:    my_creat
*  Made by: Reagan Kelley
*  Details: Takes a parent directory and creates a 
*           new file with the given basename, puts
*           in the parent directory
* 
*****************************************************/
int my_creat(MINODE* pmip, char* _basename){

    int ino;
    MINODE* mip;
    INODE inode;

    ino = ialloc(dev); // get the newly allocated inode number
    mip = iget(dev, ino); //get the new inode via minode

    //initialize inode *************
    create_inode(&mip->INODE, 0); // 0 because no block is allocated (also when 0 is given, create_inode intuitively makes it a reg file type)
    mip->dirty = 1; //mark inode dirty
    iput(mip); //write inode to disk
    //*****************************

    enter_name(pmip, ino, _basename); // add new reg file to parent directory data block

    // NOTE: We don't increment parent link count

    return 0;
}

/*****************************************************
*
*  Name:    mkdir pathname
*  Made by: Reagan
*  Details: Determines if mkdir works for the given 
*           pathname. If there is no repeated basenames
*           and the dirname is valid call my_mkdir
*
*****************************************************/
int mkdir_pathname(char* pathname){

    //divide pathname into dirname and basename
    int from_cwd, pino;
    char dirname[128], _basename[128], temp[255];
    MINODE* pmip;
    
    if(strlen(pathname) == 0 || (strlen(pathname) == 1 && pathname[0] == '/')){ //if no pathname given or pathname is '/'

        return -1;
    }
    tokenize(pathname);
    
    //determine to start at root or cwd
    if(pathname[0] == '/'){
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
    

    strcpy(_basename, name[n - 1]); //get the _basename from path

    printf("dirname: %s\nbasename: %s\n", dirname, _basename);

    if(strlen(dirname)){ //if a dirname was given
        pino = getino(dirname); //get the inode number for the parent directory

        if(!pino){ //dirname does not exist
            printf("mkdir unsuccessful\n");
            return -1;
        }

        //dirname is legit, now check if it is a directory
        pmip = iget(dev, pino); //create new minode for parent directory
    }else{ //if in cwd
        pmip = iget(dev, running->cwd->ino); //pmip becomes the cwd inode
    }
    if(strlen(dirname) && is_dir(pmip) != 0){ //pmip is not a directory (only check if not cwd)
        printf("dirname is not a directory\nmkdir unsuccessful\n");
        return -1;
    }



    if(search(pmip, _basename)){ //if not 0 -> basename exists and can't mkdir
        printf("basename already exists\nmkdir unsuccessful\n");
        return -1;
    }

    //all checks made, safe to mkdir
    
    printf("mkdir dirname and basename safe, preparing mkdir ...\n");
    return my_mkdir(pmip, _basename); 
}

/*****************************************************
*
*  Name:    my mkdir
*  Made by: Reagan
*  Details: The main mkdir function. Runs the mkdir 
*           command in the parent directory (pmip), 
*           initialzes the inodes and sets the dir 
*           entries
*
*****************************************************/
int my_mkdir(MINODE* pmip, char* _basename){
    int ino, blk;
    MINODE* mip;
    INODE inode;

    ino = ialloc(dev); // get the newly allocated inode number
    blk = balloc(dev); // get the newly allocated block number

    mip = iget(dev, ino); //get the new inode via minode
    
    //initialize inode *************
    create_inode(&mip->INODE, blk);
    mip->dirty = 1; //mark inode dirty
    iput(mip); //write inode to disk
    //*****************************

    init_dir(mip->INODE.i_block[0], pmip->ino, ino); // initialize dir entries in data block
    enter_name(pmip, ino, _basename); // add new directory to parent directory data block

    //increment parent indoes link count by 1 and mark pmip dirty
    pmip->dirty = 1;
    pmip->INODE.i_links_count++;
    iput(pmip); //write pmip inode back to disk


    return 0;
}

/*****************************************************
*
*  Name:    init dir
*  Made by: Reagan
*  Details: Sets the basic dir entries for a new 
*           directory, that being the . and .. directory 
*           paths
*
*****************************************************/
int init_dir(int dblk, int pino, int ino){ //based on pg 332

    char buf[BLKSIZE];
    DIR *dp;
    char *cp;

    get_block(dev, dblk, buf); //goes to the data block that holds new directory info
    dp = (DIR *)buf;  //buf can be read as ext2_dir_entry_2 entries
    cp = buf;         //will always point to the start of dir_entry


    // make . entry
    dp->inode = ino;
    dp->rec_len = 12; // ideal length = 4 * [ (8 * name_length + 3) / 4]
    dp->name_len = 1;
    dp->name[0] = '.';
    // make .. entry: pino=parent DIR ino, blk=allocated block
    
    cp += dp->rec_len; //rec_len is entry length in bytes. So, cp will now point to the byte after the end of the file.
    dp = (DIR *)cp;    //dp will now start at cp (the next entry)

    // make .. entry
    dp->inode = pino;
    dp->rec_len = BLKSIZE-12; // rec_len spans block
    dp->name_len = 2;
    dp->name[0] = dp->name[1] = '.';
    put_block(dev, dblk, buf); // write to blk on diks

}

/*****************************************************
*
*  Name:    enter name
*  Made by: Reagan Kelley
*  Details: Adds the new directory to the parent 
*           directory's data block. Trims the last dir 
*           entry, and either adds the new dir entry to 
*           the last data block or to a new one if there 
*           is no more room in the current data block
*
*****************************************************/
int enter_name(MINODE* pip, int ino, char* name){

   INODE inode;
   inode = pip->INODE;
   DIR *dp;
   char *cp;
   
   char buf[BLKSIZE];
   int last_used_iblock = -1;
   int ideal_length, need_length, remain;


   //get the last used iblock
   for(int i = 0; i < 12; i++){ // 12 direct blocks
      if(inode.i_block[i] == 0){ 
         last_used_iblock = i - 1;
         break;
      }
   }

   if(last_used_iblock == -1){ //this happens if i_block[0] is not initalied or all 12 blocks are used
      printf("enter_name : i_block could not be identifed");
      return -1;
   }

   //get the data block
   get_block(dev, inode.i_block[last_used_iblock], buf);

   dp = (DIR*)buf;
   cp = buf;

   while(cp + dp->rec_len < buf + BLKSIZE){ //traverse data block until at last entry
      cp += dp->rec_len;
      dp = (DIR*)cp;
   }

   ideal_length = 4 * ( (8 + dp->name_len + 3) / 4); //last dirs ideal length
   remain = dp->rec_len - ideal_length; //what will remain of the data block after last dir entry is trimmed
   need_length = 4 * ( (8 + strlen(name) + 3) / 4); // how much is needed for the newn entry

   if(remain >= need_length){ //there is room in the data block for the new dir entry
      dp->rec_len = ideal_length; //trim the last dir entry
      
      cp += dp->rec_len;
      dp = (DIR*)cp;

      dp->inode = ino;
      dp->rec_len = (buf + BLKSIZE) - cp; //goes to the end of the block
      dp->name_len = strlen(name);
      strcpy(dp->name, name); //there is a null character at the end of this but it does not matter since it wont overwrite important data

      put_block(dev, inode.i_block[last_used_iblock], buf);

      return 0;
   }

   // new dir entry won't fit in the last data block
   int blk;
   
   if(last_used_iblock + 1 >= 12){ //all 12 direct blocks used
      printf("enter name : no more space for direct iblocks (reached 12)\n");
      return -1;
   }

   blk = balloc(dev); // get the newly allocated block number
   pip->INODE.i_block[last_used_iblock + 1] = blk;
   pip->dirty = 1;

   //get the new data block
   get_block(dev, blk, buf); // NOTE: no need to write back earlier buf because no changes were made

   dp = (DIR*)buf;

   dp->inode = ino;
   dp->rec_len = BLKSIZE; //goes to the end of the block
   dp->name_len = strlen(name);
   strcpy(dp->name, name); //there is a null character at the end of this but it does not matter since it wont overwrite important data

   put_block(dev, blk, buf);

   return 0;

}

/*****************************************************
*
*  Name:    create inode
*  Made by: Reagan
*  Details: initilizes inode ip as a directory, bno the 
*           is the first data black in i_blocks. It is 
*           allocated before-hand
*
*****************************************************/
int create_inode(INODE* ip, int bno){ //from book pg 334
  
   if(!bno){ //if 0 must be reg type
        ip->i_mode = 0x81A4; // 040755: REG type and permissions
   }else{ // if not 0 must be dir type
        ip->i_mode = 0x41ED; // 040755: DIR type and permissions
   }

   ip->i_uid = running->uid; // owner uid
   ip->i_gid = running->gid; // group Id

   if(!bno){ //if 0 must be reg type
        
        ip->i_size = 0; // size in bytes for reg file

   }else{ // if not 0 must be dir type
        ip->i_size = BLKSIZE; // size in bytes for dir
   }

   if(!bno){ // if 0 must be reg type

        ip->i_links_count = 1; // links count=1 because of reg file
   }else{ // if not 0 must be dir type
        ip->i_links_count = 2; // links count=2 because of . and ..
   }
   
   ip->i_atime = ip->i_ctime = ip->i_mtime = time(0L);
   ip->i_blocks = 2; // LINUX: Blocks count in 512-byte chunks
   ip->i_block[0] = bno; // new DIR has one data block
   
   for(int i = 1; i < 14; i++){
        ip->i_block[i] = 0;
   }

   return 0;
}
