#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <time.h>
//#include "bandpass_fft.h"
//#include "send_recv_all.h"

#define N 1024

struct timeval start_fread, start_send, start_recv, start_fwrite, 
  stop_fread, stop_send, stop_recv, stop_fwrite, loop_start, loop_stop;

const char IDCHK[] = "aktkb46";
#define IDLEN 8

int sock;
struct sockaddr_in remote, host;

//sends a buffer of length len
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

//receives a buffer of length len
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

void die(char* s)
{
  perror(s);
  exit(-1);
}


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

 
 FILE *fp_rec;
 FILE *fp_play;
 if ( (fp_rec=popen("rec -q -t raw -b 16 -c 1 -e s -r 44100 -","r")) ==NULL) {die("popen:rec");}
 if ( (fp_play=popen("play -q -t raw -b 16 -c 1 -e s -r 44100 -","w")) ==NULL) {die("popen:play");}


  char send_data[N], recv_data[N];
  int n_read, n_send, n_recv, n_write;
  struct timespec start, stop;
  long remaining = 1000000000;
  int k = 0;
  while(1){
    //gettimeofday(&start, NULL);
    clock_gettime(CLOCK_MONOTONIC, &start);
    memset(send_data, 0, N);
    memset(recv_data, 0, N);

    //read from the popen file pointer
    n_read = fread(send_data,sizeof(char),N,fp_rec);
    //printf("FREAD\t");
    if(n_read == 0){shutdown(sock, SHUT_WR);break;} //EOF

    //send over the TCP
    //n_send = send(sock, send_data, n_read, 0);
    n_send = send_all(sock, send_data, n_read);
    if(n_send == -1){die("Send");}
    if(n_send == 0){shutdown(sock, SHUT_WR); break;} //EOF
    // printf("SEND\t");

    //receive from the TCP
    //n_recv=recv(sock, recv_data, N, 0);
    n_recv = recv_all(sock, recv_data, N);
    if(n_recv == -1){die("Receive");}
    if(n_recv == 0){shutdown(sock, SHUT_RD); break;}
    //printf("RECV\t");

    //write to the play file pointer
    n_write = fwrite(recv_data,sizeof(char),n_recv,fp_play);
    //printf("WRITE\t");
    k++;
    clock_gettime(CLOCK_MONOTONIC, &stop);
    remaining = remaining - (stop.tv_nsec - start.tv_nsec);
    if(remaining < 0 || remaining == 0){break;}
    //gettimeofday(&stop, NULL);
    
    //printf("\n%d \t %d \t %d \t %d \t %lu\n",n_read, n_send, n_recv, n_write,( stop.tv_nsec - start.tv_nsec)/1000);
 }

  printf("NUMBER OF TIMES = %d\n",k);
 fclose(fp_rec);
 fclose(fp_play);
 close(sock);
 return 0;
}
