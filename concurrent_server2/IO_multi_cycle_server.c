/*
3. TCP并发服务器(IO复用循环服务器）
	3.1.概述
	3.2.示例
	3.3.客户端发送
	3.4.Wireshark数据包过程
	3.5.运行结果
	3.6.相关说明
3. TCP并发服务器(IO复用循环服务器）
3.1.概述
与前面的方案，当客户端连接变多时，会新创建连接相同个数的进程或者线程，当此数值比较大时，如上千个连接，
此时线程/进程资料存储占用，以及CPU在上千个进程/线程之间的时间片调度成本凸显，造成性能下降。需要一种
新的模型来解决，select IO模型即是一种方案。

*/
#include <stdio.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <string.h>
#include <netinet/in.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "errno.h"
#include "signal.h"
#include <stdlib.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/select.h>

#ifndef NULL
#define NULL 0
#endif

#define MAX_MSG_LEN 1024
#define BUFFLEN 1024
#define SERVER_PORT 8888
#define BACKLOG 5
#define PIDNUMB 2
#define CLIENTNUM 5 //最大支持客户端数量
int connect_host[CLIENTNUM];
int connect_number=0;

void printMsg(const char* pcFmt, ...);

static void * handle_requst(void * argv)
{
    printMsg("[pid=0x%x threadid=0x%lx]handle_requst start  \n", getpid(),pthread_self() );

    int n=0;
    char buff[BUFFLEN]={0};
    time_t now;

    int maxfd=-1;//最大侦听文件描述符
    fd_set scanfd;//侦听描述符集合

    int i=0;
    int res = -1;

    struct timeval timeout;//超时时间
    for (;;)
    {
        maxfd = -1;
        FD_ZERO(&scanfd);
        //将已经建立的连接描述符添加到侦听描述符集合中，并获取最大的描述符
        int totalfdset=0;
        for(i=0;i<CLIENTNUM;i++)
        {
            if(connect_host[i]!=-1)
            {
                //printMsg("scanfd set fds:%d\n",connect_host[i]);
                FD_SET(connect_host[i],&scanfd);
                if(maxfd<connect_host[i])
                {
                    maxfd=connect_host[i];
                }
                totalfdset++;
            }
        }
        if(totalfdset<=0)
        {
            sleep(1);
            continue;
        }
        //printMsg("totalfdset=%d,maxfd=%d\n",totalfdset,maxfd);
        //timeout数值会被select改写，每次循环调用需要重新赋值，否则变成立即返回效果
        timeout.tv_sec = 1;
        timeout.tv_usec = 0;
        res = select(maxfd+1,&scanfd,NULL,NULL,&timeout);
        switch (res) {
        case 0:
            //printMsg("[pid=0x%x threadid=0x%lx]handle_requst select res=%d timeout \n", getpid(),pthread_self() ,res);
            break;
        case -1:
            printMsg("[pid=0x%x threadid=0x%lx]handle_requst select res=%d  err,errno=%d,msg=%s\n", getpid(),pthread_self() ,res,errno,strerror(errno));

            break;
        default:
            printMsg("[pid=0x%x threadid=0x%lx]handle_requst select res=%d  read event  \n", getpid(),pthread_self() ,res );

            if(connect_number<=0)
            {
                printMsg("[pid=0x%x threadid=0x%lx]handle_requst select res=%d ,connect_number<=0 ,break; \n", getpid(),pthread_self() ,res );
                break;

            }
            //查找激活的文件描述符
            for(i=0;i<CLIENTNUM;i++)
            {
                if(connect_host[i]!=-1)
                {
                    //是匹配的有效的文件描述符
                    if(FD_ISSET(connect_host[i],&scanfd))
                    {
                        memset(buff,0,BUFFLEN);
                        n = recv(connect_host[i],buff,BUFFLEN,0);
                        if(n>0 && !strncmp(buff,"TIME",4))
                        {
                            printMsg("[pid=0x%x threadid=0x%lx]recv data  sc=%d   data=[%s]\n",getpid(),pthread_self(), connect_host[i] ,buff);
                            memset(buff,0,BUFFLEN);
                            now=time(NULL);
                            sprintf(buff,"[%24s]\r\n",ctime(&now));
                            send(connect_host[i],buff,strlen(buff),0);
                        }
                        else if(n<=0)
                        {
                            printMsg("[pid=0x%x threadid=0x%lx]recv data res=%d errno=%d,msg=%s,close\n",getpid(),pthread_self(), n, errno,strerror(errno));
                            close(connect_host[i]);
                            connect_host[i] = -1;
                            connect_number--;
                            break;
                        }
                    }
                }
            }
            break;
        }

    }
    return 0;
}

