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
    else if (newHttpRequest.getMethod() == "POST")
    {
        std::cout << "Start to handle POST request" << std::endl;
        // handlePOST(newHttpRequest, newRequest);
        handleGET(newHttpRequest, newRequest);
        return nullptr;
    }
    else
    {
        // GET Request
        // if httprequest has cached
        std::cout << "Start to handle GET request" << std::endl;
        std::cout << "进入handleGet" << std::endl;
        // did not cache
        handleGET(newHttpRequest, newRequest);
        return nullptr;
    }
    // if httprequest has cached
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
    // send request to webserver and get HttpResponse
    HttpResponse recvHttpResponse = sendMsgToWebserver(newHttpRequest, newRequest);
    // send raw text of httpResponse to browser
    size_t raw_reponse_size = recvHttpResponse.getRawResponseText().size();
    sendMsgFromProxy(((Request *)newRequest)->getClientFd(), recvHttpResponse.getRawResponseText().c_str(), raw_reponse_size);
    return;
}

void Proxy::handleGET(HttpRequest request, void *newRequest)
{
    int sockfd = ((Request *)newRequest)->getClientFd();
    std::cout << "进入handleGet" << std::endl;
    std::cout << request.getHeaderMap().count("cache-control") << std::endl;
    std::cout << request.getHeaderMap()["cache-control"].find("private") << std::endl;
    std::cout << request.getHeaderMap()["cache-control"].find("no-store") << std::endl;
    // 验证是否禁用代理缓存 未禁用的情况：1.没有cache-control字段 2.有cache-control字段但没有private,no-store字段中的任意一个
    if (request.getHeaderMap().count("cache-control") == 0 ||
        (request.getHeaderMap().count("cache-control") != 0 &&
         (request.getHeaderMap()["cache-control"].find("private") == std::string::npos && request.getHeaderMap()["cache-control"].find("no-store") == std::string::npos)))
    {
        // 不禁用缓存
        std::cout << "不禁用缓存 ";
        std::string cacheKey = request.getRequestTarget();
        // 验证是否有代理缓存
        if (cache.isCached(cacheKey))
        {
            std::cout << "有代理缓存 ";
            // 不禁用缓存，有代理缓存
            // 验证有cache-control且是否有no-cache字段或者must-validation字段或者没有cache-control字段
            if (request.getHeaderMap().count("cache-control") == 0 || (request.getHeaderMap()["cache-control"].find("no-cache") == std::string::npos && request.getHeaderMap()["cache-control"].find("must-revalidate") == std::string::npos))
            {
                std::cout << "没有cache-control或者没有相应字段 ";
                // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段
                // 验证新鲜度
                if (cache.isFresh(cacheKey, request.getRequestTime()))
                {
                    std::cout << "新鲜 ";
                    // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，新鲜
                    // 验证是否是条件请求
                    if (request.getHeaderMap().count("If-None-Match") != 0 || request.getHeaderMap().count("If-Modified-Since") != 0)
                    {
                        std::cout << "条件请求 ";
                        // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，新鲜，是条件请求
                        // 比较缓存中的Etag/LastModified字段是否相同
                        if ((request.getHeaderMap().count("If-None-Match") != 0 && request.getHeaderMap()["If-None-Match"] == cache.getCacheMap()[cacheKey]->Etag) ||
                            (request.getHeaderMap().count("If-Modified-Since") != 0 && request.getHeaderMap()["If-Modified-Since"] == cache.getCacheMap()[cacheKey]->LastModified))
                        {
                            // 返回304状态码,给到最新的响应头
                            std::string newResponse = "HTTP/1.1 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
                            sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
                            return;
                        }
                        // 不相同的话就要返回缓存的全部内容，以200OK的状态缓存
                        sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                    }
                    else
                    {
                        std::cout << "不是条件请求 ";
                        // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，新鲜，不是条件请求
                        sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                    }
                }
                else
                {
                    std::cout << "不新鲜 ";
                    // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，不新鲜
                    // 验证是否是条件请求
                    if (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0)
                    {
                        std::cout << "不是条件请求 ";
                        // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，不新鲜，不是条件请求
                        // 添加条件请求信息
                        request.buildConRequest(cache.getCacheMap()[cacheKey]->Etag, cache.getCacheMap()[cacheKey]->LastModified);
                        // 发回web验证
                        HttpResponse webResponse = sendMsgToWebserver(request, newRequest);
                        cache.put(webResponse, cacheKey);
                        // 取出缓存并发送
                        sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                    }
                    else
                    {
                        std::cout << "条件请求 ";
                        // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，不新鲜，是条件请求
                        // 直接转发请求进行验证
                        HttpResponse webResponse = sendMsgToWebserver(request, newRequest);
                        cache.put(webResponse, cacheKey);
                        if (webResponse.getStatusCode() == "304")
                        {
                            std::string newResponse = webResponse.getHttpVersion() + " 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
                            sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
                            return;
                        }
                        // 取出缓存并发送
                        sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                    }
                }
            }
            else
            {
                std::cout << "有cache-control且有相应字段 ";
                // 有代理缓存，有cache-control且有相应字段，直接进行过期验证
                // 验证是否是条件请求
                if (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0)
                {
                    std::cout << "不是条件请求 ";
                    // 有代理缓存，有cache-control且有相应字段，不是条件请求
                    // 添加条件请求信息
                    request.buildConRequest(cache.getCacheMap()[cacheKey]->Etag, cache.getCacheMap()[cacheKey]->LastModified);
                    // 发回web验证
                    HttpResponse webResponse = sendMsgToWebserver(request, newRequest);
                    cache.put(webResponse, cacheKey);
                    // 取出缓存并发送
                    sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                }
                else
                {
                    std::cout << "条件请求 ";
                    // 有代理缓存，有cache-control且有相应字段，是条件请求
                    // 直接转发
                    HttpResponse webResponse = sendMsgToWebserver(request, newRequest);
                    cache.put(webResponse, cacheKey);
                    // 检验是否是304
                    if (webResponse.getStatusCode() == "304")
                    {
                        std::string newResponse = webResponse.getHttpVersion() + " 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
                        sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
                        return;
                    }
                    // 取出缓存并发送
                    sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                }
            }
        }
        else
        {
            std::cout << "没有代理缓存 ";
            // 不禁用缓存，没有代理缓存，不是条件请求
            HttpResponse webResponse = sendMsgToWebserver(request, newRequest);
            cache.put(webResponse, cacheKey);
            if (webResponse.getStatusCode() == "304" &&
                (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0))
            {
                std::cout << "条件请求 ";
                // 不禁用缓存，没有代理缓存，是条件请求
                std::string newResponse = webResponse.getHttpVersion() + " 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
                sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
                return;
            }
            std::cout << "不是条件请求 ";
            sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
        }
    }
    else
    {
        std::cout << "禁用缓存 ";
        // 禁用缓存
        HttpResponse webResponse = sendMsgToWebserver(request, newRequest);
        // 直接返回响应
        sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
    }
}

