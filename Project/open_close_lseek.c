/************* open_close_lseek.c file **************/

#define R 0
#define W 1
#define RW 2
#define APPEND 3

// Function Declarations ****************************
int my_open(char* filename, int flags);
int is_reg(MINODE* mip);
// **************************************************

extern int getino(char *pathname);
extern int creat_pathname(char* pathname);
extern OFT* oset(int dev, MINODE* mip, int mode, int* fd_loc);
extern OFT* oget(PROC* pp, int fd);

extern int my_read(int fd, char* buf, int nbytes);
/*****************************************************
*
*  Name:    is_reg
*  Made by: Reagan
*  Details: Returns 0 if mip is a regular file
*
*****************************************************/
int is_reg(MINODE* mip){
  // CHECK IF mip IS A LINK ***************************
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
  
  if(strcmp(mode_bits + 12, "0101") == 0){ // 1000 -> is a LINK file, 
    //printf("ls: Can't ls a non-directory.\n");
    iput(mip); //derefrence useless minode to ref file
    return -1;
  }else if(strcmp(mode_bits + 12, "0010") == 0){ // 0100 -> is a DIR,  
    //printf("ls: Can't ls a non-directory.\n");
    iput(mip); //derefrence useless minode to ref file
    return -1;
  }
  //************************************************************

  return 0;
}

int has_permission(MINODE* mip, int _mode){

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
  
  char user_permissions[4];
  user_permissions[3] = '\0';
  int ui = 0;
  char* permission_bits = "xwrxwrxwr"; //reversed for convenience
  for(int i = 8; i >= 6; i--){ //read the user permission bits
    if(mode_bits[i] == '1'){
      user_permissions[ui] = permission_bits[i];
    }else if(mode_bits[i] == '0'){
      user_permissions[ui] = '-';
    }else{
      printf("[ERROR: Issue reading bits]");
      return -1;
    }
    ui++;
  }

  printf("user: %s\n", user_permissions);

  // Check if the permissions match the mode
  if(mode == R && user_permissions[0] == '-'){
      printf("Permission denied for mode: read\n");
      return -1;
  }else if(mode == W && user_permissions[1] == '-'){
      printf("Permission denied for mode: write\n");
      return -1;
  }else if(mode == RW && (user_permissions[0] == '-' || user_permissions[1] == '-')){
      printf("Permission denied for mode: read-write\n");
      return -1;
  }else if(mode == APPEND && (user_permissions[0] == '-' || user_permissions[1] == '-')){
      printf("Permission denied for mode: read-write\n");
      return -1;
  }

  return 0;

}

/*****************************************************
*
*  Name:    my_open
*  Made by: Reagan
*  Details: Opens a file for read or write, 
*           returns file descripor
*
*****************************************************/
int my_open(char* filename, int flags){

    int ino, mode;
    MINODE* mip;
    OFT* table;
    int fd_loc = 0;

    // validate the open mode
    if(flags == R || flags == W || flags == RW || flags == APPEND){
        mode = flags;
    }else{
        printf("Invalid mode value\nOpen unsuccessful\n");
        return -1;
    }

    ino = getino(filename); // get inode number for the file
    if(!ino){ // if file does not exist
       
       creat_pathname(filename); // creat file
       ino = getino(filename); // get inode number for the file

    }

    mip = iget(dev, ino);

    if(is_reg(mip) != 0){ // if file is not a reg type

        printf("[%s] is not a REG file type.\n", filename);
        printf("Open unsuccessful\n");
        return -1;

    }

    if(has_permission(mip, mode) != 0){ // check if user has permission for given mode
        printf("Open unsuccessful\n");
        return -1;
    }

    table = oset(dev, mip, mode, &fd_loc);

    if(fd_loc == -1){
      printf("Open unsuccessful\n");
      return -1;
    }

    return fd_loc;


}

/*****************************************************
*
*  Name:    my_close
*  Made by: Reagan
*  Details: Closes a file descriptor and deallocates 
*           the inode and open file table
*
*****************************************************/
int my_close(int fd){
  OFT* table;

  table = oget(running, fd);

  if(!table){ //if fd does not correlate with a open file table in this proc
    printf("my_close unsuccessful\n");
    return -1;
  }

  table->refCount--; //decrement ref count for table

  if(table->refCount == 0){ // if no more refs to this open file table
    iput(table->minodePtr);
  }

  running->fd[fd] = 0; // remove this open file table from this processes fd list

  return 0;

}

/*****************************************************
*
*  Name:    my_lseek
*  Made by: Reagan
*  Details: For a given fd, change the offset to the 
*           position vlue
*
*****************************************************/
int my_lseek(int fd, int position){ // no whence parameter for project
  OFT* table;

  table = oget(running, fd);

  if(table == 0 || !table->minodePtr){
    printf("my_lseek : open file table not assigned or no minode assigned\n");
    printf("my_lseek unsuccessful\n");
    return -1;
  }

  if(table->minodePtr->INODE.i_size < position){
    printf("my_lseek : Position exceeds the size of the file\n");
    printf("my_lseek unsuccessful\n");
    return -1;
  }

  table->offset = position;

  return 0;
}

/*****************************************************
*
*  Name:    pfd
*  Made by: Reagan
*  Details: Prints all the assigned file descriptors 
*           for the running proc
*
*****************************************************/
int pfd(void){

  OFT* table;
  char mode[12];
  int ino;


  printf("fd    mode       offset    inode\n");
  printf("---   ----       ------    -------\n");

  for(int i = 0; i < nfd; i++){
    table = oget(running, i);

    if(table){
      if(table->mode == R){
        strcpy(mode, "READ");
      }else if(table->mode == W){
        strcpy(mode, "WRITE");
      }else if(table->mode == RW){
        strcpy(mode, "READ/WRITE");
      }else if(table->mode == APPEND){
        strcpy(mode, "APPEND");
      }

      ino = table->minodePtr->ino;

      printf("%-5d %-10s %-8d [dev, %d]\n", i, mode, table->offset, ino);


    }
  }


}
int level_2_debeug(char* pathname){

  int fd;
  char buf[3000];
  int end;
  
  fd = my_open(pathname, RW);
  my_lseek(fd, 0);

  end = my_read(fd, buf, 100);

  printf("Content:\n");
  buf[end] = '\0';

  printf("%s\nend was = %d\n", buf, end);

  return 0;
  //my_close(fd);

}
