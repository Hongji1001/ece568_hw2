#include "Request.hpp"

std::string Request::getRequestLine(){
    return raw_request_line;
}


int Request::getClientFd(){
    return client_connection_fd;
}





