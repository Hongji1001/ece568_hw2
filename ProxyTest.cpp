#include "ProxyTest.hpp"

void Proxy::startRun()
{
    // init as server and start listening
    std::cout << "start Run server " << std::endl;
    Server server = Server(port_num);
    // check whether proxy server can init successful
    if (server.getErrorSign() == -1)
    {
        // how to exit gracefully return nullptr
        std::cout << "can not init as a server";
        exit(EXIT_FAILURE);
    }
    std::cout << "inited server " << std::endl;
    // start getting request from client
    while (true)
    {
        int client_connection_fd = server.tryAccept();
        if (client_connection_fd == -1)
        {
            std::cout << "fail to accept" << std::endl;
            exit(EXIT_FAILURE);
        }
        if (server.getErrorSign() == -1)
        {
            std::cout << "can not init as a server";
            exit(EXIT_FAILURE);
        }
        std::cout << "has accept" << std::endl;
        // recv msg
        std::string msg = server.recvData(0);
        // string msgString(msg, msg + strlen(msg));
        std::cout << msg << std::endl;
        Request *newRequest = new Request(msg, client_connection_fd);
        pthread_t thread;
        pthread_create(&thread, NULL, handle, newRequest);
    }
}

void *Proxy::handle(void *newRequest)
{
    // 多线程如何才算结束
    std::string rawRequestText = ((Request *)newRequest)->getRequestLine();
    if (rawRequestText.empty() || rawRequestText == "\r" || rawRequestText == "\n" || rawRequestText == "\r\n")
        return nullptr;
    HttpRequest newHttpRequest = HttpRequest(rawRequestText);
    // start to verify http request
    std::cout << newHttpRequest.getHost() << std::endl;
    std::cout << newHttpRequest.getMethod() << std::endl;
    std::cout << newHttpRequest.getPort() << std::endl;
    std::cout << newHttpRequest.getRequestTarget() << std::endl;

    // handle CONNECT Request
    if (newHttpRequest.getMethod() == "CONNECT")
    {
        handleCONNECT(newHttpRequest, newRequest);
        return nullptr;
    }

    if (newHttpRequest.getMethod() == "POST")
    {
        handlePOST(newHttpRequest, newRequest);
        return nullptr;
    }
    // if httprequest has cached

    // did not cache
    std::string web_response = sendMsgToWebserver(newHttpRequest);
    sendMsgFromProxy(((Request *)newRequest)->getClientFd(), web_response.c_str(), web_response.size());
    return nullptr;
}

void Proxy::handleCONNECT(HttpRequest newHttpRequest, void *newRequest)
{
    // Receive HTTP CONNECT request from client

    // Parse the request line to get target server ip & port from it
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    // const char* webserver_hostname = newHttpRequest.getHost().c_str();
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl; // 确定port是443
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;

    // Make a new socket, connect it to the target server ip & port (make sure port is 443 for CONNECT)
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());
    int proxy_to_webserver_fd = proxy_own_client.getSockfd();
    int browser_to_proxy_fd = ((Request *)newRequest)->getClientFd();

    // Send exactly this response "HTTP/1.1 200 OK\r\n\r\n" back to the client
    const char *ACK_msg = "HTTP/1.1 200 OK\r\n\r\n";
    sendMsgFromProxy(browser_to_proxy_fd, ACK_msg, strlen(ACK_msg));
    // Then use non-blocking I/O
    // (e.g. "select") to receive and send bytes back and forth between the client/server.
    fd_set readfds;
    struct timeval tv;
    tv.tv_sec = 1;
    while (true)
    {
        std::vector<char> buff(MAX_TCP_PACKET_SIZE);
        int n = std::max(proxy_to_webserver_fd, browser_to_proxy_fd) + 1;
        FD_ZERO(&readfds);
        FD_SET(proxy_to_webserver_fd, &readfds);
        FD_SET(browser_to_proxy_fd, &readfds);
        int rv = select(n, &readfds, NULL, NULL, &tv);
        int numbytes;
        if (rv == -1)
        {
            perror("select"); // error occurred in select()
            break;
        }
        else if (rv == 0)
        {
            std::cout << "Timeout occurred! No data after 10.5 seconds." << std::endl;
            break;
        }
        else
        {
            if (FD_ISSET(proxy_to_webserver_fd, &readfds))
            {
                // recv from webserver
                numbytes = recv(proxy_to_webserver_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);
                if (numbytes < 0)
                {
                    std::cout << "recv error" << std::endl;
                    break;
                }
                if (numbytes == 0)
                {
                    std::cout << "recv finished" << std::endl;
                    break;
                }
                // std::string recv_data_from_webserver = recvMsgInProxy(proxy_to_webserver_fd){}
                // send data back to browser
                numbytes = send(browser_to_proxy_fd, buff.data(), numbytes, 0);
                if (numbytes < 0)
                {
                    std::cout << "send error" << std::endl;
                    break;
                }
            }
            if (FD_ISSET(browser_to_proxy_fd, &readfds))
            {
                // recv request from browser
                numbytes = recv(browser_to_proxy_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);
                if (numbytes < 0)
                {
                    std::cout << "recv error" << std::endl;
                    break;
                }
                if (numbytes == 0)
                {
                    std::cout << "recv finished" << std::endl;
                    break;
                }
                // send data to webserver
                numbytes = send(proxy_to_webserver_fd, buff.data(), numbytes, 0);
                if (numbytes < 0)
                {
                    std::cout << "send error" << std::endl;
                    break;
                }
            }
            buff.clear();
        }
    }
    // Keep this connection open until one side closes it (i.e. a 'recv' call returns with 0 bytes).
    // close(proxy_to_webserver_fd);
    // close(browser_to_proxy_fd);
}

