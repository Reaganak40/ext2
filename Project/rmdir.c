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
/*****************************************************
*
*  Name:    idalloc
*  Made by: Reagan Kelley
*  Details: Deallocates the inode at inode number (ino)
*
*****************************************************/
int idalloc(int dev, int ino)
{
  int i, table_loc;  
  char buf[BLKSIZE];

  for(int g = 0; g < NOFT; g++){ //use table to refer to device info
      if(mountTable[g].dev == dev){
          table_loc = g;
          break;
      }
  }

  if (ino > mountTable[table_loc].ninodes){  
    printf("inumber %d out of range\n", ino);
    return -1;
  }
  
  get_block(dev, mountTable[table_loc].imap, buf);  // get inode bitmap block into buf[]
  
  clr_bit(buf, ino-1);        // clear bit ino-1 to 0

  put_block(dev, mountTable[table_loc].imap, buf);  // write buf back

  return 0;
}

/*****************************************************
*
*  Name:    bdalloc
*  Made by: Reagan Kelley
*  Details: Deallocates the block indicated by blk
*
*****************************************************/
int bdalloc(int dev, int blk){

    int i, table_loc;
    char buf[BLKSIZE];


    for(int g = 0; g < NOFT; g++){ //use table to refer to device info
        if(mountTable[g].dev == dev){
            table_loc = g;
            break;
        }
    }


    if (blk > mountTable[table_loc].nblocks){  
      printf("inumber %d out of range\n", blk);
      return -1;
    }
  
    get_block(dev, mountTable[table_loc].bmap, buf);  // get block bitmap block into buf[]
  
    clr_bit(buf, blk);        // clear block bit

    put_block(dev, mountTable[table_loc].imap, buf);  // write buf back

    return 0;
}

/*****************************************************
*
*  Name:    clr_bit
*  Made by: Reagan Kelley
*  Details: Uses bitwise functions to take the bit 
*           (location in buf inidicated by int bit) 
*           to change it to 0.
*
*****************************************************/
int clr_bit(char* buf, int bit){
  int i,j;

  i = bit / 8; // what n byte in buf this bit belongs to (8 bits in a byte)
  j = bit % 8; // which nth bit this bit belongs to in the i'th byte.

  buf[i] &= ~(1 << j);

  return 0;
}

/*****************************************************
*
*  Name:    rmdir_pathname
*  Made by: Reagan Kelley
*  Details: Checks if pathname leads an existing dir, 
*           and removes if from disk, but only if its 
*           empty
*
*****************************************************/
int rmdir_pathname(char* pathname){
    int odev = dev; // keep track of original dev
    int ino, pino;
    MINODE* mip, *pmip;

    char buf[BLKSIZE], fname[64];
    DIR *dp;
    char *cp;
    
    //get the in-memory inode of pathname
    ino = getino(pathname); //inode number of pathname

    if(!ino){ //if pathname does not lead anywhere
      printf("rmdir unsuccessful\n");
      dev = odev; //reset back to orignal dev
      return -1;
    }

    mip = iget(dev, ino);   // put inode in a new minode

    if(is_dir(mip) != 0){ //if mip is not a directory
        printf("rmdir pathname : %s is not a directory\n", name[n-1]);
        printf("rmdir unsuccessful\n");
        dev = odev; //reset back to orignal dev

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
        dev = odev; //reset back to orignal dev

        return -1;

    }

    // dir is verified to be empty at this point

    pino = findino(mip, &ino); //find parents inode number
    pmip = iget(dev, pino);    //pmip now carries parent inode

    findmyname(pmip, ino, fname); // get name of dir to remove
    if(rm_child(pmip, fname) != 0){ // remove child from parent directory, 0 means successful
      printf("rmdir unsuccessful\n");
      dev = odev; //reset back to orignal dev
      return -1;
    }         

    //decrement parent indoes link count by 1 and mark pmip dirty
    pmip->dirty = 1;
    pmip->INODE.i_links_count--;
    iput(pmip); //write pmip inode back to disk

    bdalloc(dev, mip->INODE.i_block[0]); // deallocate the data block used by removed directory
    idalloc(dev, ino);                   // deallocate the inode used by removed directory

    dev = odev; //reset back to orignal dev
    return 0;


}

