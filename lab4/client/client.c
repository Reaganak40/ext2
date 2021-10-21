#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>

#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>
#include <libgen.h>     // for dirname()/basename()
#include <time.h> 

#define MAX 256
#define PORT 1234

#define FOR_SERVER 0
#define IS_GET 1
#define IS_PUT 2
#define FOR_CLIENT 3


char line[MAX], ans[MAX];
int n;

struct sockaddr_in saddr; 
int sfd;

// LAB4 CODE (LOCAL)=================================================================
char gpath[128];    // hold token strings
char *arg[64];      // token string pointers
char cwd[128];     // current working directory
int  n;             // number of token strings

int lab4_run_command(char* cmd);
int tokenize(char *pathname);
int set_route_control(void);
int my_ls(void);
int my_cd(void);
int my_pwd(void);
int my_mkdir(void);
int my_rmdir(void);
int my_rm(void);
int ls_file(char* fname);
int ls_dir(char* dname);
int my_cat(void);
//int send_packet(int end);

int lab4_run_command(char* cmd){ // 'main' function that runs lab4
  
  if(strcmp(arg[0], "ls") == 0){ // cmd: ls
    return my_ls();
  }

  if(strcmp(arg[0], "cd") == 0){ // cmd: cd
    return my_cd();
  }

  if(strcmp(arg[0], "pwd") == 0){ // cmd: pwd
    return my_pwd();
  }

  if(strcmp(arg[0], "mkdir") == 0){ // cmd: mkdir
    return my_mkdir();
  }

  if(strcmp(arg[0], "rmdir") == 0){ // cmd: rmdir
    return my_rmdir();
  }

  if(strcmp(arg[0], "rm") == 0){ // cmd: rm
    return my_rm();
  }

  if(strcmp(arg[0], "cat") == 0){ // cmd: rm
    return my_cat();
  }




  //CMD is not defined
  printf("Invalid cmd: %s\n", arg[0]);

  return -1;
}