void Proxy::handlePOST(HttpRequest newHttpRequest, void *newRequest)
{

    //     浏览器发起请求 TCP 连接
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    // const char* webserver_hostname = newHttpRequest.getHost().c_str();
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());
    // send msg to webserver
    size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    proxy_own_client.sendRequest(newHttpRequest.getRawRequestText().c_str(), http_raw_text_size);

    // recv response from webserver
    std::string webserver_response = proxy_own_client.recvResponse();
    std::cout << webserver_response << std::endl;
    HttpResponse recvHttpResponse = HttpResponse(webserver_response);
    std::cout << recvHttpResponse.getStatusCode() << std::endl;
    std::cout << recvHttpResponse.getReasonPhrase() << std::endl;
    std::cout << recvHttpResponse.getMsgBody() << std::endl;
    // 服务器返回100 Continue响应
    if (recvHttpResponse.getStatusCode() != "100" || recvHttpResponse.getReasonPhrase() != "Continue")
    {
        return;
    }
    else
    {
        std::vector<char> buff(MAX_TCP_PACKET_SIZE);
        int proxy_to_webserver_fd = proxy_own_client.getSockfd();
        int browser_to_proxy_fd = ((Request *)newRequest)->getClientFd();
        int numbytes;
        // 浏览器发送数据
        numbytes = recv(browser_to_proxy_fd, &buff.data()[0], MAX_TCP_PACKET_SIZE, 0);
        if (numbytes < 0)
        {
            std::cout << "recv error" << std::endl;
            return;
        }
        numbytes = send(proxy_to_webserver_fd, buff.data(), numbytes, 0);
        if (numbytes < 0)
        {
            std::cout << "send error" << std::endl;
            return;
        }
        // 服务器返回 200 OK响应
        webserver_response = proxy_own_client.recvResponse();
        std::cout << "new new new webserver_response" << std::endl;
        std::cout << webserver_response << std::endl;
        send(browser_to_proxy_fd, webserver_response.c_str(), webserver_response.size(), 0);
        return;
    }
}

std::string Proxy::sendMsgToWebserver(HttpRequest newHttpRequest)
{
    // send request to webserver
    // get port and hostname of webserver
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    // const char* webserver_hostname = newHttpRequest.getHost().c_str();
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());
    // send msg to webserver
    size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    proxy_own_client.sendRequest(newHttpRequest.getRawRequestText().c_str(), http_raw_text_size);

    // recv response from webserver
    std::string webserver_response = proxy_own_client.recvResponse();
    std::cout << webserver_response << std::endl;

    std::cout << std::endl;
    std::cout << std::endl;

    HttpResponse recvHttpResponse = HttpResponse(webserver_response);
    std::cout << recvHttpResponse.getStatusCode() << std::endl;
    std::cout << recvHttpResponse.getReasonPhrase() << std::endl;
    std::cout << recvHttpResponse.getMsgBody() << std::endl;
    return recvHttpResponse.getRawResponseText();
}

