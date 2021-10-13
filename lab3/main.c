/***** LAB3 base code *****/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>

char gpath[128];    // hold token strings
char *arg[64];      // token string pointers
int  n;             // number of token strings

char dpath[128];    // hold dir strings in PATH
char *dir[64];      // dir string pointers
int  ndir;          // number of dirs  
char* HOME;

int head, tail; // for pipes
int pd[2];

int tokenize(char *pathname) // YOU have done this in LAB2
{                            // YOU better know how to apply it from now on
  char *s;
  strcpy(gpath, pathname);   // copy into global gpath[]
  s = strtok(gpath, " ");    
  n = 0;
  while(s){
    arg[n++] = s;           // token string pointers  
    s = strtok(0, " ");
  }
  arg[n] =0;                // arg[n] = NULL pointer
}

int check_pipe(void){

  for(int i = 0; i < n; i++){

    if(strcmp(arg[i], "|") == 0){ //indicates pipe

        arg[i] = 0;
        head = 0;
        tail = i+1; //what arg index does tail begin
    
        return 1;

    }
  }

  return 0; // 0 means not pipe created


}

//get path dir, get's home address, and prints initial UI for rksh
void init_sh(char* env[ ]){
 
  char search[5] = { '\0' };
  char* path_loc = 0;

  int found_home = 0, found_path = 0;
  for(int i = 0; env[i] != 0; i++){ //find address of PATH
   
    strncpy(search, env[i], 4);
    if(strcmp(search, "PATH") == 0){
     
      path_loc = env[i]; //get to beggining of path list
      found_path = 1;
      if(found_home)
        break;

    }else if(strcmp(search, "HOME") == 0){
     
      HOME = env[i];
      found_home = 1;
      if(found_path)
        break;
    }
  }


  //tokenize PATH
  char* token;
  char *temp = (char*)malloc(200);  //we allocate memory to the dir strings to avoid ruining PATH with strtok

  strcpy(temp, path_loc);
  printf("%s\n", temp);
  token = strtok(temp + 5, ":");
  ndir = 0;

  while(token != NULL){

    dir[ndir] = token;
    ndir++;
    printf("%s\n", token);
    token = strtok(NULL, ":");
  }


  //DISPLAY SH UI
  printf("********************* Welcome to rksh *********************\n1. Show HOME directory: HOME = %s\n2. Show PATH:\n%s\n3. decompose PATH into dir strings.\n", HOME + 5, path_loc + 5);
  // show dirs
  for(int i=0; i<ndir; i++){
    printf("dir[%d] = %s\n", i, dir[i]);
  }
   
}

void io_redirections(void){
  for(int i = 0; i < n; i++){

         if(strcmp(arg[i], ">") == 0){ // write arg[i-1] content to arg[i+1]
         
          arg[i] = 0;
          close(1);
          open(arg[i+1], O_WRONLY | O_CREAT, 0644);
          break;

         }else if(strcmp(arg[i], ">>") == 0){ //append arg[i-1] content to that of arg[i+1]
         
          arg[i] = 0;
          close(1);
          open(arg[i+1], O_WRONLY | O_APPEND, 0644);
          break;

         }else if(strcmp(arg[i], "<") == 0){ //take content from arg[i+1]
          arg[i] = 0;
          close(0);
          open(arg[i+1], O_RDONLY);
          break;
         }

       }
}

void sh_exec(char* cmd, char* line, char* env[ ]){
    // make a cmd line = dir[0]/cmd for execve()
       int r = -1;
       int i;

       i = 0;
       
       //printf("PROC %d tries %d in each PATH dir:\n", getpid(), arg[0]);
       while(r == -1){
       
        strcpy(line, dir[i]); strcat(line, "/"); strcat(line, cmd);
        //printf("i=%d    cmd=%s\n", i, line);
        
        r = execve(line, arg, env);

        i++;
        if(i >= ndir){ //if exhausted all exeuctibles in PATH
          break;
        }
       
       }

       // this point is reached when execve couldn't find any valid executible in PATH
       printf("execve failed r = %d\n", r);
       exit(1);


}
int main(int argc, char *argv[ ], char *env[ ])
{
  int i;
  int pid, status;
  char *cmd;
  char line[28];

  // The base code assume only ONE dir[0] -> "/bin"
  // YOU do the general case of many dirs from PATH !!!!

  init_sh(env); // user made function : initialize all dir and UI for sh

  printf("4. ***************** rksh proccessing loop *****************\n");
  while(1){
    printf("sh %d running\n", getpid());
    printf("rksh %c : ", '%');
    fgets(line, 128, stdin);
    printf("---------------------------------------\n");
   
    line[strlen(line) - 1] = 0;

    if (line[0]==0)
      continue;
   
    tokenize(line);
    cmd = arg[0];         // line = arg0 arg1 arg2 ...


    for (i=0; i<n; i++){  // show token strings  
        printf("arg[%d] = %s\n", i, arg[i]);
    }
    // getchar();
   
    

    if (strcmp(cmd, "cd")==0){
      chdir(arg[1]);
      continue;
    }
    if (strcmp(cmd, "exit")==0)
      exit(0);
   
     pid = fork();
     
     if (pid){
       printf("sh %d forked a child sh %d\n", getpid(), pid);
       printf("sh %d wait for child sh %d to terminate\n", getpid(), pid);
       pid = wait(&status);
       printf("\nZOMBIE child=%d exitStatus=%x\n", pid, status);
       printf("main sh %d repeat loop\n", getpid());
       printf("---------------------------------------\n");
       
     }
     else{
       
       int p = 0;
       printf("child sh %d running\n", getpid());
       p = check_pipe(); //checks if pipe is neccessary


       if(p){ //if pipe was created
        pipe(pd);
        printf("(%d, %d)\n", pd[0], pd[1]);
        pid = fork();

        for (i=0; i<n; i++){  // show token strings  
            printf("arg[%d] = %s\n", i, arg[i]);
        }
    
        if(pid){
            
            printf("COMMAND: %s\n", cmd);
            if(pd){
              printf("HELLO\n");

              cmd = arg[tail];
              printf("HELLLLOOOOO1: %s\n", cmd);

              close(pd[1]);

              close(0);
              dup(pd[0]);
              close(pd[0]);

              
            }
            printf("HI\n");
            sh_exec(cmd, line, env);
        }else{
            cmd = arg[head];
            close(pd[0]);
            printf("%s\n", line);
            //close(1);   /// ERROR, PROGRAM ENDS
            dup2(pd[1], 1);
            printf("----\n");
            close(pd[1]);

            printf("HELLLLOOOOO: %s\n", cmd);
            sh_exec(cmd, line, env);

        }
    }
       io_redirections(); //user made function : change file descriptor (0,1) and open filestream
       sh_exec(cmd, line, env);

     }
  }
}

/********************* YOU DO ***********************
1. I/O redirections:

Example: line = arg0 arg1 ... > argn-1

  check each arg[i]:
  if arg[i] = ">" {
     arg[i] = 0; // null terminated arg[ ] array
     // do output redirection to arg[i+1] as in Page 131 of BOOK
  }
  Then execve() to change image


2. Pipes:

Single pipe   : cmd1 | cmd2 :  Chapter 3.10.3, 3.11.2

Multiple pipes: Chapter 3.11.2
****************************************************/