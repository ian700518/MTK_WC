#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/shm.h>

int Connect2Ser(char *HostAddr, int HostPort)
{
    // 定义socket
    int sockfd = socket(AF_INET,SOCK_STREAM,0);
    // 定义sockaddr_in
    struct sockaddr_in skaddr;
    skaddr.sin_family = AF_INET;
    skaddr.sin_port = htons(HostPort);
    skaddr.sin_addr.s_addr = inet_addr(HostAddr);


}
