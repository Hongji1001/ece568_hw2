#include "Request.hpp"

std::string Request::getRequestLine(){
    return raw_request_line;
}


int Request::getClientFd(){
    return client_connection_fd;
}

int Request::getRequestID() const{
    return request_id;
}

std::string Request::getClientIP() const{
    return client_connection_IP;
}

void* Request::get_in_addr(struct sockaddr *sa){
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}





