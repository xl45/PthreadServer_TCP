#ifndef PTHREADSERVER_H
#define PTHREADSERVER_H

#include <iostream>
#include <fstream>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <string>
#include <stdio.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/select.h>

#define RECV_BUF_MAX_SIZE 1024

typedef unsigned char BYTE;

class PthreadServer{

    public:
        PthreadServer(std::string port);
        void acceptConnections();
        virtual ~PthreadServer();

    private:
        struct addrinfo hints;
        struct addrinfo *serverinfo;
        int serverFD;
        int maxFD;
        timeval tv;
        // hold the incoming connection addr
        sockaddr_in client_addr;
        // thread array
        pthread_t thread[20];
};

#endif // PTHREADSERVER_H
