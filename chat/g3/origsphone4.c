/*what to fix:
//1:char into short not unsigned char
//2:checkskipのsoundは-35536~35536くらい
//3:#include <unistd.h>
ssize_t write(int fd, const void *buf, size_t count);
write() は buf で示されるバッファから最大 count バイトまでをファイル・ディスクリプタ fd によって参照されるファイルへと書き込む。 POSIX は write() が行なわれた後に実行した read(2) が 新しいデータを返すことを要求している。 全てのファイル・システム (file system) が POSIX 準拠ではない点に注意すること。 成功した場合、書き込まれたバイト数が返される (ゼロは何も書き込まれなかったことを示す)。 エラーならば -1 が返され、errno が適切に設定される。
count が 0 で、 fd が通常のファイル (regular file) を参照している場合、 write() は後述のエラーのいずれかを検出した場合、失敗を返すことがある。 エラーが検出されなかった場合は、 0 を返し、他に何の影響も与えない。 count が 0 で、 fd が通常のファイル以外のファイルを参照している場合、 その結果は規定されていない。
//4:N into fragmnet
*/

/*improvement:
1:/dev/dsp
2:checkskip
3:pthread
4:chat

A:UDP
B:LPF
 */

#include <stdio.h>
#include <unistd.h>  // sleep()を定義
#include <pthread.h>
#include <fcntl.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include<netinet/in.h>
#include<netinet/ip.h>
#include<netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<linux/soundcard.h>
#include<sys/ioctl.h>
#include<unistd.h>

#define N 256 //because of short being 2bytes

pthread_t th0, th1, th2, th3;
struct sockaddr_in client_addr;

void die(char *s)
{
  perror(s);
  exit(1);
}

int dsp_setup(int fd)
{
  int argx = 0x7fff0008; //0xNNNNMMMM NNNN=7fff(unlimited),fragsizw=2^MMMM this case 2^8=256
  ioctl( fd, SNDCTL_DSP_SETFRAGMENT, &argx ); //fragmentsize into 256bytes
  //notes:this must come first.
  
  int freq=44100;  
  /* サンプリング周波数の設定 */
  if ( ioctl( fd, SOUND_PCM_WRITE_RATE, &freq ) == -1 ) {
    perror( "iotcl( SOUND_PCM_WRITE_RATE )" );
    return -1;
  }    //smapling freq into intfreq

  int fmt     = AFMT_S16_LE;
  /* サウンドフォーマットの設定 */
  if ( ioctl( fd, SOUND_PCM_SETFMT, &fmt ) == -1 ) {
    perror( "ioctl( SOUND_PCM_SETFMT )" );
    return -1;
  }//into 16bit //65536vari sound //short 2bytes
  
  //end open and set devdsp
  return 1;
}

int checksend( short* buf, int x )
{
  short e = 64;//0x0040;
  int i;
  //short sound; //char buf 2こで１つの音なので
  
  for (i=0;i<x;i++) {  //
    //memcpy( &sound, &buf[i], sizeof(short) );
    if ( (buf[i] >= e)||(buf[i] <= -e) )      return 1; //send ok
  }
  
  return 0; //no send
}

void* thread0(void *arg)
{
  short buf[N];
  int fd = open("/dev/dsp", O_RDONLY, 0644);
  if( !dsp_setup(fd) ) return NULL;
  int sc = *(int *) arg;
  int x;
  
  while( 1 ){
    x = read(fd, buf, N);
    if(x == -1) die("read");
    if(checksend(buf,x)) send(sc, buf, x, 0); //is that ok?
  }
    
  close(fd);
  return NULL;
}

void* thread1(void *arg)
{
  short buf[N];
  int fd = open("/dev/dsp", O_WRONLY, 0644);
  if( !dsp_setup(fd) ) return NULL;  

  int sc = *(int *) arg;
  int x;
  
  while(1) 
    {
      x = recv(sc, buf, N, 0);
      if(x == -1) die("recv");
      if(write(fd, buf, x) == -1) die("write"); 
    }

  close(fd);
  return NULL;
}

void* thread2(void *arg)
{
  char buf[N];
  int s = *(int *) arg;
  int x;
  
  while( 1 ){
    gets(buf);
    if(buf[0] == 'e' && buf[1] == 'x' && buf[2] == 'i' && buf[3] == 't') break;
    if(send(s, buf, strlen(buf), 0) == -1) die("send");
  }
  pthread_cancel(th0);
  pthread_cancel(th1);
  pthread_cancel(th3);
  return NULL;
}

void* thread3(void *arg)
{
  char buf[N];
  int sc = *(int *) arg;
  int x;
  int i;
  while(1) 
    {
      x = recv(sc, buf, N, 0);
      if(x == -1) die("recv");
      while(i < N && buf[i] == 1) i++;
      if(i == N) break;
      buf[x] ='\n';
      write(1, "your friend > ", 14); // ダメだったらここを消す
      if(write(1, buf, x + 1) == -1) die("write");
    }
  
  pthread_cancel(th0);
  pthread_cancel(th1);
  pthread_cancel(th2);

  return NULL;
}

int function(int port_num)
{
   // begin socket
  int s = socket(PF_INET, SOCK_STREAM, 0);
  if(s == -1) 
    {
      perror("socket");
      exit(1);
    }
  // end socket

  //begin addr
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_port = htons(port_num);
  addr.sin_addr.s_addr = INADDR_ANY;

  socklen_t q = sizeof(addr);
  
  if(bind(s, (struct sockaddr *)&addr, q) == -1) 
    {
      perror("bind");
      exit(1);
    }

  if(listen(s, 10) == -1) 
    {
      perror("listen");
      exit(1);
    }
  
  socklen_t len = sizeof(struct sockaddr_in);
  int sc;
  sc = accept(s, (struct sockaddr *)&client_addr, &len);

  if(sc == -1) 
    {
      perror("accpt");
      exit(1);
    }
  return sc;
}

int main(int argc, char **argv) 
{
  if(argc<2){
    printf("[port]\n");
    return -1;
  }
  
  int port_num = atoi(argv[1]); 
  char *ip_adress;
  int sc0,sc1;
  sc0 = sc1 = function(port_num);
  int sc2, sc3;
  sc2 = sc3 = function(port_num+100);
  ip_adress = inet_ntoa(client_addr.sin_addr);
  printf("IP address %s call you.\n", ip_adress);
  printf("reject: r  accept: any key\n ");
  char c = getchar();
  if(c == 'r') return 0;
  
  pthread_create( &th0, NULL, thread0, (void *)&sc0);
  pthread_create( &th1, NULL, thread1, (void *)&sc1);
  pthread_create( &th2, NULL, thread2, (void *)&sc2);
  pthread_create( &th3, NULL, thread3, (void *)&sc3);
  
  pthread_join(th0, NULL);
  pthread_join(th1, NULL);
  pthread_join(th2, NULL);
  pthread_join(th3, NULL);
  
  return 0;
}
