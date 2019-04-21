#pragma clang diagnostic push
#pragma ide diagnostic ignored "hicpp-member-init"
#pragma ide diagnostic ignored "hicpp-signed-bitwise"
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
#include <netinet/tcp.h>
#include <bits/stdc++.h>

#define MAX_LEN 1024
using namespace std;

struct __attribute__((packed)) udp_payload
{
    char topic[50];
    unsigned char tip_date;
    char continut[1500];
};

struct Mesaj{
    int idx;
    mutable int nr_left;
    sockaddr_in adr;
    unsigned char tip_date;
    int len;
    char continut[1500];
};

struct __attribute__((packed)) daTrimis{
    char nume[50];
    unsigned char tip_date;
    char continut[1500];
    int len;
    sockaddr_in adr;
};

struct Topic_root{
    string nume;
    mutable deque<Mesaj> dq;
    mutable int nr_del;
    mutable int num_subs;
    mutable int crt_idx;
};

struct chosen_topic
{
    char sz:1;
    string topic;
    mutable int last_seen;
};

class chosen_topic_comp
{
public:
    bool operator () ( const chosen_topic &a,const chosen_topic &b)
    {
        return a.topic.compare(b.topic);
    }
};

struct client
{
    string id;
    sockaddr_in adr;
    set< chosen_topic,chosen_topic_comp> subs;
};


class Topic_root_comp
{
public:
    bool operator () (const Topic_root &a,const Topic_root &b) const
    {
        return a.nume.compare(b.nume);
    }
};

set<Topic_root,Topic_root_comp> topics;
map<int,client> clientii;

void parse_command(int fd,char* buffer)
{
    string input(buffer);
    vector <string> tokens;
    stringstream sstream(input);
    string temp;

    while(getline(sstream, temp, ' '))
        tokens.push_back(temp);

    if( tokens.size()<2) goto parse_command_fin;
    if( tokens.size()>3) goto parse_command_fin;

    if( tokens[0]=="subscribe")
    {
        chosen_topic t; t.sz=0; t.last_seen=0;
        if(tokens.size()==3)
        {
            if(tokens[2].size()!=1) { printf("invalid sz\n"); return; }
            if( tokens[2][0]=='1') t.sz = 1;
            else {
                if (tokens[2][0] == '0') t.sz = 0;
                else {
                    printf("invalid sz\n");
                    return;
                }
            }
        }

        Topic_root tr;
        tr.nr_del=0;
        tr.num_subs=1;
        tr.crt_idx=0;
        tr.nume=tokens[1];
        auto it=topics.find(tr);

        if(it==topics.end())
        {
            topics.insert(tr);
            t.last_seen=0;
        }
        else {
            it->num_subs = it->num_subs + 1;
            t.last_seen = it->crt_idx;
        }

        t.topic = tokens[1];
        clientii[fd].subs.insert(t);
    }

    if( tokens[0]=="unsubscribe")
    {
        if( tokens.size()==3) goto parse_command_fin;
        Topic_root tr; tr.nume=tokens[1];
        auto it=topics.find(tr);
        if(it==topics.end()) return;

        chosen_topic t; t.topic=tokens[1];
        it->num_subs=it->num_subs-1;

        auto it2=clientii[fd].subs.find(t);
        for (int j = it2->last_seen - it->nr_del + 1; j < it->dq.size(); j++)
            it->dq[j].nr_left--;

        if (it->dq.size()) {
            while (it->dq.front().nr_left <= 0) {
                it->dq.pop_front();
                it->nr_del = it->nr_del + 1;
                if (!it->dq.size()) break;
            }
        }
        clientii[fd].subs.erase(t);

    }
    return;

parse_command_fin:
    printf("bad command, please provide a good one\n");
}


