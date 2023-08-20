#ifndef LOCKER_H
#define LOCKER_H
#include"pthread.h"
#include"exception"
class locker
{
private:
    pthread_mutex_t mutex;
public:
    locker();
    ~locker();
    bool lock();
    bool unlock();
};

locker::locker()
{
    if(pthread_mutex_init(&mutex,nullptr) != 0){
        throw std::exception();
    }
}

locker::~locker()
{
    if(pthread_mutex_destroy(&mutex) != 0){
        throw std::exception();
    }
}

bool locker::lock(){
    if(pthread_mutex_lock(&mutex) != 0){
        throw std::exception();
    }
}

bool locker::unlock(){
    if(pthread_mutex_unlock(&mutex) != 0 ){
        throw std::exception();
    }
}


#endif