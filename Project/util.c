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
extern OFT oft[NOFT];


extern char gpath[128];
extern char *name[64];
extern MOUNT mountTable[NMOUNT]; 

extern int n;

extern int nfd;
extern int fd, dev; //dev is device (disk) file descriptor
extern int balloc(int dev); //same as ialloc but for the block bitmap
extern int is_dir(MINODE* mip);



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

/*****************************************************
*
*  Name:    Put Block
*  Made by: KC
*  Details: Dev is the disk, this function gets the 
*           block indicated by (int blk) and overwrites
*           it with buf
*
*****************************************************/
int put_block(int dev, int blk, char *buf)
{
   lseek(dev, (long)blk*BLKSIZE, 0);
   write(dev, buf, BLKSIZE);
}   

/*****************************************************
*
*  Name:    tokenize
*  Made by: KC
*  Details: Seperates a pathname into its individual 
*           directory name components and puts it in 
*           the list name, N becomes the number of tokens.
*
*****************************************************/
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
*  Name:    init_proc
*  Made by: Reagan
*  Details: Initialized a new proc at pid if it is free. 
*           If it is free assigns the running ptr and 
*           returns 0. Otherwise returns 1 and doesn't 
*           assign anything.
*
*****************************************************/
int init_proc(int pid){
   PROC* p;
   
   p = &proc[pid]; // get process at process id
   if(p->status == FREE){
      p->status = READY;
      running = p;
      nfd = 0; 
      return 0;
   }

   return 1;
}

int add_indirect_entry(int dev, int idblk, int nblk){
   char buf[BLKSIZE];

   get_block(dev, idblk, buf);

   int* ip = (int*)buf;

   while(ip < (int*)(buf + BLKSIZE)){ // go to next empty entry
      if(*ip == 0){ //an empty entry is found
         *ip = nblk;
         put_block(dev, idblk, buf); //set it and write back to disk
         return 0;
      }
      ip++;
   }

   printf("add_indirect_entry : no room to allocate in indirect block\n");
   printf("add_indirect_entry failed\n");
   return -1;



}

int make_indirect_block(int dev){
   int nblk;
   char buf[BLKSIZE];
   nblk = balloc(dev);

   if(!nblk){ //if balloc failed
      printf("make_indirect_block : could not allocate a new data block\n");
      printf("make indirect block failed\n");
      return -1;
   }

   get_block(dev, nblk, buf);

   int* ip = (int*)buf;

   while(ip < (int*)(buf + BLKSIZE)){ //set all indirect block entries to 0
      *ip = 0;
      ip++;
   }

   put_block(dev, nblk, buf);

   return nblk;
}


/*****************************************************
*
*  Name:    get indirect block
*  Made by: Reagan
*  Details: Returns the block within the indirect disk 
*           block (inode.i_block[12])
*           MAJOR PRECONDITION: offset blk_number to 
*           exclude direct blocks
*****************************************************/
int get_indirect_block(int dev, int idblk, int blk_number){

   char buf[BLKSIZE];

   if(blk_number > (BLKSIZE / 4)){ 
      printf("get_indirect_block : exceeded indrect block limit\n");
      printf("get_indirect_block unsuccessful\n");
      return -1;
   }
   //printf("Getting blk...\n");
   get_block(dev, idblk, buf); // get the indirect disk block

   int* ip = (int*)buf; //int pointer

   //printf("assigning ip...\n");
   ip += blk_number; //go to location of indrect_block where the blk number is

   return *ip;

}

