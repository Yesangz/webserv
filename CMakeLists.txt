cmake_minimum_required(VERSION 3.25)
project(webserver)

set(CMAKE_CXX_STANDARD 14)

include_directories(.)

add_executable(webserver
        conditioner.hpp
        locker.hpp
        main.cpp
        missions.hpp
        semaphore.hpp
        threadpool.hpp)
