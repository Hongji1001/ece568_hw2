#ifndef __CLIENT_HPP__
#define __CLIENT_HPP__
#include <iostream>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <cstring>
#include <string>
#include <exception>

#define MAX_TCP_PACKET_SIZE 65535

class Client
{
private:
    unsigned short int serverPort;
    std::string serverHostname;
    int sockfd;

    struct addrinfo *serverAddr;

public:
    // Client(unsigned short int port, const char *hostname);
    Client(unsigned short int port, std::string hostname);
    int getSockfd() const;
    void getAddrinfo();
    void createSocket();
    void createConnect();
    void closeClient();
    void sendRequest(const void *msg, const size_t size);
    std::string recvResponse();
    ~Client();
};

#endif