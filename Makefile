#**********************
#*
#* Progam Name: MP1. Membership Protocol.
#*
#* Current file: Makefile
#* About this file: Build Script.
#* 
#***********************

CFLAGS =  -Wall -g -std=c++11 -w

all: Application

Application: MP1Node.o EmulNet.o Application.o Log.o Params.o Member.o  
	g++ -o bin/Application bin/MP1Node.o bin/EmulNet.o bin/Application.o bin/Log.o bin/Params.o bin/Member.o ${CFLAGS}

MP1Node.o: MP1Node.cpp MP1Node.h Log.h Params.h Member.h EmulNet.h Queue.h
	g++ -o bin/MP1Node.o -c MP1Node.cpp ${CFLAGS}

EmulNet.o: EmulNet.cpp EmulNet.h Params.h Member.h
	g++ -o bin/EmulNet.o -c EmulNet.cpp ${CFLAGS}

Application.o: Application.cpp Application.h Member.h Log.h Params.h Member.h EmulNet.h Queue.h 
	g++ -o bin/Application.o -c Application.cpp ${CFLAGS}

Log.o: Log.cpp Log.h Params.h Member.h
	g++ -o bin/Log.o -c Log.cpp ${CFLAGS}

Params.o: Params.cpp Params.h 
	g++ -o bin/Params.o -c Params.cpp ${CFLAGS}

Member.o: Member.cpp Member.h
	g++ -o bin/Member.o -c Member.cpp ${CFLAGS}

clean:
	rm -rf bin/*
