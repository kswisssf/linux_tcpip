/*
linux并发服务器(concurrent server)
2.TCP并发服务器(fork方式）
    2.1.运行模型
    2.2.示例
    2.3.客户端发送
    2.4.运行结果
    2.5.结果分析

结果分析
1、两个客户端连接，fork出来的进程都能进行处理；
2、对于同一个客户端连接，fork出来的进行能够被调度处理到；
可以想象同一个端口存在一个连接队列。备注：在早期的linux版本，存在惊群问题，即会唤醒队列中所有进程，
造成性能问题，貌似新版本内核解决方案为只唤醒等待队列上的第一个进程或者线程。
3、不同进程的文件描述符可以相同，但代表不同的设备或者文件。

*/

#include <stdio.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <unistd.h>
#include "errno.h"
#include "signal.h"
#include <stdlib.h>
//#define NULL 0
#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 5
#define PIDNUMB 2

static void tcp_handle_connect(int ss)
{
    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    int n=0;
    char buff[BUFFLEN];
    time_t now;
    int count = 0;

    printf("handle_connect ss=%d ,pid=0x%x\n",ss, getpid());

    int sc;
    errno = 0;
    sc = accept(ss,(struct sockaddr*)&from,&len);
    if(sc==-1)
    {
        printf("accept sc=%d errno=%d,msg=%s\n",sc,errno,strerror(errno));
        exit(1);
    }

    static char tempStr[32] = {0};
    inet_ntop(AF_INET,&(from.sin_addr),tempStr,sizeof(tempStr));
    printf("[pid=0x%x]handle_connect ss=%d,sc=%d connect from %s:%u\n", getpid(),ss,sc,tempStr,ntohs(from.sin_port));

    while(1)
    {

        memset(buff,0,BUFFLEN);
        n=recv(sc,buff,BUFFLEN,0);

        if(n>0)
        {
            printf("[pid=0x%x]recv data ss=%d,sc=%d  port=%u data=[%s], count=%d\n",getpid(),ss,sc,ntohs(from.sin_port),buff,++count);
            memset(buff,0,BUFFLEN);
            now=time(NULL);
            sprintf(buff,"[%24s]\r\n",ctime(&now));
            send(sc,buff,strlen(buff),0);
        }

    }
    close(sc);
}

void sig_int(int num)
{
    exit(1);
}

/*

int socket(int af, int type, int protocol);
使用 <sys/socket.h> 头文件中 socket() 函数来创建套接字.

1) af 为地址族（Address Family），也就是 IP 地址类型，常用的有 AF_INET 和 AF_INET6。AF 是“Address Family”的简写，INET是“Inetnet”的简写。AF_INET 表示 IPv4 地址，例如 127.0.0.1；AF_INET6 表示 IPv6 地址，例如 1030::C9B4:FF12:48AA:1A2B。
大家需要记住127.0.0.1，它是一个特殊IP地址，表示本机地址，后面的教程会经常用到。
你也可以使用 PF 前缀，PF 是“Protocol Family”的简写，它和 AF 是一样的。例如，PF_INET 等价于 AF_INET，PF_INET6 等价于 AF_INET6。
2) type 为数据传输方式/套接字类型，常用的有 SOCK_STREAM（流格式套接字/面向连接的套接字） 和 SOCK_DGRAM（数据报套接字/无连接的套接字），我们已经在《套接字有哪些类型》一节中进行了介绍。
3) protocol 表示传输协议，常用的有 IPPROTO_TCP 和 IPPTOTO_UDP，分别表示 TCP 传输协议和 UDP 传输协议。
*/

int tcp_main(void)
{
    int ss;
    struct sockaddr_in local;
    signal(SIGINT,sig_int);

    ss=socket(AF_INET,SOCK_STREAM,0);
    printf("ss=%d\n",ss);

    memset(&local,0,sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr=htonl(INADDR_ANY);
    local.sin_port = htons(SERVER_PORT);

    int res = bind(ss,(struct sockaddr*)&local,sizeof(local));
    printf("bind res=%d\n",res);

    res = listen(ss,BACKLOG);
    printf("listen res=%d\n",res);

    pid_t pid[PIDNUMB];
    int i = 0;
    for(i =0;i<PIDNUMB;i++)
    {
        pid[i]=fork();
        if(pid[i]==0)
        {
            tcp_handle_connect(ss);
        }
    }
    while(1);

    return 0;
}

int main(void)
{
    tcp_main();

    return 0;
}
