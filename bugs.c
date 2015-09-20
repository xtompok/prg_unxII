#define _XOPEN_SOURCE 700
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <unistd.h>
#include <netinet/in.h>
#include <pthread.h>

char buf[128];
int wpos;
int rpos;
pthread_cond_t cv;
pthread_mutex_t mtx;

void * printer(void * data){
	pthread_cond_wait(&cv,&mtx);
	while (1){
		write(1,buf+wpos+1,buf[wpos]);
		wpos += 16;
		wpos %= 128;
		pthread_cond_wait(&cv,&mtx);
	}
}

void * reader(void * data){
	struct addrinfo hints;
	memset(&hints,0,sizeof(struct addrinfo));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;
	int res;
	struct addrinfo * addrinfo;
	res = getaddrinfo(NULL,"8888",&hints,&addrinfo);
	if (res){
		printf("Error when resolving: %s\n",gai_strerror(res));
		return NULL;
	}
	int listenfd = -1;
	while (addrinfo != NULL){
		listenfd = socket(addrinfo->ai_family,
			addrinfo->ai_socktype,
			addrinfo->ai_protocol);
		if (listenfd == -1)
			continue;
		if (bind(listenfd,addrinfo->ai_addr,addrinfo->ai_addrlen) == 0)
			break;
		close(listenfd);
		listenfd = -1;
		addrinfo = addrinfo->ai_next;
	}
	if (listenfd == -1){
		printf("Bind failed\n");
		return NULL;
	}
	freeaddrinfo(addrinfo); 
	int readfd;	
	if (listen(listenfd,1)==-1){
		printf("Listen failed\n");
		return NULL;
	}
	readfd = accept(listenfd,NULL,NULL);
	if (readfd == -1)
		return NULL;
	int len;	
	char tmp[16];
	while ((len = read(readfd,tmp,15))){ 
		char chlen;
		chlen = len;
		pthread_mutex_lock(&mtx);
		buf[rpos]=chlen;
		memcpy(buf+rpos+1,tmp,len);
		rpos+=16;
		rpos%=128;
		pthread_cond_signal(&cv);
		pthread_mutex_unlock(&mtx);
	}
	close(readfd);
	close(listenfd);
	return NULL;
}


int main (int argc, char ** argv){
	pthread_mutex_init(&mtx,NULL);
	pthread_cond_init(&cv,NULL);	
	wpos = 0;
	rpos = 0;
	pthread_t read_thr;
	pthread_t print_thr;
	pthread_create(&read_thr,NULL,reader,NULL);
	pthread_create(&print_thr,NULL,printer,NULL);
	pthread_join(read_thr,NULL);
	pthread_cancel(print_thr);
	return 0;
}
