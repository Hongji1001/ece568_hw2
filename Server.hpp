#include <cstdio>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>
#include <string>
#include <stdlib.h>
#include <string.h>

#define MAX_TCP_PACKET_SIZE 65535

class Server{
    private:
        int hasError;
        int socket_fd;
        int client_connection_fd;
        int port_num;
        struct addrinfo host_info;
        struct addrinfo *host_info_list;
        
    public:
    explicit Server(int port_num);

    // create a socket to listen
    int setUpStruct();
    int initSocketFd();
    int tryBind();
    int startListen();

    // try accept
    int tryAccept();
    // send and recv data
    char* recvData(int flag);
    void sendData(void* data, int flag);

    int getErrorSign();

    // int connectToServer();

};