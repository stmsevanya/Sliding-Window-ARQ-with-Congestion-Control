/*
 * udpserver.c - A UDP echo server
 * usage: udpserver <port_for_server> <dropProbability>
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include<fcntl.h>
#include <openssl/md5.h>
#include <time.h>

#define HEAD 2*sizeof(int)
#define BUFSIZE 1024

/*
 * error - wrapper for perror
 */
void error(char *msg) {
  perror(msg);
  exit(1);
}

void md5(unsigned char *c,char *buf)
{
    int i,bytes;
    FILE *fp = fopen (buf, "rb");
    MD5_CTX mdContext;
    unsigned char data[1024];

    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, fp)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
    //for(i = 0; i < MD5_DIGEST_LENGTH; i++) printf("%02x", c[i]);
}

int main(int argc, char **argv) {

  int sockfd; /* socket file descriptor - an ID to uniquely identify a socket by the application program */
  int portno; /* port to listen on */
  int clientlen; /* byte size of client's address */
  struct sockaddr_in serveraddr; /* server's addr */
  struct sockaddr_in clientaddr; /* client addr */
  struct hostent *hostp; /* client host info */
  char buf[BUFSIZE]; /* message buf */
  char *hostaddrp; /* dotted decimal host addr string */
  int optval; /* flag value for setsockopt */
  int n,i; /* message byte size */
  unsigned char c[50];
  float dropProbability = atof(argv[2]);
  int dropCount=0;
  int dropArray[10000]={0};

  /*
   * check command line arguments
   */
  if (argc != 3){
    fprintf(stderr, "usage: %s <port_for_server> <dropProbability>\n", argv[0]);
    exit(1);
  }
  portno = atoi(argv[1]);

  /*
   * socket: create the socket
   */
  sockfd = socket(AF_INET, SOCK_DGRAM, 0);
 if (sockfd < 0)
    error("ERROR opening socket");

  /* setsockopt: Handy debugging trick that lets
   * us rerun the server immediately after we kill it;
   * otherwise we have to wait about 20 secs.
   * Eliminates "ERROR on binding: Address already in use" error.
   */
  optval = 1;
  setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const void *)&optval , sizeof(int));

  /*
   * build the server's Internet address
   */
  bzero((char *) &serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr.sin_port = htons((unsigned short)portno);

  /*
   * bind: associate the parent socket with a port
   */
  if (bind(sockfd, (struct sockaddr *) &serveraddr,
	   sizeof(serveraddr)) < 0)
    error("ERROR on binding");

  /*
   * main loop: wait for a datagram, then echo it
   */
  clientlen = sizeof(clientaddr);
  while (1) {

    /*
     * recvfrom: receive a UDP datagram from a client
     */

	bzero(buf, BUFSIZE);
    n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen);

     hostaddrp = inet_ntoa(clientaddr.sin_addr);
    if (hostaddrp == NULL)
      error("ERROR on inet_ntoa\n");
    printf("server received datagram from %s\n", hostaddrp);
    //printf("server received %d/%d bytes: %s\n", strlen(buf), n, buf);

    /*
     * sendto: echo the input back to the client
     */



	char buf2[200];
	printf("file name , size , number of chunks = %s\n",buf);
    if (n < 0)
      error("ERROR reading from socket");

/*
	creating new file
*/

	for(i=0;buf[i]!='\0';++i);
	for(;buf[i]!=' ';--i);
	int chunks=atoi(buf+i+1);
	for(--i;buf[i]!=' ';--i);
	int size=atoi(buf+i+1);
	buf[i]='\0';
	int file=open(buf,O_CREAT | O_RDWR,S_IRUSR | S_IWUSR);

//    dropCount=(int)(dropProbability*chunks);
    srand(time(0));
	   printf("#chunks %d\n",chunks );
//    for(i=0;i<dropCount;i++)
//    {
//        dropArray[rand()%chunks]=1;
//    }

	strcpy(buf2,buf);
    bzero(buf,BUFSIZE);
	strcpy(buf,"OK");
    n = sendto(sockfd, buf, strlen(buf)+1, 0, (struct sockaddr *) &clientaddr, clientlen);
    if (n < 0)
      error("ERROR writing to socket");

//receiving file
	printf("receiving file..\n");
int t=0;
	bzero(buf,BUFSIZE);
	for(i=0;i<chunks && t<size && (n = recvfrom(sockfd, buf, BUFSIZE, 0,(struct sockaddr *) &clientaddr, &clientlen))>0;)
	{
//printf("expchunk seq %d, got %d\n",i,((int*)buf)[0]);
		if((float)rand()/RAND_MAX<dropProbability)
		{
		    printf("Dropped Packet %d\n",i);
		}

		else if( ((int*)buf)[0]==i && ((int*)buf)[1]==n-HEAD)
		{
			n=write(file,buf+HEAD,n-HEAD);
			t+=n-HEAD;

			((int*)buf)[0]=i+1;
			n = sendto(sockfd, buf, sizeof(int), 0, (struct sockaddr *) &clientaddr, clientlen);
            //printf("Acknowledged chunk %d\n",i);
			i++;
		}
		else if(((int*)buf)[0]<i && ((int*)buf)[1]==n-HEAD)
		{
			((int*)buf)[0]=i;
			n = sendto(sockfd, buf, sizeof(int), 0, (struct sockaddr *) &clientaddr, clientlen);
		}


		bzero(buf,BUFSIZE);

	}
	printf("transfer complete\n");
	close(file);
    /*
	((int*)buf)[0]=i;((int*)buf)[1]==MD5_DIGEST_LENGTH;
	md5(c,buf2);
        for(i = 0; i < MD5_DIGEST_LENGTH; i++) (buf+HEAD)[i]=c[i];
	printf("file received\n");

// send back md5

	n = sendto(sockfd, buf, HEAD+MD5_DIGEST_LENGTH, 0, (struct sockaddr *) &clientaddr, clientlen);
    */

  }
}
