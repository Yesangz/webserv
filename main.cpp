#include"cstdlib"
#include"cstdio"
#include"unistd.h"
#include"arpa/inet.h"
#include"sys/time.h"
#include"pthread.h"
#include"exception"
#include"errno.h"
#include<iostream>
#include"locker.hpp"
#include"missions.hpp"
#include"map"
#include"iostream"
#include"sys/epoll.h"
#include"threadpool.hpp"

#define MAX_CLIENT 100

int main(int argc, char* argv[]){
    if(argc <= 1){
        printf("please enter in: %s portnumber\n",argv[0]);
        exit(-1);
    }

    //listen socket create
    int lfd = socket(AF_INET,SOCK_STREAM,0);

    //port reuse
    int reuse = 1;
    setsockopt(lfd,SOL_SOCKET,SO_REUSEADDR,&reuse,sizeof(reuse));

    //server message bind
    struct sockaddr_in saddr;
    saddr.sin_addr.s_addr = INADDR_ANY;
    saddr.sin_family = AF_INET;
    saddr.sin_port = htons(atoi(argv[1]));
    bind(lfd,(sockaddr*)&saddr,sizeof(saddr));

    //epollfd create
    struct epoll_event events[MAX_CLIENT];
    int epollfd = epoll_create(5);
    missions::epollfd = epollfd;

    //listen prepare
    listen(lfd,MAX_CLIENT);
    struct epoll_event listen_event;
    listen_event.data.fd = lfd;
    listen_event.events = EPOLLIN;
    missions::addfd(epollfd,lfd,false);

    //thread pool create
    threadpool<missions> pool;

    //vector for restore missions
    std::map<int,missions> users;

    //定时器
    std::queue<timer> users_timer;
    while (true)
    {
        int num = epoll_wait(epollfd,events,MAX_CLIENT,-1);

        if(num > 0){
            for(int i = 0; i < num; i++){
                int sockfd = events[i].data.fd;
                if(events[i].data.fd == lfd){
                    struct sockaddr_in caddr;
                    socklen_t len = sizeof(caddr);
                    int cfd = accept(lfd,(struct sockaddr*)&caddr,&len);
                    if(cfd == -1){
                        perror("accept");
                        throw std::exception();
                    }
                    if(missions::user_count >= MAX_CLIENT){
                        close(cfd);
                        continue;
                    }
                    if(users.find(cfd) == users.end())
                    {
                        missions user;
                        user.init(cfd,caddr);
                        user.addfd(epollfd,cfd,true);
                        users.insert(std::make_pair(cfd,user));
                        // pool.JobAppend(&user);这里只是接受连接，要把任务加入监听队列，而不是加入线程池，所以不需要jobAppend
                        user.user_count++;
                        //timer
                        printf("request enter queue, fd: %d\n",cfd);
                    }
                }
                else if(events[i].events & (EPOLLRDHUP| EPOLLHUP | EPOLLERR)){
                    printf("event detected error\n");
                    if(users.find(sockfd) != users.end()){
                        printf("connect closed\n");
                        users[sockfd].close_conn();
                        users.erase(sockfd);
                    }
                }
                else if(events[i].events & EPOLLIN){
                    //检测到epollin之后，先查看是否存在该文件描述符，并且是连接状态
                    if(users.find(sockfd) != users.end()){
                        if(users[sockfd].connected && users[sockfd].read())
                        {
                            pool.JobAppend(&users[sockfd]);
                            printf("%d jobappend\n",sockfd);
                        }
                    }
                }
                else if(events[i].events & EPOLLOUT){
                    if(users.find(sockfd) != users.end()){
                        if(!users[sockfd].write())
                        {
                            users[sockfd].close_conn();
                            users.erase(sockfd);
                            printf("%d connection closed\n",sockfd);
                        }
                        printf("%d write successed\n",sockfd);
                    }
                }
            }
        }
        else if(num == -1){
            break;
        }
    }
    close(epollfd);
    close(lfd);
    users.clear();
    
}