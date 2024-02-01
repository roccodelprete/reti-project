cmake_minimum_required(VERSION 3.22.1)
project(reti_project C)

set(CMAKE_C_STANDARD 11)

add_executable(reti_project server.c
        segreteria.c
        studente.c)


