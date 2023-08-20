#ifndef SEMAPHORE_H
#define SEMAPHORE_H
#include"semaphore.h"
#include"exception"
class semaphore
{
private:
    sem_t sem;
public:
    semaphore(/* args */);
    ~semaphore();
    bool wait();
    bool post();
    bool getValue(int *sval);
};

semaphore::semaphore(/* args */)
{
    if(sem_init(&sem,0,0) != 0){
        throw std::exception();
    }
}

semaphore::~semaphore()
{
    if(sem_destroy(&sem) != 0){
        throw std::exception();
    }
}

bool semaphore::wait(){
    return sem_wait(&sem) == 0;
}

bool semaphore::post(){
    return sem_post(&sem) == 0;
}

bool semaphore::getValue(int* sval){
    return sem_getvalue(&sem,sval) == 0;
}

#endif