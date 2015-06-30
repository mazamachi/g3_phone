#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define N 256

void die(char *s){
	perror(s);
	exit(1);
}

int main(int argc, char *argv[]){
	int s = socket(PF_INET, SOCK_STREAM, 0);
	char *data;
	int n;
	data = malloc(sizeof(char)*N);
	struct sockaddr_in addr;
	addr.sin_family = AF_INET; // IPv4
	addr.sin_addr.s_addr = inet_addr(argv[1]); // IP address

	// if(inet_aton("192.168.100.70", &addr.sin_addr) == 0){ //IP address
	// 	die("inet_aton");
	// } 
	addr.sin_port = htons(atoi(argv[2])); // port number
	int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	// printf("%d\n", ret);
	if(ret == -1){ //connect
		die("connect");
	}
	printf("connected\n");
	while((n=recv(s,data,N,0)) != 0){
		// printf("%d\n", n);
		if(n==-1){
			die("recv");
		}
		write(1, data, n);
	}
	close(s);
}