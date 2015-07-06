#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <string.h>
#include <pthread.h>

#include "bandpass_fft.h"
#include "send_recv_all.h"
#include "print_array.h"

#include <time.h>

#define N 1024
#define BUF_SIZE 256


// void die(char *s){
// 	perror(s);
// 	exit(1);
// }

// int fread_n(char *m, int size, int n, void *)
void phone_recsend(void *vs){
	int* ps = (int *)vs;
	int s = *ps;
	FILE *fp_rec;
	if ( (fp_rec=popen("rec -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {
		die("popen:rec");
	}
	int cut_low=300, cut_high=5000;
	int send_len = (cut_high-cut_low)*N/SAMPLING_FREQEUENCY;
	sample_t * rec_data = malloc(sizeof(sample_t)*N);
	double * window_data = malloc(sizeof(double)*N);
	double * send_data = malloc(sizeof(double)*send_len*2);
	complex double * X = calloc(sizeof(complex double), N);
	complex double * Y = calloc(sizeof(complex double), N);
	sample_t re=0, r;
	int i;
	memset(rec_data,0,N);
	while(1){
		printf("phone loop\n");
		// 必ずNバイト読む
		re = 0;
		// オーバーラップ
		while(re<N/2){
			r=fread(rec_data+N/2,sizeof(sample_t),N/2-re,fp_rec);
			if(r==-1) die("fread");
			if(r==0) break;
			re += r;
		}
		memset(rec_data+N/2+re,0,N/2-re);


		// 窓関数(ハミング窓)
		for (i = 0; i < N; ++i)	{
			window_data[i] = (0.54-0.46*cos(2*M_PI*i/(double)(N-1)))*rec_data[i];
		}
		memcpy(rec_data,rec_data+N/2,sizeof(sample_t)*N/2);

		// 複素数の配列に変換
		sample_to_complex_double(window_data, X, N);
		// /* FFT -> Y */
		fft(X, Y, N);
		// Yの一部を送る

		for(i=0;i<send_len;i++){
			send_data[2*i]=(double)creal(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
			send_data[2*i+1]=(double)cimag(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
		}
		// printf("s in phone: %d\n", s);
		if(send_all(s,(char *)send_data,sizeof(double)*send_len*2)==-1){
			die("send");
		}
	}
}
void phone_recvplay(void *vs){
	int* ps = (int *)vs;
	int s = *ps;
	int i;
	FILE *fp_play;
	if ( (fp_play=popen("play -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null ","w")) ==NULL) {
		die("popen:play");
	}
	int cut_low=300, cut_high=5000;
	int send_len = (cut_high-cut_low)*N/SAMPLING_FREQEUENCY;
	double * recv_data = malloc(sizeof(double)*send_len*2);
	sample_t * play_data = malloc(sizeof(sample_t)*N);
	sample_t * pre_data = malloc(sizeof(sample_t)*N/2);
	complex double * X = calloc(sizeof(complex double), N);
	complex double * Y = calloc(sizeof(complex double), N);
	complex double * Z = calloc(sizeof(complex double), N);
	complex double * W = calloc(sizeof(complex double), N);
	memset(pre_data,0,N);
	while(1){
		memset(W,0.0+0.0*I,N*sizeof(complex double));
		memset(Z,0.0+0.0*I,N*sizeof(complex double));
		// memset(rec_data,0,sizeof(long)*send_len*2);
		if(recv_all(s,(char *)recv_data,sizeof(double)*send_len*2)==-1){
			die("recv");
		}
		for(i=0; i<send_len; i++){
			W[cut_low*N/SAMPLING_FREQEUENCY+i]=(double)recv_data[2*i]+(double)recv_data[2*i+1]*I;
		}
		// /* IFFT -> Z */
		ifft(W, Z, N);

		// // 標本の配列に変換
		complex_to_sample(Z, play_data, N);
				// オーバーラップを戻す
		for(i=0;i<N/2;i++){
			play_data[i] += pre_data[i];
		}
		memcpy(pre_data,play_data+N/2,N/2);
		// 無音状態だったらスキップ
		int num_low=0;
		for(i=0;i<N;i++){
			if(-10<play_data[i] && play_data[i]<10)
				num_low++;
		}
		if(num_low>80*N/100)
			continue;
		// /* 標準出力へ出力 */
		// write(1,play_data,N/2);
		fwrite(play_data,sizeof(sample_t),N/2,fp_play);
	}
}
void phone(void *vs){
	int* ps = (int *)vs;
	int s = *ps;
	FILE *fp_rec;
	FILE *fp_play;
	if ( (fp_rec=popen("rec -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {
		die("popen:rec");
	}
	if ( (fp_play=popen("play -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null ","w")) ==NULL) {
		die("popen:play");
	}

	int cut_low=300, cut_high=5000;
	int send_len = (cut_high-cut_low)*N/SAMPLING_FREQEUENCY;
	sample_t * rec_data = malloc(sizeof(sample_t)*N);
	double * window_data = malloc(sizeof(double)*N);
	double * send_data = malloc(sizeof(double)*send_len*2);
	double * recv_data = malloc(sizeof(double)*send_len*2);
	sample_t * play_data = malloc(sizeof(sample_t)*N);
	sample_t * pre_data = malloc(sizeof(sample_t)*N/2);
	complex double * X = calloc(sizeof(complex double), N);
	complex double * Y = calloc(sizeof(complex double), N);
	complex double * Z = calloc(sizeof(complex double), N);
	complex double * W = calloc(sizeof(complex double), N);

	sample_t re=0, r;
	// time_t pre,now,mem;
	int i;
	memset(rec_data,0,N);
	memset(pre_data,0,N);
	// printf("phone\n");
	while(1){
		// printf("phone loop\n");
		// 必ずNバイト読む
		re = 0;
		// オーバーラップ
		while(re<N/2){
			r=fread(rec_data+N/2,sizeof(sample_t),N/2-re,fp_rec);
			if(r==-1) die("fread");
			if(r==0) break;
			re += r;
		}
		memset(rec_data+N/2+re,0,N/2-re);


		// 窓関数(ハミング窓)
		for (i = 0; i < N; ++i)	{
			window_data[i] = (0.54-0.46*cos(2*M_PI*i/(double)(N-1)))*rec_data[i];
		}
		memcpy(rec_data,rec_data+N/2,sizeof(sample_t)*N/2);

		// 複素数の配列に変換
		sample_to_complex_double(window_data, X, N);
		// /* FFT -> Y */
		fft(X, Y, N);
		// Yの一部を送る
		for(i=0;i<send_len;i++){
			send_data[2*i]=(double)creal(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
			send_data[2*i+1]=(double)cimag(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
		}
		// printf("s in phone: %d\n", s);
		if(send_all(s,(char *)send_data,sizeof(double)*send_len*2)==-1){
			die("send");
		}

		memset(W,0.0+0.0*I,N*sizeof(complex double));
		memset(Z,0.0+0.0*I,N*sizeof(complex double));
		// memset(rec_data,0,sizeof(long)*send_len*2);
		if(recv_all(s,(char *)recv_data,sizeof(double)*send_len*2)==-1){
			die("recv");
		}
		for(i=0; i<send_len; i++){
			W[cut_low*N/SAMPLING_FREQEUENCY+i]=(double)recv_data[2*i]+(double)recv_data[2*i+1]*I;
		}
		// /* IFFT -> Z */
		ifft(W, Z, N);

		// // 標本の配列に変換
		complex_to_sample(Z, play_data, N);
				// オーバーラップを戻す
		for(i=0;i<N/2;i++){
			play_data[i] += pre_data[i];
		}
		memcpy(pre_data,play_data+N/2,N/2);
		// 無音状態だったらスキップ
		int num_low=0;
		for(i=0;i<N;i++){
			if(-10<play_data[i] && play_data[i]<10)
				num_low++;
		}
		if(num_low>80*N/100)
			continue;
		// /* 標準出力へ出力 */
		// write(1,play_data,N/2);
		fwrite(play_data,sizeof(sample_t),N/2,fp_play);
	}
}
void chat_send(void *vs){
	int *ps = (int *)vs;
	int s = *ps;
	ssize_t n;
	char chat[BUF_SIZE];

	while(1){
		gets(chat);
		if(send(s, chat, strlen(chat),0) == -1) die("send");
	}
}
void chat_recv(void *vs){
	int *ps = (int *)vs;
	int s = *ps;
	int n;
	char chat[BUF_SIZE];
	printf("chat_recv %d\n", s);
	while(1){
		n = recv(s,chat,BUF_SIZE,0);
		if(n==0) break;
		chat[n]='\n';
		if(write(1, chat, n+1) == -1) die("write");
	}
}

void *server(void *vport){
	int *pport = (int*)vport;
	int port = *pport;
	struct sockaddr_in client_addr;
	struct sockaddr_in addr;
	int ss = socket(PF_INET, SOCK_STREAM, 0);
	printf("port:%d\n",port);
	addr.sin_family = AF_INET; // IPv4
	addr.sin_port = htons(port); // port number to wait on
	addr.sin_addr.s_addr = INADDR_ANY; // any IP address can be connected
	printf("%d\n", ss);
	if(bind(ss, (struct sockaddr *)&addr, sizeof(addr)) == -1){
		die("bind");
	}
	printf("listenning\n");
	if(listen(ss,10) == -1){
		die("listen");
	}
	socklen_t len = sizeof(struct sockaddr_in);
	printf("%p\n", &client_addr);
	int s = accept(ss, (struct sockaddr *)&client_addr, &len);

	if(s==-1){
		die("accept");
	}
	if(close(ss)==-1){
		die("close");
	}
	void *p = malloc(sizeof(int));
	*(int*)(p) = s;
	pthread_exit(p);
	// return s;
}
int client(int port, char* ip){
	struct sockaddr_in addr;
	int s = socket(PF_INET, SOCK_STREAM, 0);
	addr.sin_family = AF_INET; // IPv4
	addr.sin_addr.s_addr = inet_addr(ip); // IP address
	printf("%d\n", port);
	addr.sin_port = htons(port); // port number
	int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));
	if(ret == -1){ //connect
		die("connect");
	}

	return s;
}

int main(int argc, char *argv[]){
	int n,m,n_recv;

	int s,sc;
	int port1 = atoi(argv[1]);
	int port2 = port1+100;
	struct sockaddr_in addr; // addres information for bin
	void *status1,*status2;

	if(argc==2){
		// server
		pthread_t thread_s_phone, thread_s_chat;
		if(pthread_create(&thread_s_phone,NULL,(void *)server,(void *) & port1)!=0) die("phone_create");
		if(pthread_create(&thread_s_chat,NULL,(void *)server,(void *) & port2)!=0) die("chat_create");
		int ss,ssc;
		if(pthread_join(thread_s_phone,&status1)!=0) die("join phone");
		if(pthread_join(thread_s_chat, &status2)!=0) die("join chat");
		s = *(int *)status1;
		sc = *(int *)status2;
		// ss = ssc = socket(PF_INET, SOCK_STREAM, 0);
		// s = server(port);
		// sc = server(port+100);
	}else if(argc==3){
		// client
		s = client(port1,argv[2]);
		printf("phone success\n");
		sc = client(port2,argv[2]);
	}

	printf("%d,%d\n", s,sc);
	pthread_t thread_recsend, thread_recvplay, thread2, thread3;
	int ret0,ret1,ret2,ret3;
	// int s_copy =s;
	// printf("s:%d\n", s);
	ret0 = pthread_create(&thread_recsend,NULL,(void *)phone_recsend,(void *) & s);
	ret1 = pthread_create(&thread_recvplay,NULL,(void *)phone_recvplay,(void *)&s);
	ret2 = pthread_create(&thread2,NULL,(void *)chat_send,(void *) & sc);
	ret3 = pthread_create(&thread3,NULL,(void *)chat_recv,(void *) & sc);	

	if (ret0 != 0) {
			die("thread0 create");
	}
	if (ret1 != 0) {
			die("thread1 create");
	}
	if (ret2 != 0) {
			die("thread2 create");
	}
	if (ret3 != 0) {
			die("thread3 create");
	}

	ret0 = pthread_join(thread_recsend,NULL);
	if (ret1 != 0) {
			die("cannot join thread 0");
	}

	ret1 = pthread_join(thread_recvplay,NULL);
	if (ret1 != 0) {
			die("cannot join thread 1");
	}

	ret2 = pthread_join(thread2,NULL);
	if (ret2 != 0) {
			die("cannot join thrad 2");
	}

	ret3 = pthread_join(thread3,NULL);
	if (ret3 != 0) {
			die("cannot join thrad 3");
	}


	close(s);
}
