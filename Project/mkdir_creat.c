/************* mkdir_creat.c file **************/

// Function declarations for mkdir & creat implementation ***
int tst_bit(char *buf, int bit); // in Chapter 11.3.1
int set_bit(char *buf, int bit);// in Chapter 11.3.1
int my_mkdir(MINODE* mip, char* _basename);
int init_dir(int dblk, int pino, int ino);
// **********************************************************

extern int put_block(int dev, int blk, char *buf);
extern int tokenize(char *pathname);
extern int getino(char *pathname);
extern MINODE *iget(int dev, int ino);
extern int search(MINODE *mip, char *name);
extern MINODE *iget(int dev, int ino);
extern int enter_name(MINODE* pip, int ino, char* name);
int create_inode(INODE* ip, int bno);




extern int n;
extern char *name[64];
extern int nblocks, ninodes, bmap, imap;


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
  }else if(strcmp(mode_bits + 12, "0010") == 0){ // 0100 -> is a DIR,  OKAY
    //printf("Pathname is a directory\n");
  }else{
      printf("[ERROR: Issue reading file type]");
      iput(mip); //derefrence useless minode to ref file
      return -1;

  }
  //************************************************************

  return 0;
}


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