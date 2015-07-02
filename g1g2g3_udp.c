#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>

const char IDCHK[] = "aktkb46";
#define IDLEN 8
#define N 1000

int sock;
struct sockaddr_in remote, host;

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
 int n = recvfrom(sock, data, N, 0, (struct sockaddr*)&remote, &remote_len);
 if(n == -1){die("Receive Server");}
 if(n == 0){fputs("EOF detected in Receive Server\n", stderr);} //EOF
 if(strcmp(data,IDCHK) == 0){
   if((connect(sock, (struct sockaddr*)&remote, remote_len)) == -1){die("Server Connect");}
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
 if ( (fp_play=popen("play -t raw -b 16 -c 1 -e s -r 44100 -","w")) ==NULL) {die("popen:play");}

 char send_data[N], recv_data[N];
 int n, m, n_recv;
 while(1){
   //read from the popen file pointer
   n = fread(send_data,sizeof(char),N,fp_rec);
   if(n == 0){shutdown(sock, SHUT_WR);break;} //EOF
   //send over the TCP
   m = send(sock, send_data, n, 0);
   if(m == -1){die("Send");}
   if(m == 0){shutdown(sock, SHUT_WR); break;} //EOF
   //receive from the TCP
   n_recv=recv(sock, recv_data, N, 0);
   if(n_recv == -1){die("Receive");}
   if(n_recv == 0){shutdown(sock, SHUT_RD); break;}
   fwrite(recv_data,sizeof(char),n_recv,fp_play);
 }

 fclose(fp_rec);
 fclose(fp_play);
 close(sock);
 return 0;
}