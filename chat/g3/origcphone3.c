#include <stdio.h> 
#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include<linux/soundcard.h>
#include <sys/ioctl.h>
#include <unistd.h>

#define N 256  //short

pthread_t th0, th1, th2, th3;

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
  //each 1/44100sec 2bytes written 256bytes ya 2.9msecond
  //before this 1/8000 1bytes writtten 1024bytes ya 1280msecond

  int fmt     = AFMT_S16_LE;
  /* サウンドフォーマットの設定 */
  if ( ioctl( fd, SOUND_PCM_SETFMT, &fmt ) == -1 ) {
    perror( "ioctl( SOUND_PCM_SETFMT )" );
    return -1;
  }//into 16bit //65536vari sound
  
  //end open and set devdsp
  return 1;
}

int checksend( short* buf, int x )
{
  short e = 64;//0x0040;
  int i;
  //short sound;
  
  for ( i = 0; i < x; i++) {  //char 2 de short 1
        //memcpy( &sound, &buf[i], sizeof(short) );
    if ( (buf[i] >= e)||(buf[i] <= -e) )      return 1; //send ok
  }
  
  return 0; //no send
}

void* thread0(void *arg)
{
  short buf[N];
  int fd = open("/dev/dsp", O_RDONLY, 0644);
  if(fd == -1) die("open");
  if( !dsp_setup(fd) ) return NULL;

  int s = *(int *) arg;
  int x;
  
  while( 1 ){
    x = read(fd, buf, N);
    if(x == -1) die("read");
    if(checksend(buf,x)) {
      if(send(s, buf, x, 0) == -1){
	die("send");
      }
    }
  }

  close(fd);
  return NULL;
}

void* thread1(void *arg)
{
  short buf[N];
  int fd = open("/dev/dsp", O_WRONLY, 0644);
  if(fd == -1) die("open");
  if( !dsp_setup(fd) ) return NULL;

  int s = *(int *) arg;
  int x;
  while(1) 
    {
      x = recv(s, buf, N, 0);
      if(x == -1) die("recv");
      if(write(fd, buf, x) == -1) {
	die("write");
      }
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
      if(write(1, buf, x + 1) == -1) die("write");
    }
  
  pthread_cancel(th0);
  pthread_cancel(th1);
  pthread_cancel(th2);

  return NULL;
}

int function(char *ip_adress, int port_num) 
{
  int s = socket(PF_INET, SOCK_STREAM, 0);
  
  if(s == -1) 
    {
      perror("socket");
      exit(1);
    }
  
  struct sockaddr_in addr;
  addr.sin_family = AF_INET;
  if(inet_aton(ip_adress, &addr.sin_addr) == 0) 
    {
      perror("ip adress");
      exit(1); 
    }
  addr.sin_port = htons(port_num);

  int ret = connect(s, (struct sockaddr *)&addr, sizeof(addr));

  if(ret == -1)
    {
      perror("connect");
      exit(1);
    }
  return s;
}
int main(int argc, char **argv) 
{
  if(argc!=2){
    printf("[port]\n");
    return -1;
  }
  
  char *ip_adress = "192.168.100.47";
  int port_num = atoi(argv[1]);
  
  int s0, s1;
  s0 = s1 = function(ip_adress, port_num);
  sleep(3);
  int s2, s3;
  s2 = s3 = function(ip_adress, port_num+100);
  
  // スレッド作成と起動
  pthread_create( &th0, NULL, thread0, (void *)&s0);
  pthread_create( &th1, NULL, thread1, (void *)&s1);
  pthread_create( &th2, NULL, thread2, (void *)&s2);
  pthread_create( &th3, NULL, thread3, (void *)&s3);
  
  pthread_join(th0, NULL);
  pthread_join(th1, NULL);
  pthread_join(th2, NULL);
  pthread_join(th3, NULL);
  
  return 0;
}
