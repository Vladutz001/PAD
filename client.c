#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "netio.h"

#define SERVERADDRESS "127.0.0.1" //change to your PC's address
#define SERVERPORT 1234

pid_t pid;
int sockfd;
struct sockaddr_in local, remote;
char username[100],password[100];
        
void login(){
    int n;
    char buff[10],sendToServer[256];
    printf("Bine ati venit la sistemul nostru de chat!\nPentru a te loga apasa 1, iar pentru a crea un cont nou apasa 0!\n");
    scanf("%d",&n);
    fflush(stdin);
    while(1){
        printf("Introduceti numele!\n");
        scanf("%s",username);
        printf("Introduceti parola!\n");
        scanf("%s",password);
        getchar();
        snprintf(sendToServer, 255, "%d;%s;%s;", n, username, password);
        if(send(sockfd, (void *)sendToServer, strlen(sendToServer), 0) < 0){
            printf("Can't send information to server!");
            exit(-5);
        }
        if(recv(sockfd, buff, sizeof(int), 0) < 0){
            printf("Can't receive feedback from server!");
            exit(-6);
        }
        if(atoi(buff) == 1) break;
    }
}

void f1(int signal){
    if(close(sockfd) < 0){
        printf("Unable to close socket!");
        exit(-8);
    }
    exit(0);
}

void childCode(){
    int n;
    char buff[1025];
    struct sigaction sig;
    sig.sa_flags = 0;
    sig.sa_handler = f1;
    sigemptyset(&(sig.sa_mask));
    if(sigaction(SIGUSR1, &sig, NULL) < 0){
        printf("Can't bind SIGUSR1");
        exit(-7);
    }
    for(;;){
        if((n = read(sockfd, buff, 1024)) <= 0){
            printf("Can't read from socket!");
            exit(-9);
        }
        buff[n] = '\0';
        if(n>0){
            int i,len = strlen(buff),cnt = 0;
            char message[1024];
            for(i = 0;i < len; i++){
                if(buff[i] == '\n'){
                    message[cnt] = '\0';
                    printf("%s\n",message);
                    cnt = 0;
                }else{
                    message[cnt] = buff[i];
                    cnt++;
                }
            }
        }
    }
}
    
void f2(){
    int status;
    wait(&status);
    if(!WIFEXITED(status)){
        printf("Error with child process!");
        exit(-11);
    }
    if(close(sockfd) < 0){
        printf("Error closing socket!");
        exit(-12);
    }
    exit(0);
}

void fatherCode(){
    struct sigaction sig;
    char buff[1025];
    sigemptyset(&(sig.sa_mask));
    sig.sa_handler = f2;
    sig.sa_flags = 0;
    if(sigaction(SIGCHLD, &sig, NULL) < 0){
        printf("Error using sigaction!");
        exit(-10);
    }
    fflush(stdin);
    fflush(stdout);
    for(;;){
        fgets(buff,1024,stdin);
        buff[strlen(buff)-1] = '\0';
        if(strcmp(buff,"close") == 0){
            if(kill(pid,SIGUSR1) < 0){
                printf("Error sending signal!");
                close(-14);
            }
        }
        char message[1125];
        strcpy(message, username);
        strcat(message,": ");
        strcat(message, buff);
        strcat(message,"\n");
        if(send(sockfd, message, strlen(message), 0) < 0){
            printf("Unable to send to server!");
            exit(-13);
        }
    }
}

void begin(){
    login();
    pid = fork();
    printf("pid");
    if (pid < 0){
        printf("Error using fork!");
        exit(-4);
    }
    if (pid == 0){
        printf("Done");
        childCode();
    }
    else fatherCode();
}

int main(){
    if ((sockfd = socket(PF_INET, SOCK_STREAM, 0)) < 0){
        printf("Error creating socket!");
        exit(-1);
    }
    set_addr(&local, NULL, INADDR_ANY, 0);
    if (bind(sockfd, (struct sockaddr *)&local, sizeof(local)) < 0){
        printf("Error binding socket!");
        exit(-2);
    }
    set_addr(&remote, SERVERADDRESS, 0, SERVERPORT);
    if (connect(sockfd, (struct sockaddr *)&remote, sizeof(remote)) < 0){
        printf("Error connecting to socket!!");
        exit(-3);
    }
    begin();
}
