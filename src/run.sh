#!/bin/bash

mkdir bin
cd bin

clear

g++ -rdynamic -w -g -std=c++11 -c ../executer.cpp
g++ -rdynamic -w -g -std=c++11 -c ../GIL2.cpp
g++ -rdynamic -w -g -std=c++11 -c ../lock_queue.cpp
g++ -rdynamic -w -g -std=c++11 -c ../GC.cpp
g++ -rdynamic -w -g -std=c++11 -c ../exceptions.cpp
g++ -rdynamic -w -g -std=c++11 -c ../translator.cpp
g++ -rdynamic -w -g -std=c++11 -c ../parser.cpp
g++ -rdynamic -w -g -std=c++11 -c ../ast.cpp
g++ -rdynamic -w -g -std=c++11 -c ../main_test.cpp

cd ../

g++ -rdynamic -w -g -std=c++11 bin/* -ldl -o test && valgrind --leak-check=full --track-origins=yes ./test