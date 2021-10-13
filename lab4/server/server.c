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
#include <libgen.h>
#include <time.h>

#define MAX 256
#define PORT 1234

int n;

char ans[MAX];
char line[MAX];

int main() 
{ 
    int sfd, cfd, len; 
    struct sockaddr_in saddr, caddr; 
    int i, length;
    
    printf("1. create a socket\n");
    sfd = socket(AF_INET, SOCK_STREAM, 0); 
    if (sfd < 0) { 
        printf("socket creation failed\n"); 
        exit(0); 
    }
    
    printf("2. fill in server IP and port number\n");
    bzero(&saddr, sizeof(saddr)); 
    saddr.sin_family = AF_INET; 
    //saddr.sin_addr.s_addr = htonl(INADDR_ANY); 
    saddr.sin_addr.s_addr = inet_addr("127.0.0.1"); 
    saddr.sin_port = htons(PORT);
    
    printf("3. bind socket to server\n");
    if ((bind(sfd, (struct sockaddr *)&saddr, sizeof(saddr))) != 0) { 
        printf("socket bind failed\n"); 
        exit(0); 
    }
      
    // Now server is ready to listen and verification 
    if ((listen(sfd, 5)) != 0) { 
        printf("Listen failed\n"); 
        exit(0); 
    }

    while(1){
       // Try to accept a client connection as descriptor newsock
       length = sizeof(caddr);

       cfd = accept(sfd, (struct sockaddr *)&caddr, &length);
       
       if (cfd < 0){
          printf("server: accept error\n");
          exit(1);
       }

       printf("server: accepted a client connection from\n");
       printf("-----------------------------------------------\n");
       printf("    IP=%s  port=%d\n", "127.0.0.1", ntohs(caddr.sin_port));
       printf("-----------------------------------------------\n");

       // Processing loop
       while(1){
         printf("server ready for next request ....\n");
         n = read(cfd, line, MAX);
         if (n==0){
           printf("server: client died, server loops\n");
           close(cfd);
           break;
         }

         // show the line string
         printf("server: read  n=%d bytes; line=[%s]\n", n, line);

         strcat(line, " ECHO");

         // send the echo line to client 
         n = write(cfd, line, MAX);

         printf("server: wrote n=%d bytes; ECHO=[%s]\n", n, line);
         printf("server: ready for next request\n");
       }
    }
}