#include "Proxy.hpp"


void Proxy::startRun(){
    // init as server and start listening 
    std::cout << "start Run server " << std::endl;
    Server server = Server(port_num);
    proxyLog.openLogFile("/var/log/erss/proxy.log");  // TODO: Try Catch
    // check whether proxy server can init successful
    if (server.getErrorSign() == -1){
        // how to exit gracefully return nullptr
        std::cout << "can not init as a server";
        exit(EXIT_FAILURE);
    }
    std::cout << "inited server " << std::endl;
    // start getting request from client
    while(true){
        int client_connection_fd = server.tryAccept();
        if (client_connection_fd == -1){
            std::cout << "fail to accept" << std::endl;
            exit(EXIT_FAILURE);
        }
        if (server.getErrorSign() == -1){
            std::cout << "can not init as a server";
            exit(EXIT_FAILURE);
        }
        std::cout << "has accept" << std::endl;
        // recv msg
        std::string msg = server.recvData(0);
        // string msgString(msg, msg + strlen(msg));
        std::cout << msg << std::endl;
        Request* newRequest = new Request(msg, client_connection_fd);
        pthread_t thread;
        pthread_create(&thread, NULL, handle, newRequest);
    }
}


void* Proxy::handle(void* newRequest){
    // 多线程如何才算结束
    std::string rawRequestText = ((Request*)newRequest)->getRequestLine();
    if (rawRequestText.empty() || rawRequestText == "\r" || rawRequestText == "\n" || rawRequestText == "\r\n") return nullptr;
    HttpRequest newHttpRequest = HttpRequest(rawRequestText);
    // start to verify http request
    std::cout << newHttpRequest.getHost() << std::endl;
    std::cout << newHttpRequest.getMethod() << std::endl;
    std::cout << newHttpRequest.getPort() << std::endl;
    std::cout << newHttpRequest.getRequestTarget() << std::endl;
    proxyLog.writeLogFile(getRequstLogLine(newHttpRequest, newRequest, std::string("from_browser_to_proxy")));

    // handle CONNECT Request
    if (newHttpRequest.getMethod() == "CONNECT"){
        handleCONNECT(newHttpRequest, newRequest);
        proxyLog.writeLogFile(std::string("Tunnel closed"));
        return nullptr;
    } else if (newHttpRequest.getMethod() == "POST"){
        std::cout << "Start to handle POST request" << std::endl;
        // handlePOST(newHttpRequest, newRequest);
        handlePOST(newHttpRequest, newRequest);
        return nullptr;
    } else{
        // GET Request
        // if httprequest has cached
        std::cout << "Start to handle GET request" << std::endl;
        // did not cache
        handleGET(newHttpRequest, newRequest);
        return nullptr;
    }
    // if httprequest has cached
    return nullptr;
}

