#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "bandpass_fft.h"
#include "send_recv_all.h"
#include "print_array.h"

#include <time.h>

#define N 1024

// void die(char *s){
// 	perror(s);
// 	exit(1);
// }

// int fread_n(char *m, int size, int n, void *)


int main(int argc, char *argv[]){
	int n,m,n_recv;

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
	if ( (fp_rec=popen("rec --buffer 256 -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {
		die("popen:rec");
	}
	if ( (fp_play=popen("play --buffer 1024 -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null ","w")) ==NULL) {
		die("popen:play");
	}

	// sample_t *rec_data, *play_data;
	int cut_low=300, cut_high=5000;
	int send_len = (cut_high-cut_low)*N/SAMPLING_FREQEUENCY;
	sample_t * rec_data = malloc(sizeof(sample_t)*N);
	double * send_data = malloc(sizeof(double)*send_len*2);
	double * recv_data = malloc(sizeof(double)*send_len*2);
	sample_t * play_data = malloc(sizeof(sample_t)*N);
	complex double * X = calloc(sizeof(complex double), N);
	complex double * Y = calloc(sizeof(complex double), N);
	complex double * Z = calloc(sizeof(complex double), N);
	complex double * W = calloc(sizeof(complex double), N);

	sample_t re=0, r;
	// time_t pre,now,mem;
	clock_t passage;
	double now,pre;
	passage = clock();
	now = (double)passage / CLOCKS_PER_SEC;
	int i;
	int counter=0;
	while(1){
		// 必ずNバイト読む
		re = 0;
		while(re<N){
			r=fread(rec_data+re,sizeof(sample_t),N-re,fp_rec);
			if(r==-1) die("fread");
			if(r==0) break;
			re += r;
		}
		memset(rec_data+re,0,N-re);
		// print_array(rec_data,N);


		// 複素数の配列に変換
		sample_to_complex(rec_data, X, N);
		// /* FFT -> Y */
		fft(X, Y, N);
		// Yの一部を送る

		for(i=0;i<send_len;i++){
			send_data[2*i]=(double)creal(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
			send_data[2*i+1]=(double)cimag(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
		}
		// print_array_double(send_data,send_len*2);
		if(send_all(s,(char *)send_data,sizeof(double)*send_len*2)==-1){
			die("send");
		}

		memset(W,0.0+0.0*I,N*sizeof(complex double));
		memset(Z,0.0+0.0*I,N*sizeof(complex double));
		memset(rec_data,0,sizeof(long)*send_len*2);
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

		// 無音状態だったらスキップ
		int num_low=0;
		for(i=0;i<N;i++){
			if(-10<play_data[i] && play_data[i]<10)
				num_low++;
		}
		if(num_low>80*N/100||++counter%50==0)
			continue;
		// /* 標準出力へ出力 */
		fwrite(play_data,sizeof(sample_t),N,fp_play);
	}
	close(s);
}
