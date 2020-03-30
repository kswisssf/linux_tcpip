/*
1.TCP并发服务器(单客户端单进程，统一accept）
    1.1.示例
    1.2. 客户端发送
    1.3.运行结果
    1.4.相关说明
    
1.4.相关说明
1、当客户端主动挥手断开连接，发送FIN信号过来时，服务端（本测试中服务端运行在ubuntu18.04）recv调用会返回0，TCP状态会处于CLOSE_WAIT.
    $ netstat -tap | grep 8888
    (Not all processes could be identified, non-owned process info
    will not be shown, you would have to be root to see it all.)
    tcp 0 0 0.0.0.0:8888 0.0.0.0:* LISTEN 7436/test0001
    tcp 0 0 armlinux:8888 192.168.10.2:4001 CLOSE_WAIT 7488/test0001
    此时需要应用层主动关闭，发送FIN给客户端，切换到LAST_ACK，当接收到客户端应答的ACK后完成最终关闭；
    $ netstat -tap | grep 8888
    (Not all processes could be identified, non-owned process info
    will not be shown, you would have to be root to see it all.)
    tcp 0 0 0.0.0.0:8888 0.0.0.0:* LISTEN 7873/test0001

2、客户端（本测试中客户端运行在windows）在主动断开连接时，接收到服务器的FIN和ACK后，进一步发送ACK，并切换到TIME_WAIT状态，
一般需要等待2MSL超时才能完成最终关闭（通过调整相关系统参数，可以让系统快速关闭，具体自行搜索资料）；
    netstat -anto | grep 4001
    TCP 192.168.10.2:4001 192.168.10.40:8888 TIME_WAIT 0 InHost
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

static void tcp_handle_request(int sc)
{
    printf("[pid=0x%x]tcp_handle_request start sc=%d\n", getpid(), sc);
    int n=0;
    char buff[BUFFLEN];
    time_t now;
    int count = 0;

    while(1)
    {
        memset(buff,0,BUFFLEN);
        n=recv(sc,buff,BUFFLEN,0);

        if(n>0 && !strncmp(buff,"TIME",4))
        {
            printf("[pid=0x%x]recv data  sc=%d   data=[%s], count=%d\n",getpid(), sc ,buff,++count);
            memset(buff,0,BUFFLEN);
            now=time(NULL);
            sprintf(buff,"[%24s]\r\n",ctime(&now));
            send(sc,buff,strlen(buff),0);
        }
        else if(n<=0)
        {
           printf("[pid=0x%x]recv data res=%d errno=%d,msg=%s,close & exit\n",getpid(), n, errno,strerror(errno));
           break;
        }
    }
    close(sc);
    printf("[pid=0x%x]tcp_handle_request end sc=%d\n", getpid(), sc);

}

static int tcp_handle_connect(int ss)
{
    printf("[pid=0x%x]tcp_handle_request start ss=%d\n", getpid(), ss);
    int sc;

    struct sockaddr_in from;
    socklen_t len = sizeof(from);

    while(1)
    {
        sc=accept(ss,(struct sockaddr*)&from,&len);
        if(sc>0)
        {
            static char tempStr[32] = {0};
            inet_ntop(AF_INET,&(from.sin_addr),tempStr,sizeof(tempStr));
            printf("[pid=0x%x]handle_connect ss=%d sc=%d connect from %s:%u\n", getpid(),ss,sc,tempStr,ntohs(from.sin_port));

            if(fork()>0){
                int res = close(sc);
                if(res==-1)
                {
                   printf("[pid=0x%x]close sc res=%d close,errno=%d,msg=%s\n",getpid(), res,sc, errno,strerror(errno));

                }
            }
            else{
                tcp_handle_request(sc);
            }
        }
        else if(sc==-1)
        {
            printf("accept ss=%d errno=%d,msg=%s\n",ss,errno,strerror(errno));
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