int tokenize(char *pathname) // Tokenize from previous lab
{                            
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

int set_route_control(void){
  if(strcmp(arg[0], "get") == 0){
    return IS_GET;
  }

  if(*(arg[0]) == 'l' && (strcmp(arg[0], "ls") != 0)){ //if cmd starts with 'l' and isnt "ls"
    char* s = arg[0];
    arg[0] = ++s;

    return FOR_CLIENT; //is a local command
  }
}

int my_cd(void){

  return chdir(arg[1]);
}

int my_pwd(void){
  getcwd(cwd, 127);
  printf("%s\n", cwd);

  return 0;
}

int my_mkdir(void){

  return mkdir(arg[1], 0755);
}

int my_rmdir(void){

  return rmdir(arg[1]);
}

int my_rm(void){

  return unlink(arg[1]);
}

int my_ls(void){
  
  getcwd(cwd, 127);
  return ls_dir(cwd);
}

int ls_file(char* fname){

  char *t1 = "xwrxwrxwr-------";
  char *t2 = "----------------";

  struct stat fstat, *sp;
  int r, i;
  char ftime[64];
  char linkname[255];
  char char_to_add[2] = { '\0' }; //concat 1 char to ans
  char buf[64]; //concat formatted lines to ans
  char ls_line[MAX];
  strcpy(ls_line, "");

  sp = &fstat;
  if((r = lstat(fname, &fstat)) < 0) {

    printf("can't stat %s\n", fname);
    exit(1);

  }

  if ((sp->st_mode & 0xF000) == 0x8000) // if (S_ISREG())                       
    strcat(ls_line, "-");

  if ((sp->st_mode & 0xF000) == 0x4000) // if (S_ISDIR())                       
    strcat(ls_line, "d");


  if ((sp->st_mode & 0xF000) == 0xA000) // if (S_ISLNK())                       
    strcat(ls_line, "l");
  for(i = 8; i >= 0; i--){
    if(sp->st_mode & (1 << i)){ //print r | w | x   
      char_to_add[0] = t1[i];
      strcat(ls_line, char_to_add);
    }
    else{
      char_to_add[0] = t2[i];
      strcat(ls_line, char_to_add);   

    }                                          
   }

  sprintf(buf, "%4d ",sp->st_nlink); // link count
  strcat(ls_line, buf);

  sprintf(buf, "%4d ",sp->st_gid); // gid
  strcat(ls_line, buf);

  sprintf(buf, "%4d ",sp->st_uid); // uid   
  strcat(ls_line, buf);

  sprintf(buf, "%8d ",sp->st_size); // file size  
  strcat(ls_line, buf);

  // print time  
  strcpy(ftime, ctime(&sp->st_ctime)); // print time in calendar form           
  ftime[strlen(ftime)-1] = 0; // kill \n at end                                 
  strcat(ls_line, ftime);

  // print name                                                                 
  sprintf(buf, " %s \t", basename(fname)); // print file basename  
  strcat(ls_line, buf);

  // print -> linkname if symbolic file                                         
  if ((sp->st_mode & 0xF000)== 0xA000){

    // use readlink() to read linkname                                          
    readlink(fname, linkname, 255);
    strcat(ls_line, " -> "); // print linked name    
    strcat(ls_line, linkname);

  
  }

  printf("%s", ls_line); //print the line build throughout the function
  return 0;

}

int ls_dir(char* dname){ //takes directory and iterates (and prints) through file content

  DIR* dir;
  struct dirent *d;
  dir = opendir(dname);

  if(dir == NULL){
    return -1;
  }

  while(d = readdir(dir)){
      
      ls_file(d->d_name);
      printf("\n");

    
  }

  close(dir);

  return 0;
}

int my_cat(void){
  FILE* rFile; //file to read
  char buf[MAX]; //where file content is read to before send_packet()
  rFile = fopen(arg[1], "r");

  if(!rFile){
    strcat(ans, "Error: Can't open file for mode read.\n");
    return -1; //can't open file for mode read
  }

  //begin reading file content
  while(fgets(buf, MAX, rFile) != NULL){

    printf("%s", buf);
    
    if(feof(rFile)){ //if reached end of file
      break;
    }
  }
  fclose(rFile);

  return 0;
}

//============================================================================


int main(int argc, char *argv[], char *env[]) 
{ 
    int n; char how[64];
    int i;
    int route_control; //determines how program will react to (server cmd, client cmd, or get/put)
    int stdout_copy = dup(1);
    int fd;
    char verify[MAX]; //verification message
    strcpy(verify, "ABC123");

    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    saddr.sin_port = htons(PORT); 
  
    printf("3. connect to server\n");
    if (connect(sfd, (struct sockaddr *)&saddr, sizeof(saddr)) != 0) { 
        printf("connection with the server failed...\n"); 
        exit(0); 
    } 

    printf("********  processing loop  *********\n\n");
    while (1){

      printf("\n********************* menu ***********************\n");
      printf("* get  put  ls   cd   pwd   mkdir   rmdir   rm   *\n");
      printf("* lcat     lls  lcd  lpwd  lmkdir  lrmdir  lrm   *\n");
      printf("**************************************************\n");

      printf("input a line : ");
      bzero(line, MAX);                // zero out line[ ]
      fgets(line, MAX, stdin);         // get a line (end with \n) from stdin
      

      line[strlen(line)-1] = 0;        // kill \n at end
      if (line[0]==0)                  // exit if NULL line
         exit(0);

      tokenize(line);
      route_control = set_route_control();

      if(route_control == FOR_CLIENT){ //if cmd is to be done locally
        int cmd_success;
        cmd_success = lab4_run_command(line);

        if(!cmd_success){ //cmd_sucess == 0 when lab4_run_command() encountered no issues
              printf("[Local CMD Successful]\n");
          }else{
              printf("[Local CMD Unsuccessful]\n");
        }
        continue; //back to start of loop
      }

      // if this point is reached: not a local command
      // Send ENTIRE line to server
      n = write(sfd, line, MAX);


      if(route_control == IS_GET){ //redirect to write to file
         close(1);
         fd = open(arg[1], O_WRONLY | O_CREAT, 0644);
      }
      
      char debug[MAX];
      strcpy(debug, "");

      // Read packets from sock and show it
      strcpy(ans, ""); //reset ans
      while(strcmp(debug, verify) != 0){ // server has more to send
        n = read(sfd, ans, MAX);
      
        n = write(sfd, verify, MAX); //verify with server packet was recieved
        n = read(sfd, debug, MAX); // server sends back verify message if this was the last packet
        /*
        if((route_control == IS_GET) && (strcmp(debug, verify) == 0)){ //last packet on get call
          printf("%s", ans);
          printf("%c", *(ans + strlen(ans) - 1));
        }else{
        */
        printf("%s", ans);
        


      } //while loops ends with [CMD Successful] or [CMD Unsuccessful]
      if(route_control == IS_GET){ //close file before returning back to get next cmd
        close(fd);
        dup2(stdout_copy, 1);
        close(stdout_copy);
      }
  }
}

