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

// LAB4 CODE =================================================================
char gpath[128];    // hold token strings
char *arg[64];      // token string pointers
char cwd[128];     // current working directory
int  n;             // number of token strings


int tokenize(char *pathname);
int set_route_control(void);

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

    printf("********  processing loop  *********\n");
    while (1){
      printf("input a line : ");
      bzero(line, MAX);                // zero out line[ ]
      fgets(line, MAX, stdin);         // get a line (end with \n) from stdin
      

      line[strlen(line)-1] = 0;        // kill \n at end
      if (line[0]==0)                  // exit if NULL line
         exit(0);

      // Send ENTIRE line to server
      n = write(sfd, line, MAX);

      tokenize(line);
      route_control = set_route_control();

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

