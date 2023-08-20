#ifndef CONDITIONER_H
#define CONDITIONER_H
#include"pthread.h"
#include"exception"
class conditioner
{
private:
    pthread_cond_t cond;
public:
    conditioner(/* args */);
    ~conditioner();
    bool wait(pthread_mutex_t* mutex);
    bool timewait(pthread_mutex_t* mutex, timespec* tm);
    bool broadcast();
    bool siganl();
};

conditioner::conditioner(/* args */)
{
    if(pthread_cond_init(&cond,NULL) != 0){
        throw std::exception();
    }
}

conditioner::~conditioner()
{
    if(pthread_cond_destroy(&cond) != 0){
        throw std::exception();
    }
    
}

bool conditioner::wait(pthread_mutex_t* mutex){
    return pthread_cond_wait(&cond,mutex) == 0;
    
}

bool conditioner::timewait(pthread_mutex_t* mutex, timespec* tm){
    return pthread_cond_timedwait(&cond, mutex, tm) == 0;
    
}

bool conditioner::broadcast(){
    return pthread_cond_broadcast(&cond) == 0;
}

bool conditioner::siganl(){
    return pthread_cond_signal(&cond) == 0;
}


#endif