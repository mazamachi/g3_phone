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

#define N 8192

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
	if ( (fp_rec=popen("rec -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {
		die("popen:rec");
	}
	if ( (fp_play=popen("play -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null ","w")) ==NULL) {
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

	int re=0, r;
	while(1){
		// ssize_t m = fread_n(*fp_rec, n * sizeof(sample_t), rec_data);
		// 必ずNバイト読む
		re = 0;
		while(re<N){
			r=fread(rec_data+re,sizeof(sample_t),N/sizeof(sample_t)-re,fp_rec);
			if(r==-1) die("fread");
			if(r==0) break;
			re += r;
		}
		// n=fread(rec_data,sizeof(sample_t),N/sizeof(sample_t),fp_rec);
		// re = 0;
		// re = fread(rec_data,sizeof(sample_t), N-re, fp_rec);
		memset(rec_data+re,0,N-re);
		// 複素数の配列に変換
		sample_to_complex(rec_data, X, N);
		// /* FFT -> Y */
		fft(X, Y, N);

		// Yの一部を送る
		int i;
		for(i=0;i<send_len;i++){
			send_data[2*i]=(double)creal(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
			send_data[2*i+1]=(double)cimag(Y[cut_low*N/SAMPLING_FREQEUENCY+i]);
		}
		if(send_all(s,(char *)send_data,sizeof(double)*send_len*2)==-1){
			die("send");
		}

		memset(W,0.0+0.0*I,N*sizeof(complex double));
		memset(Z,0.0+0.0*I,N*sizeof(complex double));
		memset(rec_data,0,sizeof(long)*send_len*2);
		if(recv_all(s,(char *)recv_data,sizeof(double)*send_len*2)==-1){
			die("recv");
		}

		// printf("%d\n", cut_low*N/SAMPLING_FREQEUENCY);
		for(i=0; i<send_len; i++){
			W[cut_low*N/SAMPLING_FREQEUENCY+i]=(double)recv_data[2*i]+(double)recv_data[2*i+1]*I;
		}
		// /* IFFT -> Z */
		ifft(W, Z, N);

		// // 標本の配列に変換
		complex_to_sample(Z, play_data, N);
		// /* 標準出力へ出力 */
		// write_n(1, N, send_data);
		// write_n(1, N, play_data);

		fwrite(play_data,sizeof(sample_t),N/sizeof(sample_t),fp_play);
		// write(1,recv_data,n_recv);
	}
	close(s);
}