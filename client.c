/*
 * udpclient.c - A simple UDP client
 * usage: udpclient <host> <port>
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include <fcntl.h>
#include <openssl/md5.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/time.h>

#define BUFSIZE 1024
# define filebufsize 1024000
# define HEAD 2*sizeof(int)
/*
 * error - wrapper for perror
 */

 static int alarm_fired = 0;

 void mysig(int sig)
 {
     //printf("Timeout\n");
     //if(alarm_fired==1)return;
     if (sig == SIGALRM)
     {
         printf("Timeout\n");
         alarm_fired = 1;
     }

 }


void error(char *msg) {
    perror(msg);
    exit(0);
}

void md5(unsigned char *c,char *buf)
{
    int i;
    FILE *fr = fopen (buf, "rb");
    MD5_CTX mdContext;
    int bytes;
    unsigned char data[1024];
    MD5_Init (&mdContext);
    while ((bytes = fread (data, 1, 1024, fr)) != 0)
        MD5_Update (&mdContext, data, bytes);
    MD5_Final (c,&mdContext);
}


int main(int argc, char **argv) {
    int sockfd, portno,n;
    int serverlen;
    struct sockaddr_in serveraddr;
    struct hostent *server;
    char *hostname;
    char buf[BUFSIZE*1000],buf1[BUFSIZE];
    struct timeval tv;
	struct itimerval Time;
    int i,m;
    unsigned char c[50];
    /* check command line arguments */
    if (argc != 3) {
       fprintf(stderr,"usage: %s <hostname> <port>\n", argv[0]);
       exit(0);
    }
    hostname = argv[1];
    portno = atoi(argv[2]);

    /* socket: create the socket */
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");

    /* gethostbyname: get the server's DNS entry */
    server = gethostbyname(hostname);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host as %s\n", hostname);
        exit(0);
    }

    /* build the server's Internet address */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)server->h_addr,
	  (char *)&serveraddr.sin_addr.s_addr, server->h_length);
    serveraddr.sin_port = htons(portno);

    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, (char*)&tv, sizeof(struct timeval));


    /* get file from user */
	serverlen = sizeof(serveraddr);
    bzero(buf, BUFSIZE);
    puts("Enter fileName you want to transfer");
    fgets(buf, BUFSIZE, stdin);
	//scanf("%s",buf);
	printf("%s\n",buf);
    for(i=0;buf[i]!='\n' && buf[i]!='\0';++i);
    buf[i]='\0';

    /* finding size of file  */
	FILE *fp=fopen(buf,"rb");
	fseek(fp, 0L, SEEK_END);
	int x= ftell(fp);
	fclose(fp);

    // y is #chunks
	int y=(x%(BUFSIZE-HEAD)?x/(BUFSIZE-HEAD)+1:x/(BUFSIZE-HEAD));

    /* opening file for transfer */
	char buf2[200];
	strcpy(buf2,buf);
	int fr=open(buf,O_RDONLY);
	buf[i]=' ';
	sprintf(buf+i+1,"%d",x);
	for(;buf[i]!='\n' && buf[i]!='\0';++i);
	buf[i]=' ';++i;
	sprintf(buf+i,"%d",y);
		printf("fileName, size, #chunks are -> %s\n",buf);   // buf contains -> [fileName<space>size<space>chunks]

        printf("Requesting Server to receive data : ");

	while(1){
    	   n = sendto(sockfd, buf, strlen(buf), 0, (struct sockaddr*)&serveraddr, serverlen);
    	   printf("%d\n",n);
        	n = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&serveraddr, &serverlen);
    	if(n>0)break;
	}

	printf("server says : %s\ntransfering file...\n",buf);
	bzero(buf,BUFSIZE);

    /* transfering file */
    int w=3;
    int last_chunk=0;
    int pos=0;
    int maxAck=0;
    int lastSize=0;
    int chunksLoaded=0;
    int j=0;
	//printf("seeker=%d, x=%d",lseek(fr,0,SEEK_CUR),x);

    printf("#chunks=%d\n",y);
    printf("\nwindow size= %d\n",w);
    
    while(maxAck<y)
    {


        while((last_chunk-maxAck)<w)
        {
            if(maxAck>=y || last_chunk>=y)break;
            if(last_chunk<chunksLoaded) n=((int*)(buf+pos))[1];
            else
            {
                bzero(buf+pos,BUFSIZE);
                n=read(fr,buf+pos+HEAD,BUFSIZE-HEAD);
                ((int*)(buf+pos))[0]=last_chunk;
                ((int*)(buf+pos))[1]=n;
                chunksLoaded++;
            }
            sendto(sockfd, buf+pos, n+HEAD, 0, (struct sockaddr*)&serveraddr, serverlen);
            //printf("last_chunk %d\n",last_chunk);
            last_chunk++; lastSize=n; pos=(pos+BUFSIZE+filebufsize)%filebufsize;
        }
        //printf("Combined Chunks Sent upto seq no = %d\n",last_chunk-1);

	bzero(buf1,BUFSIZE);

        alarm_fired=0;
        //alarm(1);
    	Time.it_value.tv_usec=50000;
    	Time.it_value.tv_sec=0;
    	setitimer(ITIMER_REAL,&Time,NULL);
        alarm_fired=0;
        (void) signal(SIGALRM, mysig);

	while(recvfrom(sockfd, buf1, sizeof(int), MSG_DONTWAIT, (struct sockaddr*)&serveraddr, &serverlen)>0 || !alarm_fired)
    	{
            i=((int*)buf1)[0];
    	    if(i>maxAck)
            {

		j=i-maxAck;
                maxAck=i;
		if(w+j>1000)j=1000-w;
		w=w+j;

                printf("updating window %d\n",w);
		j=2*j;
		while(j--)
		{                
		    if(last_chunk>=y)break;
                
                    if(last_chunk<chunksLoaded) n=((int*)(buf+pos))[1];
                    else
                    {
                        bzero(buf+pos,BUFSIZE);
                        n=read(fr,buf+pos+HEAD,BUFSIZE-HEAD);
                        ((int*)(buf+pos))[0]=last_chunk;
                        ((int*)(buf+pos))[1]=n;
                        chunksLoaded++;
                    }
                    sendto(sockfd, buf+pos, n+HEAD, 0, (struct sockaddr*)&serveraddr, serverlen);
                    //printf("last_chunk %d\n",last_chunk);
                    last_chunk++; lastSize=n; pos=(pos+BUFSIZE+filebufsize)%filebufsize;
                }

		alarm_fired=0;
		//alarm(1);
	    	Time.it_value.tv_usec=50000;
	    	Time.it_value.tv_sec=0;
	    	setitimer(ITIMER_REAL,&Time,NULL);
		alarm_fired=0;
		(void) signal(SIGALRM, mysig);


            }
    	    bzero(buf1,BUFSIZE);
            if(maxAck==last_chunk)break;
	}

    	Time.it_value.tv_usec=0;
    	Time.it_value.tv_sec=0;
    	setitimer(ITIMER_REAL,&Time,NULL);

    	//printf("maxAck received = %d, last_chunk=%d, chunksLoaded=%d\n",maxAck,last_chunk,chunksLoaded);

        pos=((maxAck*BUFSIZE)+filebufsize)%filebufsize;
        w/=2;
        if(w<2)w=2;
	if(w>1000)w=1000;
        last_chunk=maxAck;
	printf("updating window %d\n",w);

    }

	printf("Last Ack was %d\n",maxAck);

    /*
    int i;
    for(i=0;(n=read(fr,buf+HEAD,BUFSIZE-HEAD))>0;++i)
	{
		((int*)buf)[0]=i;
		((int*)buf)[1]=n;

		while(1){
    		sendto(sockfd, buf, n+HEAD, 0, &serveraddr, serverlen);
    		m = recvfrom(sockfd, buf1, BUFSIZE, 0, &serveraddr, &serverlen);
    		if(m>0 && ((int*)buf1)[0]==i)break;

		}
		bzero(buf,BUFSIZE);
	}
    m = recvfrom(sockfd, buf1, BUFSIZE, 0, &serveraddr, &serverlen);
    if(m>0 && ((int*)buf1)[0]==i)break;use next set
    */

	printf("\ntransfer complete\n");
/*
	md5(c,buf2);
	while(1){
	m = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr*)&serveraddr, &serverlen);
	if(m>0 && ((int*)buf)[0]==i)
	{
    /* checking md5 */
/*		if(!strncmp(buf+HEAD,(char *)c,MD5_DIGEST_LENGTH))
		{
			printf("transfer complete md5 matched\n");
			break;
		}
		else {
			printf("transfer error md5 not matched\n");
			break;
		}
	}

	}
*/

    /* send the message to the server
    serverlen = sizeof(serveraddr);
    n = sendto(sockfd, buf, strlen(buf), 0, &serveraddr, serverlen);
    if (n < 0)
      error("ERROR in sendto");

    /* print the server's reply
    n = recvfrom(sockfd, buf, strlen(buf), 0, &serveraddr, &serverlen);
    if (n < 0)
      error("ERROR in recvfrom");
    printf("Echo from server: %s", buf);*/
    return 0;
}
