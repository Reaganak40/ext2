
/*********** permissions.c file ****************/


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
    }

    p->uid = pid;

    if(running && (p->cwd == 0)){
        p->cwd = iget(running->cwd->dev, running->cwd->ino);
    }else if(running && (running->cwd->dev != p->cwd->dev)){
        dev = p->cwd->dev;
    }
    running = p;
    nfd = 0; 


    return 1;
}

int make_proc(char* pathname){

    if(strlen(pathname) == 0){ //if no parameter display running processes
        printf("List of Current Running Proccesses:\n");

        for(int i = 0; i < NPROC; i++){
            if(proc[i].status == READY){
                printf("PID: %-3d GID: %-3d CWD: (ino %d @ dev %d)", proc[i].pid, proc[i].gid, proc[i].cwd->ino,  proc[i].cwd->dev);

                if(&proc[i] == running){
                    printf (" ... ACTIVE USER");
                }
                printf("\n");
            }
        }

        return 0;
    }
    int target_proc;

    target_proc = atoi(pathname);

    if(target_proc == 0 && pathname[0] != '0'){
        printf("make_proc: parameter must be an int (proc id)\nproc unsuccessful\n");
        return -1;
    }

    if(target_proc >= NPROC){
        printf("make_proc: requested proc id exceeds limit!\nproc unsuccessful\n");
        return 0;
    }else if(target_proc < 0){
        printf("make_proc: requested proc id cannot be <0\nproc unsuccessful\n");
        return 0;
    }

    init_proc(target_proc);
}

int has_permission(MINODE* mip, int _mode){

  INODE f_inode;
  f_inode = mip->INODE;
  u16 mode = f_inode.i_mode;                    // 16 bits mode
  char mode_bits[16];
  mode_bits[16] = '\0';

  if(running->uid == 0){ // if super user
      return 0; // SUPERUSER always okay
  }

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

  
  char user_permissions[10];
  user_permissions[9] = '\0';
  int ui = 0;
  char* permission_bits = "xwrxwrxwr"; //reversed for convenience
  for(int i = 8; i >= 0; i--){ //read the user permission bits
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

  //printf("user: %s\n", user_permissions);

  if(f_inode.i_uid == running->uid){ //is owner -> check owner permissions

    // Check if the permissions match the mode: OWNER
    if(_mode == R && (user_permissions[0] == '-')){
        printf("Permission denied for mode: read\n");
        return -1;
    }else if(_mode == W && (user_permissions[1] == '-')){
        printf("Permission denied for mode: write\n");
        return -1;
    }else if(_mode == RW && ((user_permissions[0] == '-') || (user_permissions[1] == '-'))){
        printf("Permission denied for mode: read-write\n");
        return -1;
    }else if(_mode == APPEND && ((user_permissions[0] == '-') || (user_permissions[1] == '-'))){
        printf("Permission denied for mode: read-write\n");
        return -1;
    }else if(_mode == X && (user_permissions[3] == '-')){
        printf("Permission denied to execute this file.\n");
        return -1;
    }

  }else{ // is not owner -> check other permissions

    if(_mode == OWNER_ONLY){
        printf("Permissions denied. Not owner.\n");
        return -1;
    }
    // Check if the permissions match the mode: OTHER
    if(_mode == R && (user_permissions[6] == '-')){
        printf("Permission denied for mode: read\n");
        return -1;
    }else if(_mode == W && (user_permissions[7] == '-')){
        printf("Permission denied for mode: write\n");
        return -1;
    }else if(_mode == RW && ((user_permissions[6] == '-') || (user_permissions[7] == '-'))){
        printf("Permission denied for mode: read-write\n");
        return -1;
    }else if(_mode == APPEND && ((user_permissions[6] == '-') || (user_permissions[7] == '-'))){
        printf("Permission denied for mode: read-write\n");
        return -1;
    }else if(_mode == X && (user_permissions[8] == '-')){
        printf("Permission denied to execute this file.\n");
        return -1;
    }

  }

  

  return 0;

}
