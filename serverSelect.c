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
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */

#include <string.h>
#include <time.h>
#include <sys/stat.h>


#define BUF_SIZE 256

//Request Info
typedef struct {
char *Method;
char *Path;
char *IPaddr;
char *PortNo;
char *Protocol;}ReqInfo;
 
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

//Main Using select()
int main(int argc, char *argv[])
{
    /*********************/
    fd_set readfds;
    fd_set allfds;
    int fdmax;
    FD_ZERO(&readfds);
    FD_ZERO(&allfds);
    int n,m;
    int nready;
    int i;int maxi=0;
    /*********************/
    
    int sockfd, newsockfd, portno, pid;
    socklen_t clilen;
    struct sockaddr_in serv_addr, cli_addr;
    struct sigaction sa;          // for signal SIGCHLD
    
    /**********************/
    int client[FD_SETSIZE];
    for(i=0;i<FD_SETSIZE;i++)
        client[i]=-1;
    /*********************/
    
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

    /******************************/
    FD_SET(sockfd, &allfds);
    fdmax = sockfd;
    /******************************/

    /****** Kill Zombie Processes ******/
    sa.sa_handler = sigchld_handler; // reap all dead processes
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("sigaction");
        exit(1);
    }
    /*********************************/

    /*******change for select()*******/
    while (1) {
        readfds = allfds;
        nready=select(fdmax+1, &readfds, NULL, NULL, NULL);
        if(nready<0)
        {
            perror("select");
            exit(-1);
        }
        if(nready==0)
            continue;
        if(FD_ISSET(sockfd,&readfds))//new client coming
        {
            newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
            if (newsockfd < 0)
                error("ERROR on accept");
            for(i=0;i<FD_SETSIZE;i++)
            {
                if(client[i]<0)
                {
                    client[i]=newsockfd;
                    if(i>maxi) maxi=i;
                    break;
                }
            }
            if(i==FD_SETSIZE)
            {
                fprintf(stderr,"too many clients\n");
                exit(-1);
            }
            FD_SET(newsockfd,&allfds);
            if(newsockfd>fdmax) fdmax=newsockfd;
            if(--nready<=0) continue;
        }
        for(i=0;i<=maxi;i++)
        {
            newsockfd = client[i];
            if(newsockfd==-1) continue;
            if(FD_ISSET(newsockfd,&readfds))
            {
                dostuff(newsockfd);
                close(newsockfd);
                client[i]=-1;
                FD_CLR(newsockfd,&allfds);
                
                if(--nready<=0) break;
            }
            
        }
        
    } /* end of while */
    return 0; /* we never get here */
}

//GET Info(FilePath,IP,Portno) IP,Portno not used here
ReqInfo parseRequest(char *Request)
 {  ReqInfo internalData;
    char *needle ="HTTP/";
     char *temp = Request;
     char *buf = strstr(temp, needle);
     internalData.Protocol=buf;
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
    internalData.IPaddr = token;
    if(token!=NULL)
    {
    token = strtok(NULL,":");
    }
    internalData.PortNo = token;
    return internalData;
}

//header lines

int IsValidHeader(ReqInfo request)
{
    if((strcmp("GET",request.Method)==0)||(strcmp("POST",request.Method)==0))
        if((strcmp("HTTP/1.1\r\nHost",request.Protocol)==0)||(strcmp("HTTP/1.0\r\nHost",request.Protocol)==0))
            return 1;
    return 0;
}

//header-ResponseTime
char *CurrentTime()
{
  time_t timep;
  time(&timep); 
  return ctime(&timep);
}

//header-FileLastModified
char *LastModified(char* FilePath)
{
    if(FilePath[0]=='/')
    FilePath++;
    struct stat buf;
  int rtn = stat(FilePath,&buf);
    if(rtn!=0)
    {
        return NULL;
    }
  time_t t = buf.st_mtime;
  char *output;
  output = ctime(&t);
  return output;
}

//header-FileLength
int Length(char* FilePath)
{
    if(FilePath[0]=='/')
    FilePath++;
  struct stat buf;
  int rtn = stat(FilePath,&buf);
    if(rtn!=0)
    { return -1;}
  int len = (int)(buf.st_size);  
  return len;
}

