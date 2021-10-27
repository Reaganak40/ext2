/************* cd_ls_pwd.c file **************/
int cd()
{
  printf("cd: under construction READ textbook!!!!\n");

  // READ Chapter 11.7.3 HOW TO chdir
}

int ls_file(MINODE *mip, char *name)
{
  printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
}

/*****************************************************
*
*  Name:    ls_dir
*  Made by: Reagan Kelley
*  Details: list content of directory
* 
*****************************************************/
int ls_dir(MINODE *mip)
{
  printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");

  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;

  u32 ino;      // for file inode number
  MINODE* fmip;  // to point to the inode for file

  get_block(dev, mip->INODE.i_block[0], buf); //goes to the data block that holds this directory info
  dp = (DIR *)buf;  //buf can be read as ext2_dir_entry_2 entries
  cp = buf;         //will always point to the start of dir_entry
  
  while (cp < buf + BLKSIZE){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     
     ino = dp->inode; 

     

     printf("%s  ", temp); //print dir name

     cp += dp->rec_len; //rec_len is entry length in bytes. So, cp will now point to the byte after the end of the file.
     dp = (DIR *)cp;    //dp will now start at cp (the next entry)
  }
  printf("\n");
}

/*****************************************************
*
*  Name:    ls
*  Made by: Reagan Kelley
*  Details: Runs the ls shell command
* 
*****************************************************/
int ls()
{
  printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");
  ls_dir(running->cwd); //running process is what "calls" the ls command, get its cwd
}

char *pwd(MINODE *wd)
{
  printf("pwd: READ HOW TO pwd in textbook!!!!\n");
  if (wd == root){
    printf("/\n");
    return;
  }
}