/*****************************************************
*
*  Name:    get double indirect block
*  Made by: Reagan
*  Details: Returns the block within the double indirect 
*           disk block (inode.i_block[13])
*           MAJOR PRECONDITION: offset blk_number to 
*           exclude direct blocks and indirect blocks
*****************************************************/
int get_double_indirect_block(int dev, int didblk, int blk_number){

   char buf[BLKSIZE];

   int _diblk = blk_number / (BLKSIZE / 4); //this is what indirect block will have the data block we are looking for
   int to_return; // this will be the true data block number

   if(_diblk > (BLKSIZE / 4)){ // if the data block number is too big to be in the double indirect block (256 indirect blods)
      printf("get_double_indirect_block : exceeded double indrect block limit\n");
      printf("get_double_indirect_block unsuccessful\n");
      return -1;
   }

   get_block(dev, didblk, buf); // get the double indirect disk block

   int* ip = (int*)buf; //int pointer
   ip += _diblk; //go to location of indrect_block where the blk number is

   if(*ip == 0){ // indirect block entry is not assigned, stop here

      return -2; // -2 means that indirect block entry not assigned
   }

   blk_number %= (BLKSIZE / 4); // will divide by 256 -> what blk within the determined indirect blk
   // ip now points to an indirect block
   to_return = get_indirect_block(dev, *ip, blk_number);


   if(to_return == -1){
      printf("get_double_indirect_block unsuccessful\n");
      return -1;
   }
   //printf("returning ip...");
   return to_return;

}
/*****************************************************
*
*  Name:    oset
*  Made by: Reagan
*  Details: Either allocates a new open file table or 
*           get's a preeixsting one to return
*
*****************************************************/
OFT* oset(int dev, MINODE* mip, int mode, int* fd_loc){
   OFT* table;

   for(int i = 0; i < NOFT; i++){ //traverse tables
      table = &oft[i]; // get next open file table

      if(table->minodePtr == mip){ // if minode belongs to this tables minodePtr
         if(table->mode == mode){
            printf("oset : used pre-existing table\n");
            table->refCount++;
            *fd_loc = i;
            return table;
         }

      }
   }

   for(int i = 0; i < NOFT; i++){ //traverse tables
      table = &oft[i]; // get next open file table

      if(table->refCount == 0){ // go to next unused table
         table->minodePtr = mip;
         table->mode = mode;
         table->offset = 0;
         table->refCount = 1;

         int assigned = 0;
         for(int f = 0; f < nfd; f++){
            if(running->fd[f] == 0){ //if there is a free spot in proc's file descriptors list
               running->fd[f] = table;
               *fd_loc = f;
               assigned = 1;
               printf("oset : fd assigned to empty spot in fd list of proc\n");
               break;
   
            }
         }
         
         if(!assigned){ // if no fd spots were available -> expand list
            nfd++;
            if(nfd == NFD){ //if fd limit reached for this proc
               printf("OH NO: FD limit reached for this PROC!\n");
               *fd_loc = -1;
            }else{ //expand list and add fd
               printf("oset : expanded fd limit for this proc\n");
               running->fd[nfd - 1] = table;
               *fd_loc = nfd - 1;
            }

         }
         return table;
      }

   }


   printf("PANIC: No more Open File Tables!\n");

   return 0;

}