//header-Filetype
char *FileType(char* FilePath)
{
    char *p;
    p= rindex(FilePath,'.'); //reverse index find '.'
    if(strcmp(p,".txt")==0)
        return "text/html";
    else if(strcmp(p,".html")==0)
        return "text/html";
    else if(strcmp(p,".htm")==0)//for old system
        return "text/html";
    else if(strcmp(p,".jpg")==0)
        return "image/jpg";
    else if(strcmp(p,".gif")==0)
        return "image/gif";
    else if(strcmp(p,".mp4")==0)
        return "video/mp4";
    else if(strcmp(p,".png")==0)
        return "image/png";
    else if(strcmp(p,".pdf")==0)
        return "pdf/application";
    else return "";
    
}

//Construct Response Header 
char *constructHeader(char* FilePath)
{
   FILE *fp;
    fp=fopen(FilePath+1,"r");
   if(fp==NULL)
     return NULL;
   else
   {
    char *header =malloc(sizeof(char)*BUF_SIZE);
    memset(header,0,sizeof(char)*BUF_SIZE);
    strcpy(header,"HTTP/1.1 200 OK\nConnection: close\nDate: ");
    char *time = CurrentTime();
    strcat(header,time); 
    strcat(header,"Last-Modified: ");
    char *modtime = LastModified(FilePath);
    strcat(header,modtime); 
    strcat(header,"Content-Length: ");
    int leng = Length(FilePath);
    char len[32];
    sprintf(len,"%d",leng);
    strcat(header,len);  
    strcat(header,"\nContent-Type: ");
    char *type = FileType(FilePath);
    strcat(header,type);  
    strcat(header,"\r\n\r\n");    
    return header;
    free(header);
    }
   fclose(fp);   
}

//Send Data With Header to Client
void datasend(int sock,char* URL)
{ 
  int m,n=BUF_SIZE,hlen;//hlen is header length
  FILE *fp;
  char *head=constructHeader(URL);
  char wbuf[BUF_SIZE];
  bzero(wbuf,BUF_SIZE);
  fp=fopen(URL+1,"rb");
   hlen=strlen(head);
   printf("%s\n",head);
   strcpy(wbuf,head);
   //printf("%d",hlen);
   //printf("%d",m);
   m=fread(wbuf+hlen,1,BUF_SIZE-hlen,fp);//combine header and data
   if (m < 0) error("ERROR read from file");
   m=send (sock,wbuf,hlen+m,0);
   if (m < 0) error("ERROR writing to socket");
   //printf("%s",wbuf);
   bzero(wbuf,BUF_SIZE);
  while (n == BUF_SIZE)
    {n=fread(wbuf,1,BUF_SIZE,fp);
     //printf("%d",n);
     if (n < 0) error("ERROR read from file");
     m=send(sock,wbuf,BUF_SIZE,0);
     //printf("%s",wbuf);
     //printf("%d",m);
     if (m < 0) error("ERROR writing to socket");
     bzero(wbuf,BUF_SIZE);
     //if(feof(fp)==0) break;
    }
  fclose(fp);
 }

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
   int n,m;
   char buffer[BUF_SIZE],request[BUF_SIZE];
   bzero(buffer,BUF_SIZE);
   ReqInfo internalData;
   n = recv(sock,buffer,BUF_SIZE-1,0);
   if (n < 0) error("ERROR reading from socket");
   strcpy(request,buffer);
    internalData = parseRequest(request);
    //printf("%s,%s\n",internalData.Method,internalData.Protocol);
    printf("Here is the message:\n%s",buffer);
    bzero(buffer,BUF_SIZE);
    //PART A: Dump Request Message to the Console
    while (n == BUF_SIZE-1)
    {
        n = recv(sock,buffer,BUF_SIZE-1,0);
        if (n < 0) error("ERROR reading from socket");
        printf("%s",buffer);
        bzero(buffer,BUF_SIZE);
    }
   if(!IsValidHeader(internalData))
   {
       m = send(sock,"HTTP/1.1 400 Bad Request\r\n\r\n 400 Bad Request",sizeof("HTTP/1.1 400 Bad Request\r\n\r\n 400 Bad Request")-1,0);
       printf("HTTP/1.1 400 Bad Request\r\n\r\n");
       if (m < 0) error("ERROR writing to socket");
   }
   else if(Length(internalData.Path)<0)
    {
        m = send(sock,"HTTP/1.1 404 Not Found\r\n\r\n 404 Not found",sizeof("HTTP/1.1 404 Not Found\r\n\r\n 404 Not found")-1,0);
        printf("HTTP/1.1 404 Not Found\r\n\r\n");
        if (m < 0) error("ERROR writing to socket");
        //printf("%d",m);
        fprintf(stderr,"file cannot open\n");
    }
   
    else
    {datasend(sock,internalData.Path);}
   
}



