#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <netdb.h>
#include "dabai.h"

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
      DBGSK("Get host name error~~~!!!!\n");
      return -1;
    }
    switch(result->ai_family)
    {
        case AF_UNSPEC:
            DBGSK("Unspecified\n");
            break;
        case AF_INET:
            DBGSK("AF_INET (IPv4)\n");
            sockaddr_ipv4 = (struct sockaddr_in *)result->ai_addr;
            DBGSK("\tIPv4 address %s\n", inet_ntoa(sockaddr_ipv4->sin_addr));
            inet_pton(AF_INET, inet_ntoa(sockaddr_ipv4->sin_addr), &server.sin_addr.s_addr);
            break;
        case AF_INET6:
            DBGSK("AF_INET6 (IPv6)\n");
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
            DBGSK("Other %ld\n", result->ai_family);
            break;
    }


    DBGSK("serve ip is %s\n", inet_ntoa(server.sin_addr));
    /* 與 server 端連線 */
    ret = connect(sock, (struct sockaddr *)&server, sizeof(server));
    if(ret < 0)
    {
      DBGSK("connect error : %d\n", ret);
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
      DBGSK("Fbuf is %s, file length is %d\n", Fbuf, sendfilesize);
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
        DBGSK("select function error~~!!(In Sockeet)\n");
        break;
      }
      else if(ret == 0)
      {
        DBGSK("select function timeout~~!!(In Socket)\n");
        break;
      }
      else
      {
        if(FD_ISSET(sock, &readfd))
        {
          n = read(sock, buf, sizeof(buf));
          if(n>0)
          {
            DBGSK("\t[Info] Receive %d bytes: %s\n", n, buf);
            while(write_num < sendfilesize)
            {
              write_num = write(sock, Fbuf, sendfilesize);
            }
            DBGSK("\t[Info] Transfer %d bytes: %s\n", write_num, Fbuf);
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

  DBGSK("into SockConnProcess ~~~!!!\n");
  //if(fork() == 0)
  //{
    //if(fork() == 0)
    //{
      DBGSK("First print Nameaddr Sockarg->Addr '%s', Sockarg->Port '%d'\n", Sockarg->Addr, Sockarg->Port);
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
