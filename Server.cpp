#include "Server.hpp"

Server::Server(int port_num) : port_num(port_num) {
  hasError = 0;
  if (setUpStruct() == -1){
    hasError = 1;
  }
  if (initSocketFd() == -1){
    hasError = 1;
  }
  if (tryBind() == -1){
    hasError = 1;
  }
  if (startListen() == -1){
    hasError = 1;
  }
}

int Server::setUpStruct(){
  int status;
  memset(&host_info, 0, sizeof(host_info));

  host_info.ai_family   = AF_UNSPEC;
  host_info.ai_socktype = SOCK_STREAM;
  host_info.ai_flags    = AI_PASSIVE;
  std::string port_str = std::to_string(port_num);
  status = getaddrinfo(nullptr, port_str.c_str(), &host_info, &host_info_list);
  if (status != 0) {
    // "Error: cannot get address info for host"
    return -1;
  } //if
  return status;
}

int Server::initSocketFd(){
  socket_fd = socket(host_info_list->ai_family, 
		     host_info_list->ai_socktype, 
		     host_info_list->ai_protocol);
  if (socket_fd == -1) {
    // "Error: cannot create socket" << endl;
    return -1;
  } //if
  return socket_fd;
}

int Server::tryBind(){
  int status;
  int yes = 1;
  status = setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
  status = bind(socket_fd, host_info_list->ai_addr, host_info_list->ai_addrlen);
  if (status == -1) {
    // "Error: cannot bind socket" << endl;
    return -1;
  } //if
  return status;
}

int Server::startListen(){
    int status;
    status = listen(socket_fd, 100);
  if (status == -1) {
    // "Error: cannot listen on socket" 
    return -1;
  } //if
  freeaddrinfo(host_info_list);
  return status;
}


int Server::tryAccept(){
    struct sockaddr_storage socket_addr;
    socklen_t socket_addr_len = sizeof(socket_addr);
    client_connection_fd = accept(socket_fd, (struct sockaddr *)&socket_addr, &socket_addr_len);
    if (client_connection_fd == -1) {
      // "Error: cannot accept connection on socket"
      return -1;
    } //if
    return client_connection_fd;
}

int Server::getErrorSign(){
  return hasError;
}

std::string Server::recvData(int flag){
  // TODO: select function
  char recvbuff[MAX_TCP_PACKET_SIZE];

  int numbytes;

  if ((numbytes = recv(client_connection_fd, recvbuff, MAX_TCP_PACKET_SIZE - 1, flag)) == -1) {
      hasError = 1;
      return nullptr;
  }
  recvbuff[numbytes] = '\0';
  return std::string(recvbuff);
}

void Server::sendData(void* data, size_t dataSize, int flag){
  int status;
  status = send(client_connection_fd, data, dataSize, flag); 
  if (status == -1){
    hasError = 1;
  }
}
