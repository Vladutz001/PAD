#include<sys/types.h>
#include<sys/socket.h>
#include<netinet/in.h>
#include<string.h>
#include<unistd.h>
#include<netdb.h>
#include<stdio.h>
#include "netio.h"

int set_addr(struct sockaddr_in *addr, char *name, u_int32_t inaddr, short sin_port){
    struct hostent *h;
    
    memset((void *)addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    if (name != NULL){
        h = gethostbyname(name);
        if (h == NULL) return -1;
        addr -> sin_addr.s_addr = *(u_int32_t *) h -> h_addr_list[0];
    }else   addr -> sin_addr.s_addr = htonl(inaddr);
    addr -> sin_port = htons(sin_port);
    return 0;
}

int stream_read(int sockfd, char *buf, int len){
    int nread;
    int remaining = len;
    while (remaining > 0){
        if ((nread = read(sockfd, buf, remaining)) == -1) return -1;
        if (nread == 0) break;
        remaining -= nread;
        buf += nread;
    }
    return len - remaining;
}

int stream_write(int sockfd, char *buf, int len){
    int nwr;
    int remaining = len;
    while (remaining >0){
        if ((nwr = write(sockfd, buf, remaining)) == -1) return -1;
        remaining -= nwr;
        buf += nwr;
    }
    return len - remaining;
}