void Proxy::handleCONNECT(HttpRequest newHttpRequest, void* newRequest){
    // Receive HTTP CONNECT request from client

    // Parse the request line to get target server ip & port from it
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    // const char* webserver_hostname = newHttpRequest.getHost().c_str();
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;  // 确定port是443
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;

    // Make a new socket, connect it to the target server ip & port (make sure port is 443 for CONNECT)
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());
    int proxy_to_webserver_fd = proxy_own_client.getSockfd();
    int browser_to_proxy_fd = ((Request*)newRequest)->getClientFd();

    // Send exactly this response "HTTP/1.1 200 OK\r\n\r\n" back to the client
    const char* ACK_msg = "HTTP/1.1 200 OK\r\n\r\n";
    // proxyLog.writeLogFile("Responding" + to_string("HTTP/1.1 200 OK")));
    sendMsgFromProxy(browser_to_proxy_fd, ACK_msg, strlen(ACK_msg));
    // Then use non-blocking I/O 
    // (e.g. "select") to receive and send bytes back and forth between the client/server.
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 1;
    while (true){
        std::vector<char> buff(MAX_TCP_PACKET_SIZE);
        int n = std::max(proxy_to_webserver_fd, browser_to_proxy_fd) + 1;
        FD_ZERO(&readfds);
        FD_SET(proxy_to_webserver_fd, &readfds);
        FD_SET(browser_to_proxy_fd, &readfds);
        int rv = select(n, &readfds, NULL, NULL, &tv);
        int numbytes;
        if (rv == -1){
            perror("select"); // error occurred in select()
            break;
        } else if (rv == 0){
            std::cout << "Timeout occurred! No data after 10.5 seconds." << std::endl;
            break;
        } else{
            if (FD_ISSET(proxy_to_webserver_fd, &readfds)){
                // recv from webserver
                numbytes = recv(proxy_to_webserver_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);
                if (numbytes < 0){
                    std::cout << "recv error" << std::endl;
                    break;
                }
                if (numbytes == 0){
                    std::cout << "recv finished" << std::endl;
                    break;
                }
                // std::string recv_data_from_webserver = recvMsgInProxy(proxy_to_webserver_fd){}
                // send data back to browser
                numbytes = send(browser_to_proxy_fd, buff.data(), numbytes, 0);
                if (numbytes < 0){
                    std::cout << "send error" << std::endl;
                    break;
                }
            }
            if(FD_ISSET(browser_to_proxy_fd, &readfds)){
                // recv request from browser
                numbytes = recv(browser_to_proxy_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);
                if (numbytes < 0){
                    std::cout << "recv error" << std::endl;
                    break;
                }
                if (numbytes == 0){
                    std::cout << "recv finished" << std::endl;
                    break;
                }
                // send data to webserver
                numbytes = send(proxy_to_webserver_fd, buff.data(), numbytes, 0);
                if (numbytes < 0){
                    std::cout << "send error" << std::endl;
                    break;
                }
            }
            buff.clear();
        }
    }

    // Keep this connection open until one side closes it (i.e. a 'recv' call returns with 0 bytes).
    // close(proxy_to_webserver_fd);
    close(browser_to_proxy_fd);
}

void Proxy::handlePOST(HttpRequest newHttpRequest, void* newRequest){
    // send request to webserver first and get response back
    // get port and hostname of webserver
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());

    // send request to webserver and get HttpResponse
    HttpResponse recvHttpResponse = sendMsgToWebserver(newHttpRequest, newRequest, proxy_own_client);
    proxyLog.writeLogFile(getResponseLogLine(recvHttpResponse, newRequest, newHttpRequest.getHost()));
    // send raw text of httpResponse to browser
    size_t raw_reponse_size = recvHttpResponse.getRawResponseText().size();
    sendMsgFromProxy(((Request*)newRequest)->getClientFd(), recvHttpResponse.getRawResponseText().c_str(), raw_reponse_size);
    close(((Request*)newRequest)->getClientFd());
    return;
}

void Proxy::handleGET(HttpRequest newHttpRequest, void* newRequest){
    // send request to webserver first and get response back
    // get port and hostname of webserver
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());

    // send request to webserver and get HttpResponse
    HttpResponse recvHttpResponse = sendMsgToWebserver(newHttpRequest, newRequest, proxy_own_client);
    if (recvHttpResponse.checkIsChunked()){
        std::cout << "start sending chunked data" << std::endl;
        size_t raw_reponse_size = recvHttpResponse.getRawResponseText().size();
        std::cout << recvHttpResponse.getRawResponseText().c_str() << std::endl;
        sendMsgFromProxy(((Request*)newRequest)->getClientFd(), recvHttpResponse.getRawResponseText().c_str(), raw_reponse_size);
        // continue to recv and send
        while (true){
            std::string temp = proxy_own_client.recvResponse();
            std::cout << temp << std::endl;
            if (temp.empty()) break;
            sendMsgFromProxy(((Request*)newRequest)->getClientFd(), temp.c_str(), temp.size());
        }
        proxyLog.writeLogFile(getResponseLogLine(recvHttpResponse, newRequest, newHttpRequest.getHost()));
    } else{
        // send raw text of httpResponse to browser
        size_t raw_reponse_size = recvHttpResponse.getRawResponseText().size();
        sendMsgFromProxy(((Request*)newRequest)->getClientFd(), recvHttpResponse.getRawResponseText().c_str(), raw_reponse_size);
    }
    close(((Request*)newRequest)->getClientFd());
    return ;
}

