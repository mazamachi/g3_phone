int send_all(int sock,char *buf,int len){
	int sent = 0;
	while(sent < len){
		int n = send(sock,buf + sent, len-sent,0);
		if(n == -1){
			perror("cant send\n");
			exit(1);
		}else if(n == 0){
			fprintf(stderr,"EOF\n");
			return sent;
		}
		sent += n;
	}

	return sent;
}

int recv_all(int sock,char *buf,int len){
	int received  = 0;
	while(received < len){
		int n = recv(sock, buf + received,len - received,0);
		if(n==-1){
			perror("cant recv\n");
			exit(1);
		}else if(n==0){
			fprintf(stderr,"EOF\n");
			return received;
		}
		received += n;
	}
	return received;
}