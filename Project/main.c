/****************************************************************************
*                   KCW: mount root file system                             *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>

#include "type.h"

// functions used from other source files *******
extern MINODE *iget();
extern int get_block(int dev, int blk, char *buf);
extern void iput(MINODE *mip);
// *********************************************
int quit(); //local function defintion

MINODE minode[NMINODE];
MINODE *root;
PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[64];  // assume at most 64 components in pathname
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, iblk;
char line[128], cmd[32], pathname[128];

#include "cd_ls_pwd.c"
#include "mkdir_creat.c"
#include "rmdir.c"

/*****************************************************
*
*  Name:    Init
*  Made by: KC
*  Details: Sets all global minode stuff and processes
*           to 0.
*****************************************************/
int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  //initialize all minodes to 0 (no minodes)
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }

  //Initialize all procs to 0 (no procs)
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
  }
}

/*****************************************************
*
*  Name:    mount root
*  Made by: KC
*  Details: load root INODE and set root pointer to it
*
*****************************************************/
int mount_root()
{  
  printf("mount_root()\n");
  root = iget(dev, 2); // 2nd inode is always root in ext2 file system
}

char *disk = "diskimage";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];

  //opens disk for read and write
  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }

  dev = fd;    // global dev same as this fd   

  /********** read super block  ****************/
  get_block(dev, 1, buf); // BLOCK #1 is reserved for Superblock
  sp = (SUPER *)buf;      // sp (super-pointer) reads buf as a ext2_super_block

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){ // 0xEF53 specifies that its an ext2 file system
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count; //how many inodes are on the disk
  nblocks = sp->s_blocks_count; //how many blocks are on the disk

  /********** Group Descriptor Block *************/
  get_block(dev, 2, buf); //  BLOCK #2 is reserved for Group Descriptor Block
  gp = (GD *)buf;         //  gd (group descriptor pointer) reads buf as a ext2_group_desc

  bmap = gp->bg_block_bitmap; // bmap info received from group descriptor block
  imap = gp->bg_inode_bitmap; // imap info received from group descriptor block
  iblk = gp->bg_inode_table;  // iblk info received from group descriptor block
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, iblk);

  init();       // initilize all globals (procs and minodes)
  mount_root(); // Sets root pointer to an minode that contains the root dir inode
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];           // P0 is initialized (super-user)
  running->status = READY;      // Proc is ready
  running->cwd = iget(dev, 2);  //ino 2 is root directory, so iget will make p0's cwd root directory
  printf("root refCount = %d\n", root->refCount);

  // WRTIE code here to create P1 as a USER process
  
  while(1){ //shell loop
    printf("input command : [ls|cd|pwd|mkdir|rmdir|creat|quit] ");
    fgets(line, 128, stdin);  //get command from user
    line[strlen(line)-1] = 0;

    if (line[0]==0) //no command given, loop
       continue;
    pathname[0] = 0;

    sscanf(line, "%s %s", cmd, pathname);  //tokenize cmd and pathname from user input
    printf("cmd=%s pathname=%s\n", cmd, pathname);
  
    //HANDLING COMMANDS
    if (strcmp(cmd, "ls")==0)
       ls(pathname);
    else if (strcmp(cmd, "cd")==0)
       cd(pathname);
    else if (strcmp(cmd, "pwd")==0)
       pwd(running->cwd);
    else if (strcmp(cmd, "mkdir")==0)
       mkdir_pathname(pathname);
    else if (strcmp(cmd, "rmdir")==0)
       rmdir_pathname(pathname);
    else if (strcmp(cmd, "creat")==0)
       creat_pathname(pathname);
    else if (strcmp(cmd, "quit")==0)
       quit();
  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      iput(mip);
  }
  exit(0);
}
