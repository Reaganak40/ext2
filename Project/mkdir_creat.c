/************* mkdir_creat.c file **************/

// Function declarations for mkdir & creat implementation ***
int tst_bit(char *buf, int bit); // in Chapter 11.3.1
int set_bit(char *buf, int bit);// in Chapter 11.3.1
// **********************************************************

extern int put_block(int dev, int blk, char *buf);

int ialloc(int dev)  // allocate an inode number from inode_bitmap
{
  int  i;
  char buf[BLKSIZE];

  // read inode_bitmap block
  get_block(dev, imap, buf);

  for (i=0; i < ninodes; i++){
    if (tst_bit(buf, i)==0){
        set_bit(buf, i);
        put_block(dev, imap, buf);
        printf("allocated ino = %d\n", i+1); // bits count from 0; ino from 1
        return i+1;
    }
  }
  return 0;
}

int tst_bit(char *buf, int bit){ // in Chapter 11.3.1

    return 0;
}

int set_bit(char *buf, int bit){ // in Chapter 11.3.1
    return 0;
}