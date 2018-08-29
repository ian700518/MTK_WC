#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <netdb.h>
#include "dabai.h"

#define DBG_EN 0

int Connect2Ser(char *HostAddr, int HostPort, int SelNA)
{
    struct sockaddr_in server;
    int sock;
    char buf[256];
    unsigned char *Fbuf;
    int n, i, ret, recct = 0, write_num = 0, sendfilesize = 0;;
    struct hostent *p;
    struct timeval timeout;
    fd_set readfd;
    FILE *fp_sock;

    /* 製作 socket */
    signal(SIGPIPE, SIG_IGN);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    /* 準備連線端指定用的 struct 資料 */
    server.sin_family = AF_INET;
    server.sin_port = htons(HostPort);

    /* Get Server Site address */
    if(SelNA) // get Name address
    {
      //printf("Get a Name Address\n");
      p = gethostbyname(HostAddr);
      if(p != NULL)
      {
        for(i=0;p->h_addr_list[i] != NULL;i++)
        {
          inet_pton(AF_INET, inet_ntoa(*(struct in_addr *)p->h_addr_list[i]), &server.sin_addr.s_addr);
        }
      }
      else
      {
        //printf("Get host name error~~~!!!!\n");
        return -1;
      }
    }
    else  // get IP Address
      inet_pton(AF_INET, HostAddr, &server.sin_addr.s_addr);

    #if DBG_EN
        printf("serve ip is %s\n", inet_ntoa(server.sin_addr));
    #endif
    /* 與 server 端連線 */
    ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
    if(ret < 0)
    {
      #if DBG_EN
          printf("connect error : %d\n", ret);
      #endif
      return 0;
    }

    /* 從伺服器接受資料 */
    memset(buf, 0, sizeof(buf));
    Fbuf = (char *)calloc(FILESIZE, sizeof(char));
    fp_sock = fopen("/DaBai/HostDeviceInfo.json", "r");
    if(fp_sock != NULL)
    {
      while(!feof(fp_sock))
      {
        fread(Fbuf, 1, FILESIZE, fp_sock);
      }
      sendfilesize = ftell(fp_sock);
      #if DBG_EN
          printf("Fbuf is %s, file length is %d\n", Fbuf, sendfilesize);
      #endif
      fclose(fp_sock);
    }
    while(1)
    {
      FD_ZERO(&readfd);
      FD_SET(sock, &readfd);
      timeout.tv_sec = 1;
      timeout.tv_usec = 0;
      ret = select(sock+1, &readfd, NULL, NULL, &timeout);
      if(ret < 0)
      {
        #if DBG_EN
            printf("select function error~~!!(In Sockeet)\n");
        #endif
        break;
      }
      else if(ret == 0)
      {
        #if DBG_EN
            printf("select function timeout~~!!(In Socket)\n");
        #endif
        break;
      }
      else
      {
        if(FD_ISSET(sock, &readfd))
        {
          n = read(sock, buf, sizeof(buf));
          if(n>0)
          {
            #if DBG_EN
                printf("\t[Info] Receive %d bytes: %s\n", n, buf);
            #endif
            while(write_num < sendfilesize)
            {
              write_num = write(sock, Fbuf, sendfilesize);
            }
            #if DBG_EN
                printf("\t[Info] Transfer %d bytes: %s\n", write_num, Fbuf);
            #endif
          }
          break;
        }
      }
    }
    //n = read(sock, buf, sizeof(buf));
    //printf("\t[Info] Receive %d bytes: %s\n", n, buf);

    /* 結束 socket */
    close(sock);
    free(Fbuf);
    //printf("[Info] Close socket\n");
    return 0;
}

void *SockConnProcess(void *arg)
{
  struct SocketPara *Sockarg;
  Sockarg = (struct SocketPara *) arg;

  printf("into SockConnProcess ~~~!!!\n");
  if(fork() == 0)
  {
    //if(fork() == 0)
    //{
      while(1)
      {
        Connect2Ser(Sockarg->Addr, Sockarg->Port, Sockarg->NameAddr);
        sleep(1);
      }
    //}
    //else
    //{
    //  waitpid();
    //}
  }
  else
  {
    waitpid();
  }
}
