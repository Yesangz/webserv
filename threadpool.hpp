#ifndef THREADPOOL_H
#define THREADPOOL_H
#include"cstdlib"
#include"cstdio"
#include"unistd.h"
#include"arpa/inet.h"
#include"sys/time.h"
#include"errno.h"
#include"locker.hpp"
#include"conditioner.hpp"
#include"semaphore.hpp"
#include"queue"
template<typename T>
class threadpool
{
private:
    int MAX_thread = 8;
    int MAX_request = 40;
    pthread_t* threads;
    bool t_end;
    semaphore sem;
    locker lock;
    std::queue<T*> jobQueue;
public:
    threadpool();
    ~threadpool();
    bool JobAppend(T* request);
    void run();
    static void* worker(void* arg);
};

template<typename T>
threadpool<T>::threadpool(/* args */)
{
    threads = new pthread_t[MAX_thread];
    for(int i = 0; i < MAX_thread; i++){
        printf("creat the %dth thread\n",i);
        if (pthread_create(&threads[i],NULL,worker,this)!= 0 && pthread_detach(threads[i]) != 0){
            delete[] threads;
            throw std::exception();
        }
    }
    t_end = false;
}
template<typename T>
threadpool<T>::~threadpool()
{
    delete[] threads;
}

template<typename T>
void* threadpool<T>::worker(void* arg)
{
    threadpool* pool = (threadpool*)arg;
    pool->run();
    return (void*)pool;
}

template<typename T>
void threadpool<T>::run()
{
    while (!t_end)
    {
        sem.wait();
        lock.lock();
        if(jobQueue.empty()){
            lock.unlock();
            continue;
        }
        T* job = jobQueue.front();
        jobQueue.pop();
        job->process();
        lock.unlock();
    }
}

template<typename T>
bool threadpool<T>::JobAppend(T* request)
{
    lock.lock();
    if(jobQueue.size() >= MAX_request){
        lock.unlock();
        return false;
    }
    jobQueue.push(request);
    sem.post();
    lock.unlock();
    // printf("request append\n");
    return true;
}

#endif