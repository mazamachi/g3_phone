#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "g123_phone.h"

#define N 100000

void die(char *s){
	perror(s);
	exit(1);
}

int main(int argc, char *argv[]){
	int n,m,n_recv;
	char *send_data, *recv_data;
	send_data = malloc(sizeof(char)*N);
	recv_data = malloc(sizeof(char)*N);

	int s,ss;
	struct sockaddr_in addr; // addres information for bin
	struct sockaddr_in client_addr;
	if(argc==2){
		// server
		ss = socket(PF_INET, SOCK_STREAM, 0);

		addr.sin_family = AF_INET; // IPv4
		addr.sin_port = htons(atoi(argv[1])); // port number to wait on
		addr.sin_addr.s_addr = INADDR_ANY; // any IP address can be connected
		if(bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1){
			die("bind");
		}

		if(listen(ss,10) == -1){
			die("listen");
		}

		socklen_t len = sizeof(struct sockaddr_in);
		s = accept(ss, (struct sockaddr *)&client_addr, &len);

		if(s==-1){
			die("accept");
		}
		if(close(ss)==-1){
			die("close");
		}
	}else if(argc==3){
		// client
		s = socket(PF_INET, SOCK_STREAM, 0);
		addr.sin_family = AF_INET; // IPv4
		addr.sin_addr.s_addr = inet_addr(argv[1]); // IP address
		addr.sin_port = htons(atoi(argv[2])); // port number
		int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
		if(ret == -1){ //connect
			die("connect");
		}
	}

	FILE *fp_rec;
	FILE *fp_play;
	if ( (fp_rec=popen("rec -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {
		die("popen:rec");
	}
	if ( (fp_play=popen("play -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","w")) ==NULL) {
		die("popen:play");
	}

	while(1){
		n=fread(send_data,sizeof(char),N,fp_rec);
		if(send(s,send_data,n,0)==-1){
			die("send");
		}
		if((n_recv=recv(s,recv_data,N,0))==-1){
			die("recv");
		}
		fwrite(recv_data,sizeof(char),n_recv,fp_play);
		// write(1,recv_data,n_recv);
	}
	close(s);
}