HttpResponse Proxy::sendMsgToWebserver(HttpRequest newHttpRequest, void *newRequest)
{
    // send request to webserver first and get response back
    // get port and hostname of webserver
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
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
    // check whether encoding is trunked data
    HttpResponse recvHttpResponse = HttpResponse(webserver_response);
    if (recvHttpResponse.checkIsChunked())
    {
        std::cout << "start sending chunked data" << std::endl;
        size_t raw_reponse_size = recvHttpResponse.getRawResponseText().size();
        std::cout << recvHttpResponse.getRawResponseText().c_str() << std::endl;
        sendMsgFromProxy(((Request *)newRequest)->getClientFd(), webserver_response.c_str(), webserver_response.size());
        // continue to recv and send
        while (true)
        {
            std::string temp = proxy_own_client.recvResponse();
            std::cout << temp << std::endl;
            if (temp.empty())
                break;
            sendMsgFromProxy(((Request *)newRequest)->getClientFd(), temp.c_str(), temp.size());
        }
    }
    else
    {
        // normal case
        std::cout << "start sending normal GET data" << std::endl;
        size_t msgContentLength = recvHttpResponse.getContentLength();
        size_t msgBodySize = recvHttpResponse.getMsgBodySize();
        if (msgBodySize < msgContentLength)
        {
            std::cout << "The data is not fully received" << std::endl;
            std::cout << "msgContentLength: " << msgContentLength << std::endl;
            std::cout << "msgBodySize: " << msgBodySize << std::endl;
            while (msgBodySize < msgContentLength)
            {
                std::string temp = proxy_own_client.recvResponse();
                if (temp.empty())
                    break;
                webserver_response += temp;
                msgBodySize += temp.size();
            }
            return HttpResponse(webserver_response);
        }
        else
        {
            return HttpResponse(webserver_response);
        }
    }
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

// void Proxy::validation(HttpRequest &request, int sockfd)
// {
//     // 验证是否禁用代理缓存 未禁用的情况：1.没有cache-control字段 2.有cache-control字段但没有private,no-store字段中的任意一个
//     if (request.getHeaderMap().count("cache-control") == 0 ||
//         (request.getHeaderMap().count("cache-control") != 0 &&
//          (request.getHeaderMap()["cache-control"].find("private") == std::string::npos && request.getHeaderMap()["cache-control"].find("no-store") == std::string::npos)))
//     {
//         // 不禁用缓存
//         std::string cacheKey = request.getRequestTarget();
//         // 验证是否有代理缓存
//         if (cache.isCached(cacheKey))
//         {
//             // 不禁用缓存，有代理缓存
//             // 验证有cache-control且是否有no-cache字段或者must-validation字段或者没有cache-control字段
//             if (request.getHeaderMap().count("cache-control") == 0 || (request.getHeaderMap()["cache-control"].find("no-cache") == std::string::npos && request.getHeaderMap()["cache-control"].find("must-revalidate") == std::string::npos))
//             {
//                 // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段
//                 // 验证新鲜度
//                 if (cache.isFresh(cacheKey, request.getRequestTime()))
//                 {
//                     // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，新鲜
//                     // 验证是否是条件请求
//                     if (request.getHeaderMap().count("If-None-Match") != 0 || request.getHeaderMap().count("If-Modified-Since") != 0)
//                     {
//                         // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，新鲜，是条件请求
//                         // 比较缓存中的Etag/LastModified字段是否相同
//                         if ((request.getHeaderMap().count("If-None-Match") != 0 && request.getHeaderMap()["If-None-Match"] == cache.getCacheMap()[cacheKey]->Etag) ||
//                             (request.getHeaderMap().count("If-Modified-Since") != 0 && request.getHeaderMap()["If-Modified-Since"] == cache.getCacheMap()[cacheKey]->LastModified))
//                         {
//                             // 返回304状态码,给到最新的响应头
//                             std::string newResponse = "HTTP/1.1 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
//                             sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
//                             return;
//                         }
//                         // 不相同的话就要返回缓存的全部内容，以200OK的状态缓存
//                         sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//                     }
//                     else
//                     {
//                         // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，新鲜，不是条件请求
//                         sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//                     }
//                 }
//                 else
//                 {
//                     // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，不新鲜
//                     // 验证是否是条件请求
//                     if (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0)
//                     {
//                         // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，不新鲜，不是条件请求
//                         // 添加条件请求信息
//                         request.buildConRequest(cache.getCacheMap()[cacheKey]->Etag, cache.getCacheMap()[cacheKey]->LastModified);
//                         // 发回web验证
//                         HttpResponse webResponse = sendMsgToWebserver(request);
//                         cache.put(webResponse, cacheKey);
//                         // 取出缓存并发送
//                         sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//                     }
//                     else
//                     {
//                         // 不禁用缓存，有代理缓存，没有cache-control或者没有相应字段，不新鲜，是条件请求
//                         // 直接转发请求进行验证
//                         HttpResponse webResponse = sendMsgToWebserver(request);
//                         cache.put(webResponse, cacheKey);
//                         if (webResponse.getStatusCode() == "304")
//                         {
//                             std::string newResponse = webResponse.getHttpVersion() + " 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
//                             sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
//                             return;
//                         }
//                         // 取出缓存并发送
//                         sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//                     }
//                 }
//             }
//             else
//             {
//                 // 有代理缓存，有cache-control且有相应字段，直接进行过期验证
//                 // 验证是否是条件请求
//                 if (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0)
//                 {
//                     // 有代理缓存，有cache-control且有相应字段，不是条件请求
//                     // 添加条件请求信息
//                     request.buildConRequest(cache.getCacheMap()[cacheKey]->Etag, cache.getCacheMap()[cacheKey]->LastModified);
//                     // 发回web验证
//                     HttpResponse webResponse = sendMsgToWebserver(request);
//                     cache.put(webResponse, cacheKey);
//                     // 取出缓存并发送
//                     sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//                 }
//                 else
//                 {
//                     // 有代理缓存，有cache-control且有相应字段，是条件请求
//                     // 直接转发
//                     HttpResponse webResponse = sendMsgToWebserver(request);
//                     cache.put(webResponse, cacheKey);
//                     // 检验是否是304
//                     if (webResponse.getStatusCode() == "304")
//                     {
//                         std::string newResponse = webResponse.getHttpVersion() + " 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
//                         sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
//                         return;
//                     }
//                     // 取出缓存并发送
//                     sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//                 }
//             }
//         }
//         else
//         {
//             // 不禁用缓存，没有代理缓存，不是条件请求
//             HttpResponse webResponse = sendMsgToWebserver(request);
//             cache.put(webResponse, cacheKey);
//             if (webResponse.getStatusCode() == "304" &&
//                 (request.getHeaderMap().count("If-None-Match") == 0 && request.getHeaderMap().count("If-Modified-Since") == 0))
//             {
//                 // 不禁用缓存，没有代理缓存，是条件请求
//                 std::string newResponse = webResponse.getHttpVersion() + " 304 Not Modified\r\n" + cache.getCacheMap()[cacheKey]->rawResponseHead + "\r\n";
//                 sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
//                 return;
//             }
//             sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
//         }
//     }
//     else
//     {
//         // 禁用缓存
//         HttpResponse webResponse = sendMsgToWebserver(request);
//         // 直接返回响应
//         sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
//     }
// }