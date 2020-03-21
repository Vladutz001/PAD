#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <pthread.h>
#include "netio.h"

#define PORT 5234

typedef struct ClientInfo{
    int conn;
}ClientInfo;

int connfd, sockfd, n, fd;
struct sockaddr_in local, remote;
struct ClientInfo* c;
char buff[1024],user[100],passw[100];

void connectToClient(){
    char fileDescriptorsString[20];
    socklen_t size = sizeof(remote);
    if((connfd = accept(sockfd, (struct sockaddr *)&remote, &size)) < 0){
        printf("Unable to create connection!");
        exit(-5);
    }
    sprintf(fileDescriptorsString, "%d", connfd);
    c = (ClientInfo *)malloc(sizeof(ClientInfo));
    c->conn = connfd;
    if (write(fd, fileDescriptorsString, strlen(fileDescriptorsString)) < 0){
        printf("Unable to write to file!");
        exit(-8);
    }
}

void login(){
    FILE *uf;
    int log, found = 0;
    char loginString[256];
    if ((uf = fopen("credentials.txt", "a+")) == NULL){
        printf("Error opening credentials file!");
        exit(-11);
    }
    while (found == 0){
        if(recv(c->conn, loginString, 255, 0) < 0){
            printf("Error receiving credentials info from client!");
            exit(-12);
        }
        sscanf(loginString, "%d;%[^;];%[^;];", &log, user, passw);
        if(log == 0){
            fprintf(uf, "%s;%s;\n", user, passw);
            char r[] = "1";
            if (send(c->conn, r, strlen(r), 0) < 0){
                printf("Error sending confirmation to client!");
                exit(-13);
            }
            found = 1;
        }else{
            char foundName[100],foundPassw[100];
            while (fscanf(uf, "%[^;];%[^;];\n", foundName, foundPassw) != EOF){
                if (strcmp(foundName, user) == 0 && strcmp(foundPassw, passw) == 0){
                    char r[] = "1";
                    if(send(c->conn, r, strlen(r), 0) < 0){
                        printf("Error sending confirmation to client!");
                        exit(-14);
                    }
                    found = 1;
                    break;
                }
            }
            if(found == 0){
               char v[] = "0";
               if(send(c->conn, v, strlen(v), 0) < 0){
                   printf("Error sending failure to client!");
                   exit(-15);
               }
            }
        }
        fseek(uf, 0, SEEK_SET);
    }
    fclose(uf);
    
}

void *child(void *arg){
    c = (ClientInfo *)arg;
    int length;
    char fileDescriptorsString[20];
    int number;
    login();
    while(1){
        n = read(c->conn, (void *) buff, 1024);
        if (n <= 0) {
            pthread_exit(NULL);
        }
        buff[n] = '\0';
        printf("%s\n", buff);
        lseek(fd, 0, SEEK_SET);
        while ((length = read(fd, (void *)fileDescriptorsString, sizeof(char))) > 0){
            if (length < 0){
                printf("Error reading from file!");
                exit(-9);
            }
            fileDescriptorsString[length] = '\0';
            number = atoi(fileDescriptorsString);
            if (write(number, (void *)buff, strlen(buff)) < 0){
                printf("Error writing to clients!");
                exit(-10);
            }
        }
    }
}

int main(){
    if ((fd = open("sockets.txt", O_RDWR | O_APPEND | O_TRUNC | O_CREAT,0777)) < 0){
        printf("Error opening socket descriptors files!");
        exit(-8);
    }
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating socket for server!");
        exit(-1);
    }
    set_addr(&local, NULL, INADDR_ANY, PORT);
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt))<0){
        perror("setsockopt");exit(EXIT_FAILURE);
    }
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&opt, sizeof(opt))<0){
        perror("setsockopt");exit(EXIT_FAILURE);
    }
    if (bind(sockfd, (struct sockaddr *)&local, sizeof(local)) < 0){
        printf("Error binding socket for server!");
        exit(-2);
    }
    if (listen(sockfd, 5) < 0){
        printf("Unable to listen on server socket!");
        exit(-3);
    }
    while (1){
        pthread_t t;
        connectToClient();
        if (pthread_create(&t, NULL, child, (void *)c) < 0){
            printf ("Error creating thread!");
            exit(-4);
        }
    }
    if (close(fd)<0){
        printf("Error closing file!");
        exit(-6);
    }
    if (close(connfd)<0){
        printf("Error closing socket!");
        exit(-7);
    }
}
