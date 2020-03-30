/*
2.TCP并发服务器(单客户端单线程，统一accept）
    2.1.示例
    2.2. 客户端发送
    2.3.运行结果
    2.4.Wireshark数据包过程

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
#include <pthread.h>
//#define NULL 0
#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 5
#define PIDNUMB 2

static void *tcp_handle_request(void * argv)
{
    int sc = *((int*)argv);
    printf("[pid=0x%x threadid=0x%lx]tcp_handle_request start sc=%d\n", getpid(),pthread_self(), sc);
    int n=0;
    char buff[BUFFLEN];
    time_t now;
    int count = 0;
    while(1)
    {
        memset(buff,0,BUFFLEN);
        n=recv(sc,buff,BUFFLEN,0);

        if(n>0)
        {
            printf("[pid=0x%x threadid=0x%lx]recv data  sc=%d   data=[%s], count=%d\n",getpid(),pthread_self(), sc ,buff,++count);
            memset(buff,0,BUFFLEN);
            now=time(NULL);
            sprintf(buff,"[%24s]\r\n",ctime(&now));
            send(sc,buff,strlen(buff),0);
        }
        else if(n<=0)
        {
           printf("[pid=0x%x threadid=0x%lx]recv data res=%d errno=%d,msg=%s,close & exit\n",getpid(),pthread_self(), n, errno,strerror(errno));
           break;
        }
    }
    close(sc);
    printf("[pid=0x%x threadid=0x%lx]tcp_handle_request end sc=%d\n", getpid(),pthread_self(), sc);

}

static int tcp_handle_connect(int ss)
{
    printf("[pid=0x%x threadid=0x%lx]tcp_handle_request start ss=%d\n", getpid(),pthread_self(), ss);
    int sc;

    struct sockaddr_in from;
    socklen_t len = sizeof(from);

    pthread_t thread_do;
    while(1)
    {
        sc=accept(ss, (struct sockaddr*)&from, &len);
        if(sc>0)
        {
            static char tempStr[32] = {0};
            inet_ntop(AF_INET, &(from.sin_addr), tempStr, sizeof(tempStr));
            printf("[pid=0x%x]handle_connect ss=%d sc=%d connect from %s:%u\n", getpid(),ss,sc,tempStr,ntohs(from.sin_port));

            int res = pthread_create(&thread_do, NULL, tcp_handle_request, &sc);
            if(res == -1){
               printf("[pid=0x%x]pthread_create   res=%d  ,errno=%d,msg=%s\n",getpid(), res , errno,strerror(errno));

            }
        }
        else if(sc==-1)
        {
            printf("accept ss=%d errno=%d,msg=%s\n", ss, errno, strerror(errno));
            exit(1);
        }
    }

}

int tcp_main(void)
{
    int ss;
    struct sockaddr_in local;

    ss=socket(AF_INET,SOCK_STREAM,0);
    printf("ss=%d\n",ss);

    memset(&local,0,sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr=htonl(INADDR_ANY);
    local.sin_port = htons(SERVER_PORT);

    int res = bind(ss,(struct sockaddr*)&local,sizeof(local));
    if(res==-1)
    {
        printf("bind res=%d,errno=%d,msg=%s\n",res,errno,strerror(errno));
        exit(1);

    }
    printf("bind res=%d\n",res);

    res = listen(ss,BACKLOG);
    if(res==-1)
    {
        printf("listen res=%d,errno=%d,msg=%s\n",res,errno,strerror(errno));
        exit(1);

    }
    printf("listen res=%d\n" ,res);

    tcp_handle_connect(ss);

    return 0;
}

int main(void)
{
    tcp_main();

    return 0;
}
