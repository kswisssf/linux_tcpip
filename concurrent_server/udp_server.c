/*
linux并发服务器(concurrent server)
1.UDP并发服务器(fork方式）
	1.1.运行模型
	1.2.示例
	1.3.客户端发送
	1.4.运行结果
	1.5.结果分析

结果分析
1、两个客户端发送数据，fork出来的进程都能进行处理；
2、对于同一个客户端发送的数据，fork出来的进行能够被调度处理到；
可以想象同一个端口存在一个阻塞队列。备注：在早期的linux版本，存在惊群问题，即会唤醒队列中所有进程，造成性能问题，貌似新版本内核解决方案为只唤醒等待队列上的第一个进程或者线程。
3、fork出来的进程中的同一个函数内变量的地址数值相同，但互补影响（在各自的进程空间中）

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

static void handle_connect(int s)
{
    struct sockaddr_in from;
    socklen_t len = sizeof(from);
    int n=0;
    char buff[BUFFLEN];
    time_t now;
    int count = 0;

    printf("handle_connect s=%d buff=%p,count addr=%p,pid=0x%x\n",s,buff,&count,getpid());
    while(1)
    {
        memset(buff,0,BUFFLEN);
        n=recvfrom(s,buff,BUFFLEN,0,(struct sockaddr*)&from,&len);
        if(n>0)
        {
             printf("recv from port=%u data=[%s],pid=0x%x,count=%d\n",ntohs(from.sin_port),buff,getpid(),++count);
             memset(buff,0,BUFFLEN);
             now=time(NULL);
             sprintf(buff,"[%24s]\r\n",ctime(&now));
             sendto(s,buff,strlen(buff),0,(struct sockaddr*)&from,len);
        }
        else
        {
            printf("n=%d,buff=[%s],pid=0x%x\n",n,buff,getpid());
        }
        sleep(2);
    }

}
void sig_int(int num)
{
    exit(1);
}

int main(void)
{
    int ss;
    struct sockaddr_in local;
    signal(SIGINT,sig_int);

    ss=socket(AF_INET,SOCK_DGRAM,0);
    printf("ss=%d\n",ss);

    memset(&local,0,sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr=htonl(INADDR_ANY);
    local.sin_port = htons(SERVER_PORT);

    int res = bind(ss,(struct sockaddr*)&local,sizeof(local));
    printf("bind res=%d\n",res);

    pid_t pid[PIDNUMB];
    int i = 0;
    for(i =0;i<PIDNUMB;i++)
    {
        pid[i]=fork();
        if(pid[i]==0)
        {
            handle_connect(ss);
        }
        else
        {
            printf("pid=%x\n",pid[i]);
        }
    }
    while(1);

    return 0;
}
