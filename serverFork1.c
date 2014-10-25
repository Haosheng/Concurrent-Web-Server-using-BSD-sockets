/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

//10/24/2014 parsetype
typedef struct {
  char *Method;
  char *Path;
  char *IPaddr;
  char *PortNo;
}ReqInfo;

void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
         else //returns the process ID of the child process to the parent
             close(newsockfd); // parent doesn't need this 
     } /* end of while */
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
ReqInfo parseRequest(char *Request)
{
  ReqInfo internalData;
  char *token = NULL;
  token = strtok(Request," ");
  internalData.Method = token;
  if(token!=NULL)
  {
    token = strtok(NULL," ");
  }
  internalData.Path = token;
  if(token!=NULL)
  {
    token = strtok(NULL,":");
    if(token!=NULL)
    {
      token = strtok(NULL,":");
    }
  }
  internalData.IPaddr=token;
  if(token!=NULL)
  {
    token = strtok(NULL,":");
  }
  internalData.PortNo=token;
 return internalData;
}
void fileSend(int sock, char *URL)
{
 int m,n=256;
 FILE *fp;
 char wbuf[256];
 bzero(wbuf,256);
 fp=fopen(URL+1,"r");
 if(fp==NULL)
  {
    fprintf(stderr,"file cannot open");
  }
 while(n==256)
 {
  n=fread(wbuf,1,256,fp);
  if(n<0) error("ERROR read from file");
  m=write(sock,wbuf,256);
  if(m<0) error("ERROR writing to socket");
  bzero(wbuf,256);
 }
 fclose(fp);
}

void dostuff (int sock)
{
   ReqInfo getInfo;
   int n=0;
   char buffer[256];
   char Infobuffer[256];     
   bzero(buffer,256);       
   n=read(sock,buffer,255);
   if (n < 0) error("ERROR reading from socket"); 
   strcpy(Infobuffer,buffer);
   getInfo = parseRequest(Infobuffer);
   printf("%s\n",getInfo.Path);
   printf("%s",buffer);  
   while(n==255)
   {
    bzero(buffer,256);
    n=read(sock,buffer,255);
    printf("%s",buffer);    
   }
   fileSend(sock,getInfo.Path);
   printf("\n");   
   n = write(sock,"I got your message",18);
   if (n < 0) error("ERROR writing to socket");
}
