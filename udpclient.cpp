//
// Created by longman-stan on 20.04.2019.
//
#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-member-init"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <bits/stdc++.h>
using namespace std;
#define BUFLEN 256

struct __attribute__((packed)) payload
{
    char topic[50];
    unsigned char tip_date;
    char continut[1500];
};
string s;

void err( string msg)
{
    perror(msg.c_str());
    exit(0);
}

int main()
{
    int sockfd, n;
    struct sockaddr_in serv_addr;
    //struct hostent *server;

    fd_set read_fds;  //multimea de citire folosita in select()
    fd_set tmp_fds;    //multime folosita temporar
    int fdmax;     //valoare maxima file descriptor din multimea read_fds

    FD_ZERO(&read_fds);
    FD_ZERO(&tmp_fds);

    char buffer[BUFLEN];

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0)
        err(string("ERROR opening socket"));

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(8080);
    // serv_addr.sin_addr.s_addr = INADDR_ANY;
    inet_aton("127.0.0.1",&serv_addr.sin_addr);

    if (connect(sockfd,(struct sockaddr*) &serv_addr,sizeof(serv_addr)) < 0)
        err(string("ERROR udp connecting"));
    FD_SET(sockfd, &read_fds);
    FD_SET(0, &read_fds);
    fdmax = sockfd;
    int i;
    // main loop
    while (1) {
        tmp_fds = read_fds;
        if (select(fdmax + 1, &tmp_fds, NULL, NULL, NULL) == -1)
            err(string("ERROR in select"));

        if (FD_ISSET(STDIN_FILENO, &tmp_fds)) {
            payload p;
            memset(p.topic,0,1551);
            int nr=read(STDIN_FILENO,p.topic,50);
            if(nr==0) return 0;
            p.topic[nr-1]='\0';
            if( strcmp(p.topic,"exit")==0) break;
            nr = read(STDIN_FILENO,buffer,BUFLEN);
            p.tip_date=buffer[0];
            nr = read(STDIN_FILENO,p.continut,1500);
            p.continut[nr-1]='\0';
            printf("%s\n",p.topic);
            printf("%d\n",p.tip_date);
            printf("%s\n",p.continut);
            n = send(sockfd,&p,sizeof(p), 0);
            if (n < 0)
                err(string("ERROR writing to socket"));
        }
    }

    return 0;
}

#pragma clang diagnostic pop