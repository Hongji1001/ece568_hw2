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
        std::string client_connection_IP;

    public:
        Request(std::string msg, int socket_fd){
            client_connection_fd = socket_fd;
            raw_request_line = msg;
            request_id = next_request_id;
            next_request_id ++ ;

            // get ip
            struct sockaddr_storage socket_addr;
            socklen_t socket_addr_len = sizeof(socket_addr);
            char ipAddress[INET6_ADDRSTRLEN] = {0};
            getsockname(socket_fd, (struct sockaddr*)&socket_addr, &socket_addr_len);//获取sockfd表示的连接上的本地地址
            inet_ntop(socket_addr.ss_family,
                    get_in_addr((struct sockaddr *)&socket_addr),
                    ipAddress, sizeof ipAddress);
            client_connection_IP = std::string(ipAddress, strlen(ipAddress));
        }
        std::string getRequestLine();

        int getClientFd();
        int getRequestID() const;
        void *get_in_addr(struct sockaddr *sa);
        std::string getClientIP() const;
};