// ******************* rmdir.c *************************

// Function Declarations ***********
int clr_bit(char* buf, int bit);
int rm_child(MINODE* pmip, char* name);
// *********************************

extern int getino(char *pathname);
extern MINODE *iget(int dev, int ino);
extern int is_dir(MINODE* mip);
extern int findino(MINODE *mip, u32 *myino);
extern int findmyname(MINODE *parent, u32 myino, char myname[ ]);


extern int n;
extern char *name[64];

//Page 338: int idalloc(dev, ino) deallocate an inode (number)
int idalloc(int dev, int ino)
{
  int i;  
  char buf[BLKSIZE];

  if (ino > ninodes){  
    printf("inumber %d out of range\n", ino);
    return -1;
  }
  
  get_block(dev, imap, buf);  // get inode bitmap block into buf[]
  
  clr_bit(buf, ino-1);        // clear bit ino-1 to 0

  put_block(dev, imap, buf);  // write buf back

  return 0;
}

int bdalloc(int dev, int blk){

    return 0;
}


int clr_bit(char* buf, int bit){

}

int rmdir_pathname(char* pathname){
    int ino, pino;
    MINODE* mip, *pmip;

    char buf[BLKSIZE], fname[64];
    DIR *dp;
    char *cp;
    
    //get the in-memory inode of pathname
    ino = getino(pathname); //inode number of pathname
    mip = iget(dev, ino);   // put inode in a new minode

    if(is_dir(mip) != 0){ //if mip is not a directory
        printf("rmdir pathname : %s is not a directory\n", name[n-1]);
        printf("rmdir unsuccessful\n");
        return -1;
    } 

    get_block(dev, mip->INODE.i_block[0], buf); //goes to the data block that holds the first few entries
    dp = (DIR *)buf;  //buf can be read as ext2_dir_entry_2 entries
    cp = buf;         //will always point to the start of dir_entry

    cp += dp->rec_len; // read past . directory
    dp = (DIR *)cp;    //dp will now start at cp (the .. directory)

    if(dp->rec_len != BLKSIZE-12){ //if the .. directory does not span the data block, that means there are other dir entries
        printf("rmdir pathname : %s is not empty\n", name[n-1]);
        printf("rmdir unsuccessful\n");
        return -1;

    }

    // dir is verified to be empty at this point

    pino = findino(mip, &ino); //find parents inode number
    pmip = iget(dev, pino);    //pmip now carries parent inode

    findmyname(pmip, ino, fname); // get name of dir to remove
    rm_child(pmip, name);         // remove child from parent directory

    //decrement parent indoes link count by 1 and mark pmip dirty
    pmip->dirty = 1;
    pmip->INODE.i_links_count--;
    iput(pmip); //write pmip inode back to disk

    bdalloc(dev, mip->INODE.i_block[0]); // deallocate the data block used by removed directory
    idalloc(dev, ino);                   // deallocate the inode used by removed directory

    return 0;


}

int rm_child(MINODE* pmip, char* name){

}