static void *handle_connect(void *argv)
{
    int ss=*((int*)argv);

    printMsg("[pid=0x%x threadid=0x%lx]handle_connect start ss=%d\n", getpid(), pthread_self(), ss);
    struct sockaddr_in from;
    socklen_t len = sizeof(from);

    while(1)
    {
        //接收连接
        int sc = accept(ss, (struct sockaddr*)&from, &len);
        if(sc>0)
        {
            static char tempStr[32] = {0};
            inet_ntop(AF_INET, &(from.sin_addr), tempStr, sizeof(tempStr));
            printMsg("[pid=0x%x  threadid=0x%lx]handle_connect ss=%d sc=%d connect from %s:%u\n", getpid(),pthread_self(),ss,sc,tempStr,ntohs(from.sin_port));
            int i = 0;
            for(i=0; i < CLIENTNUM; i++)
            {
                if(connect_host[i]==-1)
                {
                    //客户端连接未已经达到上线，则记录连接
                    connect_host[i]=sc;
                    connect_number++;
                    printMsg("[pid=0x%x  threadid=0x%lx]client queue is set ,count=%d,sc=%d\n",getpid(),pthread_self(),connect_number,sc);
                    break;
                }
            }
            if(i>=CLIENTNUM)
            {
                //客户端连接已经达到上线，则主动关闭连接
                printMsg("[pid=0x%x  threadid=0x%lx]client queue is full ,count=%d,cannot add ,close sc=%d\n",getpid(),pthread_self(),connect_number,sc);
                close(sc);
            }
        }
        else if(sc==-1)
        {
            printMsg("accept error sc=%d errno=%d,msg=%s\n",sc,errno,strerror(errno));
            exit(1);
        }
    }

    return 0;
}

int main(void)
{
    int ss;
    struct sockaddr_in local;

    ss=socket(AF_INET, SOCK_STREAM, 0);
    printMsg("ss=%d\n",ss);

    memset(&local, 0, sizeof(local));
    local.sin_family = AF_INET;
    local.sin_addr.s_addr=htonl(INADDR_ANY);
    local.sin_port = htons(SERVER_PORT);

    //设置端口复用
    int opt = 1;
    errno = 0;
    int res = setsockopt(ss, SOL_SOCKET, SO_REUSEADDR, (const void *)&opt, sizeof(opt));
    if(res == -1)
    {
        printMsg("set SO_REUSEADDR error, errno=%d,msg=%s\n", errno,strerror(errno));
    }


    res = bind(ss, (struct sockaddr*)&local, sizeof(local));
    if(res==-1)
    {
        printMsg("bind res=%d,errno=%d,msg=%s\n",res,errno,strerror(errno));
        exit(1);

    }
    printMsg("bind res=%d\n",res);

    res = listen(ss, BACKLOG);
    if(res==-1)
    {
        printMsg("listen res=%d,errno=%d,msg=%s\n",res,errno,strerror(errno));
        exit(1);

    }
    printMsg("listen res=%d\n" ,res);

    //原书是memset(connect_host,-1,CLIENTNUM);，不能初始化完全，此函数是按字节进行设置的
    memset(connect_host, -1, CLIENTNUM*sizeof(connect_host[0]));

    pthread_t thread_do_connect;
    pthread_t thread_do_request;

    res = pthread_create(&thread_do_connect,NULL,handle_connect,(void*)&ss);
    printMsg("pthread_create thread_do_connect res=%d\n",res);
    res = pthread_create(&thread_do_request,NULL,handle_requst,NULL);
    printMsg("pthread_create thread_do_request res=%d\n",res);

    res = pthread_join(thread_do_connect,NULL);
    printMsg("pthread_join thread_do_connect res=%d\n",res);
    res = pthread_join(thread_do_request,NULL);
    printMsg("pthread_join thread_do_request res=%d\n",res);

    res = close(ss);
    printMsg("close ss res=%d\n",res);
    
    return 0;
}


void printMsg(const char* pcFmt, ...)
{
    va_list     vList;
    char    acBuf[MAX_MSG_LEN];
    va_start(vList, pcFmt);
    va_end(vList);
    vsnprintf(acBuf, MAX_MSG_LEN, pcFmt, vList);

    struct tm *tm_now;
    struct timeval nowtime;
    gettimeofday(&nowtime,0);
    tm_now = localtime(&nowtime.tv_sec);
    printf("[%04d%02d%02d% 02d:%02d:%02d.%03d]%s", tm_now->tm_year+1900, tm_now->tm_mon+1, tm_now->tm_mday,
           tm_now->tm_hour, tm_now->tm_min, tm_now->tm_sec,nowtime.tv_usec/1000,
           acBuf);

}
