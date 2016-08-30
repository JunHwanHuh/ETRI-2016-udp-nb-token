#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include "mtime.h"
#include "tbucket.h"
#include "parameter.h"

int fd2;
void *buffer_monitor(void *data)
{	
	
	int tot, unsent, free, ilen;
	ilen = sizeof (int);
	unsent=0;
	getsockopt (fd2, SOL_SOCKET, SO_SNDBUF, &tot, &ilen); // Send Buffer총용양 조사
	printf("buffer size ? %d\n", tot);
	while(1)
	{
		sleep(1);	
		ioctl (fd2, TIOCOUTQ, &unsent); // Send Buffer에 쌓여있는 양 조사
		free = tot - unsent; // Send Free Buffer 양 계산
		//printf("empty size : %d\n", free);
	}
}


int main(int argc, char **argv)
{

//---------------------------------------------------------------------------------//
/*-----------------------declaration, initialization, connection-------------------*/
//---------------------------------------------------------------------------------//

	struct sockaddr_in myaddr, remaddr;
	int fd, fd1, readcnt, i=1, slen=sizeof(remaddr), bsize, rn, tot;
	int s_count=0,f_count=0, t_count=0;
	long e_stime, e_etime, e_dtime, s_rate;
	char d_buf[DBUFLEN], i_buf[IBUFLEN];
	char *i_bufp = i_buf, *server = "127.0.0.1";
	qtokenbucket_t bucket;
	time_t st_stamp;

	/* tokenBuket Initiallization */
	qtokenbucket_init(&bucket, INIT_TOKEN, MAX_TOKEN, TOKEN_RATES);

	/* open Data file */
	if((fd = open(argv[1], O_RDONLY)) == -1) {
		perror("open failed");
		exit(1);
	}

	/* create Info file */
	if((fd1 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644)) == -1) {
		perror("create failed");
		exit(1);
	}

	/* create a socket */
	if ((fd2 = socket(AF_INET, SOCK_DGRAM, 0)) == -1)
		printf("socket created\n");

	/* manipulate udp socket buffer size */
	bsize = SOCKET_BUFFER_SIZE;
	rn = sizeof(int);
	//getsockopt(fd2, SOL_SOCKET, SO_SNDBUF, &bsize, (socklen_t *)&rn);
	//printf("Send buf size : %d\n", bsize);
	setsockopt(fd2, SOL_SOCKET, SO_SNDBUF, &bsize, (socklen_t)rn);
	getsockopt(fd2, SOL_SOCKET, SO_SNDBUF, &bsize, (socklen_t *)&rn);
	//printf("Send buf size : %d\n", bsize);

	/* bind it to all local addresses and pick any port number */

	memset((char *)&myaddr, 0, sizeof(myaddr));
	myaddr.sin_family = AF_INET;
	myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	myaddr.sin_port = htons(0);

	if (bind(fd2, (struct sockaddr *)&myaddr, sizeof(myaddr)) < 0) {
		perror("bind failed");
		return 0;
	}       

	/* now define remaddr, the address to whom we want to send messages */
	/* For convenience, the host address is expressed as a numeric IP address */
	/* that we will convert to a binary format via inet_aton */

	memset((char *) &remaddr, 0, sizeof(remaddr));
	remaddr.sin_family = AF_INET;
	remaddr.sin_port = htons(SERVICE_PORT);
	remaddr.sin_addr.s_addr = inet_addr("127.0.0.1");

	if (inet_aton(server, &remaddr.sin_addr)==0) {
		fprintf(stderr, "inet_aton() failed\n");
		exit(1);
	}

	/* output file manipulation */
	sprintf(i_bufp, "\n");
	if( write(fd1, i_bufp, sizeof(i_buf)) == -1 )
	{
		printf("write error\n");
		exit(0);
	}

	sprintf(i_bufp, "\n\n index  success_count   fail_count     send_time_stamp\n");
	if( write(fd1, i_bufp, sizeof(i_buf)) == -1 )
	{
		printf("write error\n");
		exit(0);
	}


	/* create Token Bucket Thread */
	int thr_id;
	pthread_t p_thread;
	thr_id = pthread_create(&p_thread, NULL, buffer_monitor, NULL);
	if (thr_id<0)
	{
		perror("thread create error: ");
	}
//---------------------------------------------------------------------------------//
/*-----------------------------experiment description------------------------------*/
//---------------------------------------------------------------------------------//
	e_stime = GetTimeStamp();
	memset(d_buf, 0x00, DBUFLEN);
	memset(i_buf, 0x00, DBUFLEN);

	/* start */
	while (1) {
		if (qtokenbucket_consume(&bucket, 1) == false) {
			// Bucket is empty. Let's wait
			usleep(qtokenbucket_waittime(&bucket, 1) * 1000);
			continue;
		}

		if((readcnt = read(fd, d_buf, DBUFLEN))==0){
			printf("read complete\n");
			break;
		}
		else if(readcnt < 0){
			close(fd);
			close(fd1);
			printf("read error exit\n");
			exit(0);
		}

		if (sendto(fd2, d_buf, sizeof(d_buf), 0, (struct sockaddr *)&remaddr, slen)==-1){
	      	++f_count;
			perror("sendto");
	    }
	    else
	      	++s_count;
	   

      	if((f_count + s_count) == (1024)){
      		t_count = i;
      		//printf("send 1Mbytes with index %d, s_count-%d, f_count-%d\n", i, s_count, f_count);
      		//write outputfile after send message
      		sprintf(i_bufp, "   %d         %d            %d        %lld \n", i++, s_count, f_count, GetTimeStamp());

      		if( write(fd1, i_buf, sizeof(i_buf)) == -1 )
			{
		    	printf("write error\n");
		    	exit(0);
			}
      		f_count = 0;
      		s_count = 0;		
      	}
    }
    /* end */

    e_etime = GetTimeStamp();
    e_dtime = ((e_etime - e_stime) / 1000000);
    s_rate = 1024 / e_dtime;

    lseek(fd1, 0, SEEK_SET);
    sprintf(i_bufp, "\n<parameter setting>\nsocket buffer size : %d bytes\nsend packet size : %d bytes\n\n<token bucket setting>\ninit_token : %d\nmax_token : %d\ntoken_rate : %d (token/s)\n\n<experiment result>\nmode : udp token-bucket blocking (if socket buffer is full)\nsend file size : 1GB\nsocket buffer size : %d\nexperiments start time : %ld (micro second)\nexperiments end time : %ld (micro second)\nexperiments duration : %lds\ndata send rate : %ld MB/s \ntotal index(1MB per index) : %d\n",SOCKET_BUFFER_SIZE, DBUFLEN, INIT_TOKEN, MAX_TOKEN, TOKEN_RATES, bsize, e_stime, e_etime, e_dtime, s_rate, t_count);

	if( write(fd1, i_buf, sizeof(i_buf)) == -1 )
	{
		printf("write error\n");
		exit(0);
	}
//---------------------------------------------------------------------------------//
/*-----------------------------deallocation, termination---------------------------*/
//---------------------------------------------------------------------------------//
	
	close(fd);
	close(fd1);
	close(fd2);

	return 0;
}

