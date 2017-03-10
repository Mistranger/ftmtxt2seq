cmake_minimum_required(VERSION 3.5)
project(ftmtxt2seq)

set(CMAKE_CXX_STANDARD 11)

set(SOURCE_FILES
        ftmtxt.cpp
        ftmtxt.h
        main.cpp
        itfile.cpp
        itfile.h
        outputfile.cpp
        outputfile.h
        util.cpp
        util.h)

add_executable(ftmtxt2seq ${SOURCE_FILES})