void Proxy::sendMsgFromProxy(int sockfd, const char *msg, size_t size)
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

void Proxy::validation(HttpRequest &request)
{
    // 验证是否禁用缓存
    if (request.getHeaderMap().count("cache-control") == 0 || (request.getHeaderMap().count("cache-control") != 0 && request.getHeaderMap()["cache-control"].find("no-store")))
    {
        // 不禁用缓存
        std::string cacheKey = request.getRequestTarget();
        // 验证是否有代理缓存
        if (cache.isCached(cacheKey))
        {
            // 有代理缓存
            // 验证有cache-control且是否有no-cache字段或者must-validation字段或者没有cache-control字段
            if (request.getHeaderMap().count("cache-control") == 0 || (request.getHeaderMap()["cache-control"].find("no-cache") == std::string::npos && request.getHeaderMap()["cache-control"].find("must-revalidate") == std::string::npos))
            {
                // 没有cache-control或者没有相应字段
                // 验证新鲜度
                if (cache.isFresh(cacheKey, request.getRequestTime()))
                {
                    // 新鲜
                    // 验证是否是条件请求
                    if (request.getHeaderMap().count("If-None-Match") != 0 || request.getHeaderMap().count("If-Modified-Since") != 0)
                    {
                        // 是条件请求
                        // 比较缓存中的Etag/LastModified字段是否相同
                        if (request.getHeaderMap().count("If-None-Match") != 0 && request.getHeaderMap()["If-None-Match"] == cache.getCacheMap()[cacheKey]->Etag)
                        {
                            // 返回304
                            // TODO:修改响应头的状态码为304
                            cache.getCacheMap()[cacheKey]->rawResponseStartLine = "GET 304 Not Modified HTTP/1.1";
                            sendMsgFromProxy(, cache.get(cacheKey).c_str(), );
                            return;
                        }
                        if (request.getHeaderMap().count("If-Modified-Since") != 0 && request.getHeaderMap()["If-Modified-Since"] == cache.getCacheMap()[cacheKey]->LastModified)
                        {
                            // 返回304
                            // TODO:修改响应头的状态码为304
                            cache.getCacheMap()[cacheKey]->rawResponseStartLine = "GET 304 Not Modified HTTP/1.1";
                            sendMsgFromProxy(, cache.get(cacheKey).c_str(), );
                            return;
                        }
                        // 不相同的话就要返回缓存的全部内容，以200OK的状态缓存
                        sendMsgFromProxy(, cache.get(cacheKey).c_str(), );
                        return;
                    }
                    else
                    {
                        // 不是条件请求
                        // 直接返回缓存的响应,缓存中的status code应该都是200OK,只有300行需要修改为304
                        sendMsgFromProxy(, cache.get(cacheKey).c_str(), );
                        return;
                    }
                }
                else
                {
                    // 不新鲜
                    // 验证是否是条件请求
                    if (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0)
                    {
                        // 不是条件请求
                        // 添加条件请求信息
                        request.buildConRequest(cache.getCacheMap()[cacheKey]->Etag, cache.getCacheMap()[cacheKey]->LastModified);
                        // 发回web验证
                        HttpResponse webResponse = sendMsgToWebserver(request);
                        // TODO:处理响应
                    }
                    else
                    {
                        // 是条件请求
                        // 直接转发请求进行验证
                        HttpResponse webResponse = sendMsgToWebserver(request);
                        // TODO:处理响应
                    }
                }
            }
            else
            {
                // 有cache-control且有相应字段
                // 直接转发请求进行验证
                HttpResponse webResponse = sendMsgToWebserver(request);
                // TODO:处理响应
            }
        }
        else
        {
            // 没有代理缓存，直接转发给web
            HttpResponse webResponse = sendMsgToWebserver(request);
            // TODO:处理响应
        }
    }
    else
    {
        // 禁用缓存
        HttpResponse webResponse = sendMsgToWebserver(request);
        // TODO:处理响应
    }
}