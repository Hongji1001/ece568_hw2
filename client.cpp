#include "client.hpp"

Client::Client(unsigned short int port, std::string hostname) : serverPort(port)
{
    serverHostname = hostname;
    try
    {
        getAddrinfo();
        createSocket();
        createConnect();
    }
    catch (const std::exception &e)
    {
    }
    freeaddrinfo(serverAddr);
}
int Client::getSockfd() const
{
    return sockfd;
}

void Client::getAddrinfo()
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    if (getaddrinfo(serverHostname.c_str(), std::to_string(serverPort).c_str(), &hints, &serverAddr) != 0)
    {
        std::perror("failed: client cannot get addr info.");
        throw std::exception();
    }
}

void Client::createSocket()
{
    if ((sockfd = socket(serverAddr->ai_family, serverAddr->ai_socktype, serverAddr->ai_protocol)) == -1)
    {
        std::perror("failed: client cannot create socket.");
        throw std::exception();
    }
}
void Client::createConnect()
{
    if (connect(sockfd, serverAddr->ai_addr, serverAddr->ai_addrlen) == -1)
    {
        perror("failed: client cannot connect server.");
        throw std::exception();
    }
}

void Client::closeClient()
{
    if (close(sockfd) != 0)
    {
        perror("failed: client cannot close socket.");
        throw std::exception();
    }
}

void Client::sendRequest(const void *msg, const size_t size)
{
    int numBytes = 0;
    int recvBytes = 0;
    while ((numBytes < size))
    {
        if ((recvBytes = send(sockfd, msg, size, 0)) == -1)
        {
            perror("client send");
            throw std::exception();
        }
        numBytes += recvBytes;
    }
}

std::string Client::recvResponse()
{
    // std::vector<char> buff(MAX_TCP_PACKET_SIZE);
    // std::string ans;
    // int numBytes = 0;
    // if ((numBytes = recv(sockfd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0)) == -1)
    // {
    //     perror("client recv");
    //     throw std::exception();
    // }

    // for (int i = 0; i < numBytes; i ++ ){
    //     ans += buff[i];
    // }
    // ans += '\0';
    // return ans;

    char buf[MAX_TCP_PACKET_SIZE];
    int numBytes = 0;
    if ((numBytes = recv(sockfd, buf, sizeof(buf), 0)) == -1)
    {
        perror("client recv");
        throw std::exception();
    }
    // buf[numBytes] = '\0';
    return std::string(buf, numBytes);
}

Client::~Client()
{
    try
    {
        closeClient();
    }
    catch (const std::exception &e)
    {
    }
}
