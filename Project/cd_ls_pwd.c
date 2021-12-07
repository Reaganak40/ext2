/************* cd_ls_pwd.c file **************/

int ls_file(MINODE *mip, char *name);
// functions used from other source files *******
extern int findino(MINODE *mip, u32 *myino);
extern int getino(char *pathname);
extern int findmyname(MINODE *parent, u32 myino, char myname[ ]);
extern int is_dir(MINODE* mip);
//***********************************************

/*****************************************************
*
*  Name:    cd
*  Made by: Reagan Kelley
*  Details: changes working directory to thaty of pathnamwe
* 
*****************************************************/
int cd(char *pathname)
{
  //printf("cd: under construction READ textbook!!!!\n");
  int ino = getino(pathname); //get inode number for pathname
  
  if(!ino){ // directory path could not be found
    printf("cd: pathname does not exist!\n");
    return -1;
  }


  MINODE* mip = iget(dev, ino); //create a new minode with the inode for that pathname

  if(is_dir(mip) != 0){
    printf("can't cd to a non-directory\n");
    return -1;
  }


  iput(running->cwd);           // one less reference to old cwd
  running->cwd = mip; //finalize directory change

  return 0;

}

/*****************************************************
*
*  Name:    ls_file
*  Made by: Reagan Kelley
*  Details: print ls line for given file
* 
*****************************************************/
int ls_file(MINODE *mip, char *name)
{
  //printf("ls_file: to be done: READ textbook!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls
  
  INODE f_inode;
  f_inode = mip->INODE;

  u16 mode = f_inode.i_mode;                    // 16 bits mode
  u16 hard_link_count = f_inode.i_links_count;  // hard link count 
  u16 uid = f_inode.i_uid;                      // owner uid
  u16 gid = f_inode.i_gid;                      // the group id by which this file belongs to
  time_t fmtime = f_inode.i_mtime;                 // file date
  u32 fsize = f_inode.i_size;                   // size of file
  char mode_bits[16];
  mode_bits[16] = '\0';
  char buf[64];

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
  
  if(strcmp(mode_bits + 12, "0001") == 0){ // 1000 -> is a REG file
    printf("-");
  }else if(strcmp(mode_bits + 12, "0010") == 0){ // 0100 -> is a DIR
    printf("d");
  }else if(strcmp(mode_bits + 12, "0101") == 0){ // 0100 -> is a DIR
    printf("l");
  }else{
      printf("[ERROR: Issue reading file type]");

  }
  char* permission_bits = "xwrxwrxwr"; //reversed for convenience
  for(int i = 8; i >= 0; i--){ //read last 8 bits of mode to display permission bits
    if(mode_bits[i] == '1'){
      printf("%c", permission_bits[i]);
    }else if(mode_bits[i] == '0'){
      printf("-");
    }else{
      printf("[ERROR: Issue reading bits]");
      return -1;
    }
  }

  //print hard-link count
  printf("   %d", hard_link_count);

  //print owner uid
  printf("   %d", uid);

  //print gruop id
  printf("   %d", gid);

  // print time in calendar form   
  strcpy(buf, ctime(&fmtime) + 4); // print time in calendar form  (+4 to ignore day of week)     
  buf[strlen(buf)-1] = '\0'; // kill \n at end                                 
  printf("  %s", buf);

  //print file size
  printf (" %6d", fsize);

  printf("    %s\n", name);
  //printf("MODE: %d\n", mode);


  return 0;
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
  //printf("ls_dir: list CWD's file names; YOU FINISH IT as ls -l\n");

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
     fmip = iget(dev, ino);
     
    ls_file(fmip, temp);
     //printf("%s  ", temp); //print dir name

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
int ls(char* pathname)
{
  //printf("ls: list CWD only! YOU FINISH IT for ls pathname\n");

  if(strlen(pathname) == 0){ //no ls pathname
    ls_dir(running->cwd); //running process is what "calls" the ls command, get its cwd
    return 0;
  }

  u32 path_ino;
  MINODE* mip;

  path_ino = getino(pathname); //returns the inode number for this pathname

  if(!path_ino){ //path doesn't exist
    return -1;
  }

  mip = iget(dev, path_ino);     // make a new minode for this path dir


  if(is_dir(mip) != 0){ //pathname is not a directory
    printf("cant ls a non-directory\n");
    return -1;
  }

  ls_dir(mip); //run ls_dir on the pathname
  return 0;
  
}

/*****************************************************
*
*  Name:    pwd
*  Made by: Reagan Kelley
*  Details: Runs the pwd shell command. Recursive 
*           function that works its way up to the root 
*           printing the current dir along the way
* 
*****************************************************/
char *pwd(MINODE *wd)
{

  u32 my_ino, parent_ino;
  MINODE* pip;
  char name[64];
  char* pwd_return_value;

  
  if (wd == root){
    printf("/");

    if(wd == running->cwd){ //this was the last dir in the chain
      printf("\n");
    }

    return 0;
  }
  //printf("dev: %d, ino: %d\n", dev, wd->ino);
  parent_ino = findino(wd, &my_ino); //my_ino is this file's inode number. Parent inode is the parent directory of this file

  if(wd->ino == 2){ //if at a mounted root

    for(int i = 0; i < NMOUNT; i++){ // allocate device to mount table
        if(mountTable[i].dev != 0 && mountTable[i].mounted_inode){
          if(mountTable[i].mounted_inode->ino == parent_ino){
            dev = mountTable[i].mounted_inode->dev; // change to new device
            break;
          }
        }
    }

  }

  pip = iget(dev, parent_ino);       //pip becomes the minode for parent directory



  pwd_return_value = pwd(pip); // start working way back up to root

  if(pip->dev != wd->dev){ // if at a mount crossing point
    strcpy(name, "");
  }else{
    findmyname(pip, my_ino, name); //get the name of myino
  }

  if(pwd_return_value != 0){ //if the parent dir is not root

    if(pip->dev == wd->dev){ // if not at a mount crossing point
      printf("/");
    }
  }
  printf("%s", name);

  if(wd == running->cwd){ //this was the last dir in the chain
    printf("\n");

    if(dev != running->cwd->dev){ //if pwd traversed through different systems
          dev = running->cwd->dev; // reset dev
    }
  }

  return (char*)1;




  
}



