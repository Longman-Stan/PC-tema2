#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
#pragma ide diagnostic ignored "modernize-use-nullptr"
//
// Created by longman-stan on 20.04.2019.
//
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
using namespace std;
#define MSGLEN 1024

struct __attribute__((packed)) daPrimit{
    char nume[50];
    unsigned char tip_date;
    char continut[1500];
    int len;
    sockaddr_in adr;
};

void parse_command(char* buffer)
{
    string input(buffer);
    vector <string> tokens;
    stringstream sstream(input);
    string temp;

    while(getline(sstream, temp, ' '))
        tokens.push_back(temp);

    printf("%s %s\n",tokens[0].c_str(),tokens[1].c_str());
}

int main(int argc,char* argv[])
{
    if(argc<4)
    {
        printf("Correct usage of the subscriber is ./subscriber <ID_client> <IP_server> <PORT_server>\n");
        return 0;
    }
    int id=atoi(argv[1]);
    char* server_ip=argv[2];
    int server_port=atoi(argv[3]);

    int sockfd = socket(AF_INET, SOCK_STREAM, 0);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    int errc = inet_pton(AF_INET, server_ip, &server_addr.sin_addr);
    if(errc <= 0) {
        perror("inet_pton");
        exit(-1);
    }

    if(connect(sockfd, (struct sockaddr*)(&server_addr), sizeof(server_addr)) < 0 )
    {
        perror("connect");
        exit(-1);
    }


    char mesaj[MSGLEN];
    strcpy(mesaj,argv[1]);
    send(sockfd, mesaj, strlen(mesaj), 0);

    fd_set client_set;
    FD_ZERO(&client_set);
    FD_SET(sockfd,&client_set);
    FD_SET(STDIN_FILENO, &client_set);
    int maxfd = 0;
    if(sockfd>maxfd)
        maxfd=sockfd;

    int len;
    char err[MSGLEN]; strcpy(err,"");
    daPrimit m;

    while(1)
    {

        fd_set tmp_set = client_set;
        int errc = select(maxfd + 1, &tmp_set, NULL, NULL, NULL);

        if(errc<0)
        {
            perror(strcat(strcat(err,"select subscriber:"),argv[1]));
            exit(EXIT_FAILURE);
        }

        if(FD_ISSET(STDIN_FILENO, &tmp_set))
        {
            // procesez de la tastatura
            len=read(STDIN_FILENO,mesaj,MSGLEN);
            mesaj[len-1]='\0';
            if(len==0) return 0;
            if(len<0)
            {
                perror( strcat(strcat(err,"nu a citit subscriber:"),argv[1]));
                exit(EXIT_FAILURE);
            }
            if(strcmp(mesaj,"exit")==0) {
                send(sockfd,NULL,0,0);
                return 0;
            }
            send(sockfd, mesaj, len, 0);
            parse_command(mesaj);
        }

        int put;
        uint32_t nr;
        uint32_t nr1;
        uint32_t modulu;
        uint8_t p;
        int nrcif,aux;

        if(FD_ISSET(sockfd, &tmp_set))
        {
            len=recv(sockfd,&m,sizeof(m),0);
            if(len==0)
                break;
            if(len<0)
            {
                perror( strcat(strcat(err,"eroare citire subscriber:"),argv[1]));
                exit(EXIT_FAILURE);
            }

            printf("%s:%d - %s - ",inet_ntoa(m.adr.sin_addr),ntohs(m.adr.sin_port),m.nume);

            switch(m.tip_date)
            {
                case 0:
                    memcpy(&nr,m.continut+1,sizeof(uint32_t));
                    if(m.continut[0]==1) printf( "INT - -%u\n",ntohl(nr));
                    else printf( "INT - %u\n",ntohl(nr));
                    break;
                case 1:
                    memcpy(&nr1,m.continut,sizeof(uint16_t));
                    nr1=ntohs(nr1);
                    if((nr1/10)%10==0) printf( "SHORT_REAL - %u.0%u\n",nr1/100,nr1%10);
                    else printf( "SHORT_REAL - %u.%u\n",nr1/100,nr1%100);
                    break;
                case 2:
                    memcpy(&modulu,m.continut+1,sizeof(uint32_t));
                    modulu=ntohl(modulu);
                    memcpy(&p,m.continut+sizeof(uint32_t)+1,sizeof(uint8_t));
                    nrcif=0;
                    aux=modulu;
                    while(aux) {nrcif++; aux/=10;}
                    if( nrcif<p)
                    {
                        printf("0.");
                        while(p>nrcif) { printf("0"); p--;}
                        printf("%u\n",modulu);
                        break;
                    }
                    put=1; while(p) { p--; put*=10;}
                    if(m.continut[0]==1) printf("FLOAT - -%.14g\n",(double)modulu/put);
                    else printf("FLOAT - %.14g\n",(double)modulu/put);
                    break;
                case 3:
                    printf("STRING -");
                    fflush(stdout);
                    write(STDOUT_FILENO,m.continut,m.len);
                    printf("\n");
                    break;
            }
        }

    }

    return 0;
}

#pragma clang diagnostic pop