/*****************************************************
*
*  Name:    rm_child
*  Made by: Reagan Kelley
*  Details: Given an parent directory, fname is found 
*           in the dir_entries, and it is removed.
*
*****************************************************/
int rm_child(MINODE* pmip, char* fname){
    
    INODE inode;
    inode = pmip->INODE;
    
    char buf[BLKSIZE], temp[256];
    DIR *dp, *ldp;
    char *cp, *lcp;

    int last_used_iblock = -1, old_diff;
    //get the last used iblock
    for(int i = 0; i < 12; i++){ // 12 direct blocks
      if(inode.i_block[i] == 0){ 
         last_used_iblock = i - 1;
         break;
      }
    }

    if(last_used_iblock == -1){ //this happens if i_block[0] is not initalied or all 12 blocks are used
      printf("rm_child : i_block could not be identifed\n");
      return -1;
    }

    // find dir entry name in parent

    for(int i = 0; i <= last_used_iblock; i++){ //traverse the used iblocks
      get_block(dev, inode.i_block[i], buf); // look at the next used data block 

      dp = (DIR*)buf;
      cp = buf;

      //traverse the current data block
      while (cp < buf + BLKSIZE){
        strncpy(temp, dp->name, dp->name_len); //copy the name of the dir entry
        temp[dp->name_len] = 0;

        if(strcmp(fname, temp) == 0){ //if the fname and dir entry name match -> this is the dir entry we are looking for
          
          if(dp->rec_len == BLKSIZE){ //condition 1: first and only entry in a data block
            bdalloc(dev, inode.i_block[i]); //deallocate the data block

            pmip->INODE.i_block[i] = 0; // derefrence data block from i_blocks
            
            for(int j = i + 1; j <= last_used_iblock; j++){ //compact i_block array to elimated deleted entry if its between nonzero entries
              if(pmip->INODE.i_block[j] != 0){ //if the next i block is assigned
                printf("rm_child : reformatting parent i_blocks...\n");
                pmip->INODE.i_block[j - 1] = pmip->INODE.i_block[j]; //overwrite the previous i_block value with this one
                pmip->INODE.i_block[j] = 0; //overwrite the i_block value with 0, since its moved in the array

              }else{
                break; //happens once a 0 i_block is found, inidicates no more i_blocks to find
              }
            }

            //iput(pmip); //write inode back to block

            return 0; //condition 1 complete

          }else if(dp->rec_len == (buf + BLKSIZE) - cp){ // condition 2: dir entry is last in the data block

            printf("before-last rec len: %d\nlast rec len: %d\n", ldp->rec_len, dp->rec_len);
            ldp->rec_len += dp->rec_len; //extend the last dir entry to the end of the block (this will stop the removed dir entry from being read)
            put_block(dev, inode.i_block[i], buf); //write back changes to the data block

            printf("Condition 2 complete.\n");
            return 0; //condition 2 complete
          
          }else{ //condition 3: dir entry is in the beginning or middle of the block
            lcp = cp;
            ldp = dp; //keep the deleted dir location
            while (dp->rec_len != (buf + BLKSIZE) - cp){ //go until the last dir entry in the block is found

              cp += dp->rec_len; //go to the next dir entry
              dp = (DIR *)cp;
              

            }
            printf("Try memcpy\n");
            
            old_diff = ldp->rec_len; // keep track of how much space the old dir used to take in the block

            memcpy(ldp, lcp + ldp->rec_len, (buf + BLKSIZE) - (lcp + ldp->rec_len)); //move all block data left from the deleted dir's end
            printf("Worked...\n");

            cp = lcp; //go back to starting location where old dir used to be
            dp = (DIR *)cp;

            while (cp + dp->rec_len != (buf + BLKSIZE) - old_diff){ //go until the last dir entry in the block is found again
              

              cp += dp->rec_len; //go to the next dir entry
              dp = (DIR *)cp;

            }

            dp->rec_len += old_diff; //add the removed dir rec len to last dir entry so it will extend the to the ned of the block again

            put_block(dev, inode.i_block[i], buf); //write back changes to the data block

            return 0; //condition 3 complete


          }
        }
        
        //go to the next dir entry
        lcp = cp; //save last cp loc
        cp += dp->rec_len;
        ldp = dp; //save the last dir entry loc
        dp = (DIR *)cp;
      }
    }

    printf("rm_child : no dir entry was removed ...");
    return -1;
}