#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/time.h>
#include "bandpass_fft.h"
#include <pthread.h>
#include "send_recv_all.h"

#define N 4096

struct timeval start_fread, start_send, start_recv, start_fwrite, 
  stop_fread, stop_send, stop_recv, stop_fwrite, loop_start, loop_stop;

const char IDCHK[] = "aktkb46";
#define IDLEN 8

int sock;
struct sockaddr_in remote, host;

void phone_recsend(void *vs){
  int* ps = (int *)vs;
  int s = *ps;
  FILE *fp_rec;
  if ( (fp_rec=popen("rec --buffer 32 -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {
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
    for (i = 0; i < N; ++i) {
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
    memset(send_data,0,sizeof(double)*send_len*2);
  }
}
void phone_recvplay(void *vs){
  int* ps = (int *)vs;
  int s = *ps;
  int i;
  FILE *fp_play;
  if ( (fp_play=popen("play --buffer 32 -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","w")) ==NULL) {
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
    // if(num_low>80*N/100)
    //   continue;
    // /* 標準出力へ出力 */
    // write(1,play_data,N/2);
    fwrite(play_data,sizeof(sample_t),N/2,fp_play);
    memset(play_data,0,sizeof(sample_t)*N);
    memset(recv_data,0,sizeof(double)*send_len*2);
  }
}
//sends a buffer of length len
// int send_all(int sock,char *buf,int len){
//   int sent = 0;
//   while(sent < len){
//     int n = send(sock,buf + sent, len-sent,0);
//     if(n == -1){
//       perror("cant send\n");
//       exit(1);
//     }else if(n == 0){
//       fprintf(stderr,"EOF\n");
//       return sent;
//     }
//     sent += n;
//   }

//   return sent;
// }

// //receives a buffer of length len
// int recv_all(int sock,char *buf,int len){
//   int received  = 0;
//   while(received < len){
//     int n = recv(sock, buf + received,len - received,0);
//     if(n==-1){
//       perror("cant recv\n");
//       exit(1);
//     }else if(n==0){
//       fprintf(stderr,"EOF\n");
//       return received;
//     }
//     received += n;
//   }
//   return received;
// }

// void die(char* s)
// {
//   perror(s);
//   exit(-1);
// }


//on a given port, binds the host, identifies remote and connects to it
void server(char* port)
{
 if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){die("Server Socket");}
 
 host.sin_family = AF_INET;
 host.sin_port = htons(atoi(port));
 host.sin_addr.s_addr = INADDR_ANY;
 unsigned int host_len = sizeof(host);

 if(bind(sock, (struct sockaddr*)&host, host_len) == -1){die("Bind");}

 unsigned int remote_len = sizeof(remote);
 char data[IDLEN];
 printf("Waiting for handshake!\n");
 int n = recvfrom(sock, data, N, 0, (struct sockaddr*)&remote, &remote_len);
 if(n == -1){die("Receive Server");}
 if(n == 0){fputs("EOF detected in Receive Server\n", stderr);} //EOF
 if(strcmp(data,IDCHK) == 0){
   printf("Handshaken\n");
   if((connect(sock, (struct sockaddr*)&remote, remote_len)) == -1){die("Server Connect");}
   printf("Connected\n");
 }
}

//connect to a remote with given ip and port
void client(char* ip, char* port)
{
 if((sock = socket(AF_INET, SOCK_DGRAM, 0)) == -1){die("Client Socket");}

 remote.sin_family = AF_INET;
 remote.sin_port = htons(atoi(port));
 if(inet_aton(ip, &remote.sin_addr) == 0){fputs("Invalid address\n",stderr);}
 unsigned int remote_len = sizeof(remote);

 if(connect(sock, (struct sockaddr*)&remote, remote_len) == -1){die("Client Connect");}

 int n = send(sock, IDCHK, IDLEN, 0);
 if(n == -1){die("Client Send");}
 if(n == 0){fputs("EOF detected in Client Send\n", stderr);} //EOF
}

int main(int argc, char** argv)
{
 if (argc == 2) {
   server(argv[1]);
 } else if (argc == 3) {
   client(argv[1], argv[2]);
 } else {
   fputs("Server : ./udp_phone [local_port]\n", stderr);
   fputs("Client : ./upd_phone [remote_ip] [remote_port]\n", stderr);
 }
  pthread_t thread_recsend, thread_recvplay, thread2, thread3;
  int ret0,ret1,ret2,ret3;
  int s = sock;
  ret0 = pthread_create(&thread_recsend,NULL,(void *)phone_recsend,(void *) & s);
  ret1 = pthread_create(&thread_recvplay,NULL,(void *)phone_recvplay,(void *)&s);

  ret0 = pthread_join(thread_recsend,NULL);
  if (ret1 != 0) {
      die("cannot join thread 0");
  }

  ret1 = pthread_join(thread_recvplay,NULL);
  if (ret1 != 0) {
      die("cannot join thread 1");
  }

 

  //printf("NUMBER OF TIMES = %d\n",k);
 close(sock);
 return 0;
}
