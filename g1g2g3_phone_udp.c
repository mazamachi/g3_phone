#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "bandpass_fft.h"
#include "send_recv_all.h"

#define N 4096
#define SEND_N 1000


const char IDCHK[] = "aktkb46";
#define IDLEN 8

int sock;
struct sockaddr_in remote, host;


//on a given port, binds the host, identifies remote and connects to it
void server(char* port)
{
 if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1){die("Server Socket");}
 
 host.sin_family = AF_INET;
 host.sin_port = htons(atoi(port));
 host.sin_addr.s_addr = INADDR_ANY;
 unsigned int host_len = sizeof(host);

 if(bind(sock, (struct sockaddr*)&host, host_len) == -1){die("Bind");}

 unsigned int remote_len = sizeof(remote);
 char data[IDLEN];
 int n = recvfrom(sock, data, SEND_N, 0, (struct sockaddr*)&remote, &remote_len);
 if(n == -1){die("Receive Server");}
 if(n == 0){fputs("EOF detected in Receive Server\n", stderr);} //EOF
 if(strcmp(data,IDCHK) == 0){
   if((connect(sock, (struct sockaddr*)&remote, remote_len)) == -1){die("Server Connect");}
 }
}

//connect to a remote with given ip and port
void client(char* ip, char* port)
{
 if((sock = socket(PF_INET, SOCK_DGRAM, 0)) == -1){die("Client Socket");}

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

 
 FILE *fp_rec;
 FILE *fp_play;
 if ( (fp_rec=popen("rec -q -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","r")) ==NULL) {die("popen:rec");}
 if ( (fp_play=popen("play -t raw -b 16 -c 1 -e s -r 44100 - 2> /dev/null","w")) ==NULL) {die("popen:play");}

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
 fclose(fp_rec);
 fclose(fp_play);
 close(sock);
 return 0;
}