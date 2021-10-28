/*********** util.c file ****************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <unistd.h>
#include <time.h>

#include "type.h"

/**** globals defined in main.c file ****/
extern MINODE minode[NMINODE]; //Array of MINODEs (used for processing)
extern MINODE *root; //root minode (used in mount root) gets root dir
extern PROC   proc[NPROC], *running;

extern char gpath[128];
extern char *name[64];
extern int n;

extern int fd, dev; //dev is device (disk) file descriptor

// nblocks = how many blocks are on the disk (1440 for virtual floppy disks (FD))
// ninodes = how many inodes are on the disk
// bmap = what block has bitmap
// imap = what block has Inode bitmap
// iblk = what block has the first Inode
extern int nblocks, ninodes, bmap, imap, iblk;

extern char line[128], cmd[32], pathname[128];

/*****************************************************
*
*  Name:    Get Block
*  Made by: KC
*  Details: Dev is the disk, this function gets the 
*           block indicated by (int blk) and puts 
*           that block info into buf 
*
*****************************************************/
int get_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0); // move dev to read at block nunber: (blk)
   read(dev, buf, BLKSIZE);          // reads block content into buf
}   

int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

int tokenize(char *pathname)
{
  int i;
  char *s;
  printf("tokenize %s\n", pathname);

  strcpy(gpath, pathname);   // tokens are in global gpath[ ]
  n = 0;

  s = strtok(gpath, "/");
  while(s){
    name[n] = s;
    n++;
    s = strtok(0, "/");
  }
  name[n] = 0;
  
  for (i= 0; i<n; i++)
    printf("%s  ", name[i]);
  printf("\n");
}

/*****************************************************
*
*  Name:    iget
*  Made by: KC
*  Details: Creates new minode with specified inode number
*
*****************************************************/
MINODE *iget(int dev, int ino)
{
  int i;
  MINODE *mip;
  char buf[BLKSIZE];
  int blk, offset;
  INODE *ip;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){ //if target minode is currently being used, its on this device, and it matches ino (inode number)
       mip->refCount++; //increment refCount by 1
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip; //return that minode
    }
  }
  //if the minode with desired ino was not found

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){ //go to next unused minode slot
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk    = (ino-1)/8 + iblk; // blk is set the Block that has the requested inode
       offset = (ino-1) % 8;      // offset is where on that Block the inode is found
       // NOTE: We divide by 8 because there can only be 8 inodes per block, we subtract 1 to get the beginning of the inode

       //printf("iget: ino=%d blk=%d offset=%d\n", ino, blk, offset);

       get_block(dev, blk, buf);    // fill buff with content of block with requested inode
       ip = (INODE *)buf + offset;  // offset gets the exact location of inode
       // copy INODE to mp->INODE
       mip->INODE = *ip; //set new minode's inode pointer to the requested inode
       return mip; //return new minode
    }
  }   
  printf("PANIC: no more free minodes\n");
  return 0;
}

void iput(MINODE *mip)
{
 int i, block, offset;
 char buf[BLKSIZE];
 INODE *ip;

 if (mip==0) 
     return;

 mip->refCount--;
 
 if (mip->refCount > 0) return;
 if (!mip->dirty)       return;
 
 /* write INODE back to disk */
 /**************** NOTE ******************************
  For mountroot, we never MODIFY any loaded INODE
                 so no need to write it back
  FOR LATER WROK: MUST write INODE back to disk if refCount==0 && DIRTY

  Write YOUR code here to write INODE back to disk
 *****************************************************/
} 

/*****************************************************
*
*  Name:    search
*  Made by: KC
*  Details: mip is a directory. Search looks through 
*           the contents of that directory, and if it 
*           has a file with the specified name, return 
*           that files inode.
*
*****************************************************/
int search(MINODE *mip, char *name)
{
   int i; 
   char *cp, c, sbuf[BLKSIZE], temp[256];
   DIR *dp;
   INODE *ip;

   printf("search for %s in MINODE = [%d, %d]\n", name,mip->dev,mip->ino);
   ip = &(mip->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;
   printf("  ino   rlen  nlen  name\n");

   // searches through data block to file dir entry with that name
   while (cp < sbuf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     printf("%4d  %4d  %4d    %s\n", 
           dp->inode, dp->rec_len, dp->name_len, dp->name);
     if (strcmp(temp, name)==0){
        printf("found %s : ino = %d\n", temp, dp->inode);
        return dp->inode;
     }
     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   return 0; //dir name wasn't in directory
}

int getino(char *pathname)
{
  int i, ino, blk, offset;
  char buf[BLKSIZE];
  INODE *ip;
  MINODE *mip;

  printf("getino: pathname=%s\n", pathname);
  if (strcmp(pathname, "/")==0)
      return 2; //root is 2nd inode
  
  // starting mip = root OR CWD
  if (pathname[0]=='/')
     mip = root;
  else
     mip = running->cwd;

  mip->refCount++;         // because we iput(mip) later
  
  tokenize(pathname); //divide pathname into dir components

  for (i=0; i<n; i++){
      printf("===========================================\n");
      printf("getino: i=%d name[%d]=%s\n", i, i, name[i]);
 
      ino = search(mip, name[i]); //gets the child for this parent directory with name

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      iput(mip);
      mip = iget(dev, ino); // create new minode for that child directory
   }

   iput(mip); //no more reference to inode needed
   return ino;
}

// These 2 functions are needed for pwd()
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]
}

int findino(MINODE *mip, u32 *myino) // myino = i# of . return i# of ..
{
  // mip points at a DIR minode
  // WRITE your code here: myino = ino of .  return ino of ..
  // all in i_block[0] of this DIR INODE.

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  u32 parent_ino;
  int inos_found[2] = { 0 }; // keeps track of inodes found (parent, and my_ino)
  
  get_block(dev, mip->INODE.i_block[0], buf); //goes to the data block that holds this directory info
  dp = (DIR *)buf;  //buf can be read as ext2_dir_entry_2 entries
  cp = buf;         //will always point to the start of dir_entry
  
  //search data block for (dir .) and (dir ..)
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     
     if(strcmp(temp, ".") == 0){ //the dir is . -> its my_ino
      if(inos_found[1] == 0){ //if my_ino hasn't been found
            *myino = dp->inode; // return my_ino
            inos_found[1] = 1; // my_ino found;
      }
     }

     if(strcmp(temp, "..") == 0){ //the dir is .. -> its parent_ino
      if(inos_found[0] == 0){ //if parent ino hasn't been found
            parent_ino = dp->inode; // return my_ino
            inos_found[0] = 1; // my_ino found;
      }

      if(inos_found[0] == 1 && inos_found[1] == 1){ //found all inodes
         break;
      }
     }

     cp += dp->rec_len; //rec_len is entry length in bytes. So, cp will now point to the byte after the end of the file.
     dp = (DIR *)cp;    //dp will now start at cp (the next entry)
  }

  if(inos_found[0] == 0){
        printf("Error: Could not find parent inode.\n");
        return -1;
   }

   return parent_ino;
}
