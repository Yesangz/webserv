#ifndef TIMER_H
#define TIMER_H

#include"time.h"

class timer
{
private:
    /* data */
public:
    time_t expire;

    timer(/* args */);
    ~timer();
};

timer::timer(/* args */)
{
}

timer::~timer()
{
}


#endif