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

static int next_request_id = 0;


class Request{
    private:
        std::string raw_request_line;
        int request_id;
        int client_connection_fd;

    public:
        Request(std::string msg, int socket_fd){
            client_connection_fd = socket_fd;
            raw_request_line = msg;
            request_id = next_request_id;
            next_request_id ++ ;
        }
        std::string getRequestLine();

        int getClientFd();
};