/*****************************************************
*
*  Name:    oget
*  Made by: Reagan
*  Details: Returns the open file table at that proc's 
*           file descriptor index.
*
*****************************************************/
OFT* oget(PROC* pp, int fd){
   if(pp->fd[fd] == 0){
      //printf("oget : %d does not indicate any open file table in proc\n", fd);
      return 0;
   }
   return pp->fd[fd];
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

  int table_loc;

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount && mip->dev == dev && mip->ino == ino){ //if target minode is currently being used, its on this device, and it matches ino (inode number)
       mip->refCount++; //increment refCount by 1
       //printf("found [%d %d] as minode[%d] in core\n", dev, ino, i);
       return mip; //return that minode
    }
  }
  //if the minode with desired ino was not found

  for(int i = 0; i < NMOUNT; i++){ // determine where device info is on table for lookup
     if(mountTable[i].dev == dev){
        table_loc = i;
        break;
     }
  }

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount == 0){ //go to next unused minode slot
       //printf("allocating NEW minode[%d] for [%d %d]\n", i, dev, ino);
       mip->refCount = 1;
       mip->dev = dev;
       mip->ino = ino;

       // get INODE of ino to buf    
       blk    = (ino-1)/8 + mountTable[table_loc].iblk; // blk is set the Block that has the requested inode
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

/*****************************************************
*
*  Name:    iput
*  Made by: Reagan
*  Details: Writes mip's inode back to the disk and 
*           deallocates the minode
*
*****************************************************/
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

   block = (mip->ino - 1) / 8 + iblk; //get the block number that contains this inode
   offset = (mip->ino - 1) % 8; // which inode in the block this inode is

   get_block(mip->dev, block, buf); // start reading at inode block
   ip = (INODE*)buf + offset; // ip is the inode we are looking for

   *ip = mip->INODE; // overwrite inode
   put_block(mip->dev, block, buf); //write new changes back to block

   //midalloc
   
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

   if(is_dir(mip) != 0){ //if mip is not a directory, search not possible (By Reagan, found error)
      printf("search : Can't search a non-directory\n");
      return -1;
   }

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

/*****************************************************
*
*  Name:    get ino
*  Made by: KC
*  Details: takes a pathname and returns the inode number 
*           for the last file in the path
*
*****************************************************/
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

      if(ino == -1){ // search encountered an error (By Reagan)
         printf("getino : inode could not be identified in search\n");
         return -1;
      }

      if (ino==0){
         iput(mip);
         printf("name %s does not exist\n", name[i]);
         return 0;
      }

      iput(mip);
      mip = iget(dev, ino); // create new minode for that child directory

      if(mip->mounted){ // IF THIS IS A MOUNT MINODE
         int ndev = mip->mptr->dev;

         iput(mip);  // put back current minode before switching devices
         dev = ndev;

         for(int i = 0; i < NMOUNT; i++){ // switch to mount device

            if(mountTable[i].dev == ndev){
               mip = iget(dev, 2);
               ino = 2;
               break;
            }


         }
      }

   }

   iput(mip); //no more reference to inode needed
   return ino;
}

// These 2 functions are needed for pwd()

/*****************************************************
*
*  Name:    findmyname
*  Made by: Reagan
*  Details: Gets the name of the file with the given inode number. 
*           The file must be a child of the parent directory.
*
*****************************************************/
int findmyname(MINODE *parent, u32 myino, char myname[ ]) 
{
  // WRITE YOUR code here
  // search parent's data block for myino; SAME as search() but by myino
  // copy its name STRING to myname[ ]
  int old_dev = -1;
  if(parent->dev != dev){ //if not on correct device
      old_dev = dev;
      dev = parent->dev;
  }

  int i; 
  char *cp, c, sbuf[BLKSIZE], temp[256];

   ip = &(parent->INODE);

   /*** search for name in mip's data blocks: ASSUME i_block[0] ONLY ***/

   get_block(dev, ip->i_block[0], sbuf);
   dp = (DIR *)sbuf;
   cp = sbuf;

   // searches through data block to file dir entry with that name
   while (cp < sbuf + BLKSIZE){

     
     
     if(dp->inode == myino){ //if dir's inode is myino -> this is the file we are looking for
         strncpy(myname, dp->name, dp->name_len); //get the name of file
         myname[dp->name_len] = 0;

         if(old_dev != -1){ //if device was changed earlier, change back
            dev = old_dev;
         }
         return 0;
     }
     
     cp += dp->rec_len;
     dp = (DIR *)cp;
   }
   printf("(findmyname) : name could not be found in directory\n");
   return -1; //dir name wasn't in directory

}

/*****************************************************
*
*  Name:    findino
*  Made by: KC
*  Details: mip is a directory, myino is changed to 
*           the . directory inode number. 
*           The .. directory inode number is returned.
*
*****************************************************/
extern int ls_dir(MINODE *mip);

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
     }

     if(inos_found[0] == 1 && inos_found[1] == 1){ //found all inodes

         if((parent_ino == 2) && (*myino == 2) && (dev != root->dev)){ //if on a mounted root directory
               for(int i = 0; i < NMOUNT; i++){
                  if(mountTable[i].dev == dev){
                     parent_ino = mountTable[i].mounted_inode->ino; // point back to mount directory
                  }
               }
         }
         break;
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