int main(int argc,char* args[])
{
    if(argc<2)
    {
        printf("Correct usage of server is ./server <port>\n");
        return 0;
    }

    int port=atoi(args[1]);
    printf("%d\n",port);

    int listen_fd_udp,listen_fd_tcp;
    struct sockaddr_in server_addr;

    if ( (listen_fd_tcp = socket(AF_INET, SOCK_STREAM,0)) < 0 ) {
        perror("tcp socket creation failed");
        exit(EXIT_FAILURE);
    }

    if ( (listen_fd_udp = socket(AF_INET, SOCK_DGRAM,0)) < 0 ) {
        perror("udp socket creation failed");
        exit(EXIT_FAILURE);
    }

    int enable = 1;
    if (setsockopt(listen_fd_udp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt for udp(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd_tcp, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0) {
        perror("setsockopt for tcp(SO_REUSEADDR) failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(listen_fd_tcp, SOL_TCP, TCP_NODELAY, &enable, sizeof(int)) < 0) {
        perror("setsockopt for tcp(TCP_NODELAY) failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if ( bind(listen_fd_tcp, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if ( bind(listen_fd_udp, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0 ) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    int lr_tcp = listen(listen_fd_tcp, 100);
    if(lr_tcp<0)
    {
        perror("tcp listening failed");
        exit(EXIT_FAILURE);
    }

    fd_set sock_set;
    int maxfd = 0;
    FD_ZERO(&sock_set);
    FD_SET(STDIN_FILENO, &sock_set);
    FD_SET(listen_fd_tcp, &sock_set);
    FD_SET(listen_fd_udp,&sock_set);

    if(listen_fd_tcp > maxfd)
        maxfd = listen_fd_tcp;

    if(listen_fd_udp > maxfd)
        maxfd = listen_fd_udp;

    struct sockaddr_in clint;
    socklen_t client_length= sizeof(clint);

    int i,j,counter=0;
    char buff[MAX_LEN];
    int recv_m,sel,rd,new_sock,new_id;

    while(1)
    {
        fd_set tmp_sock_set = sock_set;
        sel = select(maxfd + 1, &tmp_sock_set, nullptr, nullptr, nullptr);
        if(sel<0)
        {
            perror("select error\n"); exit(EXIT_FAILURE);
        }

        for(i=0;i<=maxfd;i++)
            if(FD_ISSET(i,&tmp_sock_set))
            {
                if(i==STDIN_FILENO)
                {
                    rd = read(STDIN_FILENO,buff,MAX_LEN);
                    buff[rd-1]='\0';
                    if(strcmp(buff,"exit")==0)
                        goto iesi;
                    break;
                }

                if( i == listen_fd_tcp)
                {
                    client_length=sizeof(clint);
                    new_sock = accept(listen_fd_tcp,(struct sockaddr*)&clint,&client_length);
                    if(new_sock < 0)
                    {
                        perror("eroare acceptare nou client tcp\n");
                        exit(EXIT_FAILURE);
                    }
                    FD_SET(new_sock, &sock_set);
                    if(maxfd < new_sock)
                        maxfd = new_sock;

                    recv_m = recv(new_sock, buff, sizeof(buff), 0);
                    if(recv_m<0)
                    {
                        perror("n-am putut citi id-ul clientului");
                        exit(EXIT_FAILURE);
                    }
                    buff[recv_m]='\0';

                    if( clientii.find(new_sock)!=clientii.end()) break;
                    clientii[new_sock].id=string(buff);
                    clientii[new_sock].adr=clint;
                    printf("New client %s connected from %s:%d\n",buff,inet_ntoa(clint.sin_addr),ntohs(clint.sin_port));
                    break;
                }

                if(i==listen_fd_udp)
                {
                    udp_payload t;

                    client_length=sizeof(clint);
                    recv_m = recvfrom(i,&t,sizeof(t),0,(struct sockaddr*)&clint,&client_length);
                    printf("New message from%s:%d\n",inet_ntoa(clint.sin_addr),ntohs(clint.sin_port));
                    if(recv_m<0)
                    {
                        perror("error while receiving from udp\n");
                        exit(EXIT_FAILURE);
                    }

                    Mesaj top;
                    top.len=recv_m-51;
                    top.adr=clint;
                    memcpy(top.continut,t.continut,1500);
                    top.tip_date=t.tip_date;


                    printf("mesaj %s cu tipul %d si continut \n",t.topic,t.tip_date);
                  //  for(i=0;i<recv_m-51;i++)
                    //    printf("%d ",t.continut[i]); printf("\n");

                    Topic_root tr;
                    tr.num_subs=0;
                    tr.nr_del=0;
                    tr.nume=string(t.topic);
                    tr.crt_idx=0;
                    auto it= topics.find(tr);

                    if( it!=topics.end()) {
                        top.nr_left=it->num_subs;
                        top.idx=it->crt_idx+1;
                        it->crt_idx=it->crt_idx+1;
                        if(it->num_subs) it->dq.push_back(top);
                    }
                    else {
                        top.idx=0;
                        top.nr_left=0;
                        tr.dq.push_back(top);
                        topics.insert(tr);

                    }
                  //  for(it=topics.begin();it!=topics.end();it++)
                    //    printf("%s\n",it->nume.c_str());
                    break;
                }

                recv_m=recv(i, buff, sizeof(buff), 0);
                if(recv_m<0)
                {
                    perror("n-am putut citi comanda clientului");
                    exit(EXIT_FAILURE);
                }
                if(recv_m==0)
                {
                    close(i);
                    FD_CLR(i, &sock_set);
                    FD_CLR(i, &tmp_sock_set);
                    printf("Client %s disconnected\n",clientii[i].id.c_str());
                    break;
                }
                parse_command(i,buff);

            }

        tmp_sock_set = sock_set;
        sel = select(maxfd + 1,nullptr, &tmp_sock_set, nullptr, nullptr);

        for(i=0;i<=maxfd;i++)
            if(FD_ISSET(i,&tmp_sock_set) && i!=STDIN_FILENO && i!=listen_fd_tcp && i!=listen_fd_udp) {
                int ultimu_vazut;
                Mesaj m;
                daTrimis dtrim;
                printf("%d\n",i);
                for (auto it = clientii[i].subs.begin(); it != clientii[i].subs.end(); it++) {

                    for(auto it3=topics.begin();it3!=topics.end();it3++)
                        printf("%s ",it3->nume.c_str()); printf("\n");
                        printf("%s\n",it->topic.c_str());
                    ultimu_vazut = it->last_seen;

                    Topic_root tr;
                    tr.nume = it->topic;
                    auto it2 = topics.find(tr);
                    printf("%s %d\n",tr.nume.c_str(),it2==topics.end());
                    if(it2==topics.end() || it2->dq.empty()) continue;

                    if (it->sz == 0) {
                        if (it2->dq.back().idx != ultimu_vazut) {
                            m=it2->dq.back();
                            dtrim.adr=m.adr;
                            dtrim.len=m.len;
                            dtrim.tip_date=m.tip_date;
                            it->topic.copy(dtrim.nume,(int)it->topic.size(),0);
                            memcpy(m.continut,dtrim.continut,m.len);
                            send(i, &dtrim, sizeof(dtrim), 0);
                            it->last_seen = it2->dq.back().idx;
                        }
                    } else {
                        printf("ultimu_vazut %d,nr_del %d and size %lu\n", ultimu_vazut, it2->nr_del, it2->dq.size());
                        for (j = ultimu_vazut - it2->nr_del + 1; j < it2->dq.size(); j++) {
                            printf("%d %d %d\n", j, it2->dq[j].idx,ntohs(it2->dq[j].adr.sin_port));
                            m=it2->dq.back();
                            dtrim.adr=m.adr;
                            dtrim.len=m.len;
                            dtrim.tip_date=m.tip_date;
                            it->topic.copy(dtrim.nume,(int)it->topic.size(),0);
                            memcpy(m.continut,dtrim.continut,m.len);
                            send(i, &dtrim, sizeof(dtrim), 0);
                            it->last_seen = it2->dq[j].idx;
                            it2->dq[j].nr_left--;
                        }
                        if (!it2->dq.empty()) {
                            while (it2->dq.front().nr_left <= 0) {
                                it2->dq.pop_front();
                                it2->nr_del = it2->nr_del + 1;
                                if (it2->dq.empty()) break;
                            }
                            it->last_seen = it2->crt_idx;
                        }
                    }
                }
            }
        printf("iese\n");
    }

iesi:
    close(listen_fd_udp);
    close(listen_fd_tcp);
    return  0;
}
#pragma clang diagnostic pop