HttpResponse Proxy::sendMsgToWebserver(HttpRequest newHttpRequest, void* newRequest, Client& proxy_own_client){
    // send msg to webserver
    size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    proxy_own_client.sendRequest(newHttpRequest.getRawRequestText().c_str(), http_raw_text_size);
    proxyLog.writeLogFile(getRequstLogLine(newHttpRequest, newRequest, std::string("from_proxy_to_webserver")));
    // recv response from webserver
    std::string webserver_response = proxy_own_client.recvResponse();
    std::cout << webserver_response << std::endl;
    // check whether encoding is trunked data
    HttpResponse recvHttpResponse = HttpResponse(webserver_response);
    // normal case
    std::cout << "start sending normal GET data" << std::endl;
    size_t msgContentLength = recvHttpResponse.getContentLength();
    size_t msgBodySize = recvHttpResponse.getMsgBodySize();
    if (msgBodySize < msgContentLength){
        std::cout << "The data is not fully received" << std::endl;
        std::cout << "msgContentLength: " <<  msgContentLength << std::endl;
        std::cout << "msgBodySize: " << msgBodySize <<  std::endl;
        std::string all_response = recvAllData(proxy_own_client, webserver_response, msgContentLength, msgBodySize);
        //proxyLog.writeLogFile(getResponseLogLine(HttpResponse(all_response), newRequest, newHttpRequest.getHost()));
        return HttpResponse(all_response);
        // while(msgBodySize < msgContentLength){
        //     temp = proxy_own_client.recvResponse();
        //     if (temp.empty()) break;
        //     webserver_response += temp;
        //     msgBodySize += temp.size();
        // }
        // return HttpResponse(webserver_response);
    } else {
        return HttpResponse(webserver_response);
    }
}

void Proxy::sendMsgFromProxy(int sockfd, const char* msg, size_t size){
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

std::string Proxy::recvAllData(Client& client, std::string server_meg, size_t contentLength, size_t msgBodySize){
    std::cout << "The data is receiving continually" << std::endl;
    std::cout << "The reponse length is now:::" << server_meg.length() <<  std::endl;
    char * server_meg_char = const_cast<char *>(server_meg.c_str());
    size_t curr_len = server_meg.length();
    size_t msgCurSize = msgBodySize;
    size_t len = 0;
    std::string recv_msg_str(server_meg_char, curr_len);
    while(msgCurSize < contentLength) {
        char recv_msg[65536] = {0};
        if ((len = (recv(client.getSockfd(), recv_msg, sizeof(recv_msg), 0)) )  <= 0) break;
        std::string temp(recv_msg, len);
        recv_msg_str += temp;
        msgCurSize += len;
    }
    std::cout << "The data completed receiving" << std::endl;
    std::cout << "The data completed receiving size::: "  << recv_msg_str.size() << std::endl;
    return recv_msg_str;
}

std::string Proxy::getRequstLogLine(HttpRequest newHttpRequest, void* newRequest, std::string mode){
    std::string requestID = std::to_string(((Request*)newRequest)->getRequestID());
    std::string requestStartLine = newHttpRequest.getRequestLine();
    if (mode == "from_browser_to_proxy"){
        std::string requestIP = ((Request*)newRequest)->getClientIP();
        std::string requestTime = Time::getLocalUTC();
        return requestID + ": " + requestStartLine + " from " + requestIP + " @ " + requestTime;
    } 
    if (mode == "from_proxy_to_webserver"){
        std::string requestHost = newHttpRequest.getHost();
        return requestID + ": " + "Requesting " + requestStartLine + " from " + requestHost;   
    }
    return "";
}

std::string Proxy::getResponseLogLine(HttpResponse webserver_response, void* newRequest, std::string hostname){
    std::string requestID = std::to_string(((Request*)newRequest)->getRequestID());
    std::string responseStartLine = webserver_response.getStartLine();
    return requestID + ": " + "Received " + responseStartLine + " from " + hostname;   
}