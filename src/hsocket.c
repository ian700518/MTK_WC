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

int Connect2Ser(char *HostAddr, int HostPort)
{
    struct sockaddr_in server;
    struct sockaddr_in *sockaddr_ipv4;
    int sock;
    char buf[256];
    unsigned char *Fbuf;
    int n, i, ret, recct = 0, write_num = 0, sendfilesize = 0;;
    //struct hostent *p;
    struct timeval timeout;
    fd_set readfd;
    FILE *fp_sock;
    struct addrinfo *result = NULL;
    //struct addrinfo *ptr = NULL;
    //struct addrinfo hints;

    /* 製作 socket */
    signal(SIGPIPE, SIG_IGN);
    sock = socket(AF_INET, SOCK_STREAM, 0);

    /* 準備連線端指定用的 struct 資料 */
    server.sin_family = AF_INET;
    server.sin_port = htons(HostPort);

    ret = getaddrinfo(HostAddr, NULL, NULL, &result);
    if(ret != 0)
    {
      #if DBG_EN
          printf("Get host name error~~~!!!!\n");
      #endif
      return -1;
    }
    switch(result->ai_family)
    {
        case AF_UNSPEC:
            #if DBG_EN
                printf("Unspecified\n");
            #endif
            break;
        case AF_INET:
            #if DBG_EN
                printf("AF_INET (IPv4)\n");
            #endif
            sockaddr_ipv4 = (struct sockaddr_in *)result->ai_addr;
            #if DBG_EN
                printf("\tIPv4 address %s\n", inet_ntoa(sockaddr_ipv4->sin_addr));
            #endif
            inet_pton(AF_INET, inet_ntoa(sockaddr_ipv4->sin_addr), &server.sin_addr.s_addr);
            break;
        case AF_INET6:
            #if DBG_EN
                printf("AF_INET6 (IPv6)\n");
            #endif
            /*
            // the InetNtop function is available on Windows Vista and later
            // sockaddr_ipv6 = (struct sockaddr_in6 *) ptr->ai_addr;
            // printf("\tIPv6 address %s\n",
            //    InetNtop(AF_INET6, &sockaddr_ipv6->sin6_addr, ipstringbuffer, 46) );

            // We use WSAAddressToString since it is supported on Windows XP and later
            sockaddr_ip = (LPSOCKADDR) ptr->ai_addr;
            // The buffer length is changed by each call to WSAAddresstoString
            // So we need to set it for each iteration through the loop for safety
            ipbufferlength = 46;
            iRetval = WSAAddressToString(sockaddr_ip, (DWORD) ptr->ai_addrlen, NULL,
                ipstringbuffer, &ipbufferlength );
            if (iRetval)
                printf("WSAAddressToString failed with %u\n", WSAGetLastError() );
            else
                printf("\tIPv6 address %s\n", ipstringbuffer);
                */
            break;
        default:
            #if DBG_EN
                printf("Other %ld\n", result->ai_family);
            #endif
            break;
    }


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
  Sockarg = (struct SocketPara *)arg;

  printf("into SockConnProcess ~~~!!!\n");
  //if(fork() == 0)
  //{
    //if(fork() == 0)
    //{
      #if DBG_EN
          printf("First print Nameaddr Sockarg->Addr '%s', Sockarg->Port '%d'\n", Sockarg->Addr, Sockarg->Port);
      #endif
      while(1)
      {
        Connect2Ser(Sockarg->Addr, Sockarg->Port);
        sleep(1);
      }
    //}
    //else
    //{
    //  waitpid();
    //}
  //}
  //else
  //{
  //  waitpid();
  //}
}
