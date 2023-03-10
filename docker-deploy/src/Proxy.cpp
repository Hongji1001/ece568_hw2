#include "Proxy.hpp"
pthread_mutex_t cachelock = PTHREAD_MUTEX_INITIALIZER;
void Proxy::startRun()
{
    // init as server and start listening
    std::cout << "start Run server " << std::endl;
    Server server = Server(port_num);
    proxyLog.openLogFile("/var/log/erss/proxy.log"); // TODO: Try Catch
    // check whether proxy server can init successful
    if (server.getErrorSign() == -1)
    {
        // how to exit gracefully return nullptr
        std::cerr << "can not init as a server" << std::endl;
        throw std::exception();
    }
    std::cout << "inited server " << std::endl;
    // start getting request from client
    while (true)
    {
        int client_connection_fd = server.tryAccept();
        if (client_connection_fd == -1 || server.getErrorSign() == -1)
        {
            std::cout << "fail to accept this request" << std::endl;
            continue;
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
    proxyLog.writeRequstLogLine(newHttpRequest, newRequest, std::string("from_browser_to_proxy"));
    if (newHttpRequest.getHasError())
    {
        // if the HttpRequest is malformed request
        int error_status_code = 400;
        HttpResponse formed_response = getFormedHttpResponse(error_status_code);
        sendFormedHttpResponse(formed_response, newHttpRequest, newRequest);
        close(((Request *)newRequest)->getClientFd());
        return nullptr;
    }

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
        handlePOST(newHttpRequest, newRequest);
        return nullptr;
    }
    else
    {
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

void Proxy::handleCONNECT(HttpRequest &newHttpRequest, void *newRequest)
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
    if (proxy_own_client.getHasError())
    {
        // 如果client连接错误马上返回一个400 bad
        int error_status_code = 400;
        ;
        HttpResponse formed_response = getFormedHttpResponse(error_status_code);
        sendFormedHttpResponse(formed_response, newHttpRequest, newRequest);
        return;
    }
    int proxy_to_webserver_fd = proxy_own_client.getSockfd();
    int browser_to_proxy_fd = ((Request *)newRequest)->getClientFd();

    // Send exactly this response "HTTP/1.1 200 OK\r\n\r\n" back to the client
    const char *ACK_msg = "HTTP/1.1 200 OK\r\n\r\n";
    // proxyLog.writeLogFile("Responding" + to_string("HTTP/1.1 200 OK")));
    sendMsgFromProxy(browser_to_proxy_fd, ACK_msg, strlen(ACK_msg));
    proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
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
    close(((Request *)newRequest)->getClientFd());
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    proxyLog.writeLogFile(requestID + ": " + std::string("Tunnel closed"));
}

void Proxy::handlePOST(HttpRequest &newHttpRequest, void *newRequest)
{
    // send request to webserver and get HttpResponse
    HttpResponse recvHttpResponse = sendMsgToWebserver(newHttpRequest, newRequest);
    if (recvHttpResponse.getStatusCode() == "400")
    {
        sendFormedHttpResponse(recvHttpResponse, newHttpRequest, newRequest);
        return;
    }
    // proxyLog.writeResponseLogLine(recvHttpResponse.getStartLine(), newRequest, newHttpRequest.getHost(), "from_webserver_to_browser"); TO DELETE
    // send raw text of httpResponse to browser
    size_t raw_reponse_size = recvHttpResponse.getRawResponseText().size();
    sendMsgFromProxy(((Request *)newRequest)->getClientFd(), recvHttpResponse.getRawResponseText().c_str(), raw_reponse_size);
    close(((Request *)newRequest)->getClientFd());
    return;
}

void Proxy::handleGET(HttpRequest &newHttpRequest, void *newRequest)
{
    try
    {        
        if (newHttpRequest.getHeaderMap().count("if-none-match") != 0 || newHttpRequest.getHeaderMap().count("if-modified-since") != 0)
        {
            std::cout << "开始处理条件请求 " << std::endl;
            conditionalReq(newHttpRequest, newRequest);
        }
        else
        {
            std::cout << "开始处理非条件请求 " << std::endl;
            nonConditionalReq(newHttpRequest, newRequest);
        }
    }
    catch(const std::exception& e)
    {
        std::cerr << "GET Request failed" << std::endl;
        std::cerr << e.what() << '\n';
    }
    close(((Request *)newRequest)->getClientFd());
}

HttpResponse Proxy::sendMsgToWebserver(HttpRequest &newHttpRequest, void *newRequest)
{
    // send request to webserver first and get response back
    // get port and hostname of webserver
    unsigned short webserver_port_num = std::stoul(newHttpRequest.getPort());
    std::cout << "webserver_port_num" << std::endl;
    std::cout << webserver_port_num << std::endl;
    std::cout << "newHttpRequest.getHost()" << std::endl;
    std::cout << newHttpRequest.getHost() << std::endl;
    proxyLog.writeRequstLogLine(newHttpRequest, newRequest, std::string("from_proxy_to_webserver"));
    Client proxy_own_client = Client(webserver_port_num, newHttpRequest.getHost());
    if (proxy_own_client.getHasError())
    {
        // 如果client连接错误马上返回一个400 bad
        int status_code = 400;
        return getFormedHttpResponse(status_code);
    }
    // send msg to webserver
    size_t http_raw_text_size = newHttpRequest.getRawRequestText().size();
    try
    {
        proxy_own_client.sendRequest(newHttpRequest.getRawRequestText().c_str(), http_raw_text_size);
    }
    catch(const std::exception& e)
    {
        int status_code = 400;
        std::cout << "send request to webserver failed" << std::endl;
        return getFormedHttpResponse(status_code);
    }
    // recv response from webserver
    std::string webserver_response = proxy_own_client.recvResponse();
    // std::cout << webserver_response << std::endl;
    // check whether encoding is trunked data
    HttpResponse recvHttpResponse = HttpResponse(webserver_response);
    std::cout << recvHttpResponse.getHead().c_str() << std::endl;
    if (recvHttpResponse.checkIsChunked())
    {
        std::cout << "start sending chunked data" << std::endl;
        sendMsgFromProxy(((Request *)newRequest)->getClientFd(), webserver_response.c_str(), webserver_response.size());
        proxyLog.writeResponseLogLine(recvHttpResponse.getStartLine(), newRequest, newHttpRequest.getHost(), "from_webserver_to_browser");
        // continue to recv and send
        while (true)
        {
            std::string temp = proxy_own_client.recvResponse();
            // std::cout << temp << std::endl;
            sendMsgFromProxy(((Request *)newRequest)->getClientFd(), temp.c_str(), temp.size());
            if (temp.find("0\r\n\r\n") != std::string::npos)
                break;
        }
        return recvHttpResponse;
    }
    else
    {
        // normal case
        std::cout << "start sending normal GET data" << std::endl;
        size_t msgContentLength = recvHttpResponse.getContentLength();
        size_t msgBodySize = recvHttpResponse.getMsgBodySize();
        proxyLog.writeResponseLogLine(recvHttpResponse.getStartLine(), newRequest, newHttpRequest.getHost(), "from_webserver_to_browser");
        if (msgBodySize < msgContentLength)
        {
            std::cout << "The data is not fully received" << std::endl;
            std::cout << "msgContentLength: " << msgContentLength << std::endl;
            std::cout << "msgBodySize: " << msgBodySize << std::endl;
            // proxyLog.writeLogFile(getResponseLogLine(HttpResponse(all_response), newRequest, newHttpRequest.getHost()));
            return recvAllData(proxy_own_client, webserver_response, msgContentLength, msgBodySize);
            // while(msgBodySize < msgContentLength){
            //     temp = proxy_own_client.recvResponse();
            //     if (temp.empty()) break;
            //     webserver_response += temp;
            //     msgBodySize += temp.size();
            // }
            // return HttpResponse(webserver_response);
        }
        else
        {
            return HttpResponse(webserver_response);
        }
    }
}

void Proxy::sendMsgFromProxy(int sockfd, const char *msg, size_t size)
{
    size_t numBytes = 0;
    int recvBytes = 0;
    while ((numBytes < size))
    {
        if ((recvBytes = send(sockfd, msg, size, MSG_NOSIGNAL)) == -1)
        {
            perror("client send");
            throw std::exception();
        }
        numBytes += recvBytes;
    }
}

HttpResponse Proxy::recvAllData(Client &client, std::string server_meg, size_t contentLength, size_t msgBodySize)
{
    std::cout << "The data is receiving continually" << std::endl;
    std::cout << "The reponse length is now:::" << server_meg.length() << std::endl;
    char *server_meg_char = const_cast<char *>(server_meg.c_str());
    size_t curr_len = server_meg.length();
    size_t msgCurSize = msgBodySize;
    size_t len = 0;
    std::string recv_msg_str(server_meg_char, curr_len);
    while (msgCurSize < contentLength)
    {
        char recv_msg[65536] = {0};
        if ((len = (recv(client.getSockfd(), recv_msg, sizeof(recv_msg), 0))) <= 0)
            break;
        std::string temp(recv_msg, len);
        recv_msg_str += temp;
        msgCurSize += len;
    }
    if (msgCurSize < contentLength)
    {
        // corrupted response: doesn't receive the correct length response
        int status_code = 502;
        return getFormedHttpResponse(status_code);
    }
    std::cout << "The data completed receiving" << std::endl;
    std::cout << "The data completed receiving size::: " << recv_msg_str.size() << std::endl;
    return HttpResponse(recv_msg_str);
}

void Proxy::checkCachingResponse(HttpResponse &webResponse, HttpRequest &newHttpRequest, void *newRequest)
{
    int sockfd = ((Request *)newRequest)->getClientFd();
    std::string cacheKey = newHttpRequest.getRequestTarget();
    if (webResponse.getStatusCode() == "400")
    {
        sendFormedHttpResponse(webResponse, newHttpRequest, newRequest);
        return;
    }
    if (webResponse.checkIsChunked())
    {
        return;
    }
    if (webResponse.getStatusCode() != "304" && webResponse.getStatusCode() != "200")
    {
        std::cout << "非304或200直接发送响应返回浏览器 " << std::endl;
        sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
        return;
    }
    if (!cache.isResForbiden(webResponse))
    {
        std::cout << "响应不禁止缓存，存入缓存" << std::endl;
        cache.put(webResponse.getRawResponseText(), cacheKey);
        std::cout << "将缓存中的响应返回浏览器 " << std::endl;
        std::string cachedResponse = cache.get(cacheKey);
        sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
        proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
    }
    else
    {
        std::cout << "响应禁止缓存，直接发送响应返回浏览器 " << std::endl;
        sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
    }
}

void Proxy::conditionalReq(HttpRequest &newHttpRequest, void *newRequest)
{
    int sockfd = ((Request *)newRequest)->getClientFd();
    std::string cacheKey = newHttpRequest.getRequestTarget();
    std::cout << "进入条件请求验证" << std::endl;
    if (cache.isCached(cacheKey))
    {
        std::cout << "代理中有缓存 ";
        if (cache.isFresh(cacheKey, newHttpRequest.getRequestTime()))
        {
            std::string ETag = cache.getCacheNodeETag(cacheKey);
            std::string LastModified = cache.getCacheNodeLastModified(cacheKey);
            std::cout << "缓存新鲜 ";
            proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "valid", "");
            if ((newHttpRequest.getHeaderMap().count("if-none-match") != 0 && newHttpRequest.getHeaderMap()["if-none-match"] == ETag) ||
                (newHttpRequest.getHeaderMap().count("if-modified-since") != 0 && newHttpRequest.getHeaderMap()["if-modified-since"] == LastModified))
            {
                std::cout << "缓存匹配 ";
                std::string newResponse = "HTTP/1.1 304 Not Modified\r\n" + cache.getCacheRawResHead(cacheKey) + "\r\n";
                std::cout << "将304响应返回浏览器 " << std::endl;
                sendMsgFromProxy(sockfd, newResponse.c_str(), newResponse.size());
                proxyLog.writeResponseLogLine(std::string("HTTP/1.1 304 Not Modified"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
            }
            else
            {
                std::cout << "缓存不匹配 ";
                std::cout << "将缓存中的响应返回浏览器 " << std::endl;
                sendMsgFromProxy(sockfd, cache.get(cacheKey).c_str(), cache.get(cacheKey).size());
                proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
            }
        }
        else
        {
            std::cout << "缓存不新鲜 ";
            // TODO：计算过期时间，缓存不新鲜一定是incache的，干脆把incache的响应传进去得了
            std::string resp = cache.getCacheNodeFullResponse(cacheKey);
            proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "expired", resp);
            // TODO：这里有点小问题？如果代理的不新鲜了，浏览器也一定不新鲜吧
            HttpResponse webResponse = sendMsgToWebserver(newHttpRequest, newRequest);
            if (webResponse.getStatusCode() == "400")
            {
                sendFormedHttpResponse(webResponse, newHttpRequest, newRequest);
                return;
            }
            if (webResponse.checkIsChunked())
            {
                proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                return;
            }
            if (webResponse.getStatusCode() != "304" && webResponse.getStatusCode() != "200")
            {
                std::cout << "非304或200直接发送响应返回浏览器 " << std::endl;
                sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
                return;
            }
            if (!cache.isResForbiden(webResponse))
            {
                std::cout << "响应不禁止缓存，存入缓存" << std::endl;
                if (cache.isResMustRevalid(webResponse))
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "re-validation");
                }
                else
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "cached expired");
                }
                cache.put(webResponse.getRawResponseText(), cacheKey);
                std::cout << "将更新后缓存中的响应返回浏览器 " << std::endl;
                std::string cachedResponse = cache.get(cacheKey);
                sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
                proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
            }
            else
            {
                std::cout << "响应禁止缓存，直接发送响应" << std::endl;
                proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
            }
        }
    }
    else
    {
        std::cout << "代理中没有缓存 ";
        proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "not in cache", "");
        HttpResponse webResponse = sendMsgToWebserver(newHttpRequest, newRequest);
        if (webResponse.getStatusCode() == "400")
        {
            sendFormedHttpResponse(webResponse, newHttpRequest, newRequest);
            return;
        }
        if (webResponse.checkIsChunked())
        {
            proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
            return;
        }
        if (webResponse.getStatusCode() != "304" && webResponse.getStatusCode() != "200")
        {
            std::cout << "非304或200直接发送响应返回浏览器 " << std::endl;
            sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
            return;
        }
        if (!cache.isResForbiden(webResponse))
        {
            // 只有不禁止且200 OK的时候能够存入代理缓存
            if (webResponse.getStatusCode() == "200")
            {
                std::cout << "响应不禁止缓存，存入缓存" << std::endl;
                if (cache.isResMustRevalid(webResponse))
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "re-validation");
                }
                else
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "cached expired");
                }
                cache.put(webResponse.getRawResponseText(), cacheKey);
                std::cout << "将缓存中的响应返回浏览器 " << std::endl;
                std::string cachedResponse = cache.get(cacheKey);
                sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
                proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
                return;
            }
            // 其他时候直接转发
            sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
        }
        else
        {
            std::cout << "响应禁止缓存，直接发送响应" << std::endl;
            proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
            sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
        }
    }
}

void Proxy::nonConditionalReq(HttpRequest &newHttpRequest, void *newRequest)
{
    std::cout << "进入非条件请求验证" << std::endl;
    int sockfd = ((Request *)newRequest)->getClientFd();
    std::string requestID = std::to_string(((Request *)newRequest)->getRequestID());
    // 检验是否禁用缓存
    if (!cache.isReqForbiden(newHttpRequest))
    {
        std::cout << "请求不禁用缓存 ";
        std::string cacheKey = newHttpRequest.getRequestTarget();
        // 验证代理中是否有缓存
        if (cache.isCached(cacheKey))
        {
            std::cout << "代理中有缓存 ";
            std::string ETag = cache.getCacheNodeETag(cacheKey);
            std::string LastModified = cache.getCacheNodeLastModified(cacheKey);
            // 验证缓存是否要强制验证
            if (cache.isReqMustRevalid(newHttpRequest) || cache.isResMustRevalid(cacheKey))
            {
                std::cout << "需要强制验证 ";
                proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "requires validation", "");
                newHttpRequest.buildConRequest(ETag, LastModified);
                HttpResponse webResponse = sendMsgToWebserver(newHttpRequest, newRequest);
                if (webResponse.getStatusCode() == "400")
                {
                    sendFormedHttpResponse(webResponse, newHttpRequest, newRequest);
                    return;
                }
                if (webResponse.checkIsChunked())
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                    return;
                }
                if (webResponse.getStatusCode() != "304" && webResponse.getStatusCode() != "200")
                {
                    std::cout << "非304或200直接发送响应返回浏览器 " << std::endl;
                    sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
                    return;
                }
                if (!cache.isResForbiden(webResponse))
                {
                    std::cout << "响应不禁止缓存，存入缓存" << std::endl;
                    if (cache.isResMustRevalid(webResponse))
                    {
                        proxyLog.writeResCacheLogLine(webResponse, newRequest, "re-validation");
                    }
                    else
                    {
                        proxyLog.writeResCacheLogLine(webResponse, newRequest, "cached expired");
                    }
                    cache.put(webResponse.getRawResponseText(), cacheKey);
                    std::cout << "将缓存中的响应返回浏览器 " << std::endl;
                    std::string cachedResponse = cache.get(cacheKey);
                    sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
                    proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
                }
                else
                {
                    std::cout << "禁止缓存，直接发送响应" << std::endl;
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                    sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
                }
            }
            else
            {
                std::cout << "不需要强制验证 ";
                // 验证缓存是否新鲜
                if (cache.isFresh(cacheKey, newHttpRequest.getRequestTime()))
                {
                    std::cout << "缓存新鲜 ";
                    std::cout << "将缓存中的响应返回浏览器 " << std::endl;
                    proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "valid", "");
                    std::string cachedResponse = cache.get(cacheKey);
                    sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
                    proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
                }
                else
                {
                    std::cout << "缓存不新鲜 ";
                    std::string resp = cache.getCacheNodeFullResponse(cacheKey);
                    proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "expired", resp);
                    newHttpRequest.buildConRequest(ETag, LastModified);
                    HttpResponse webResponse = sendMsgToWebserver(newHttpRequest, newRequest);
                    if (webResponse.getStatusCode() == "400")
                    {
                        sendFormedHttpResponse(webResponse, newHttpRequest, newRequest);
                        return;
                    }
                    if (webResponse.checkIsChunked())
                    {
                        proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                        return;
                    }
                    if (webResponse.getStatusCode() != "304" && webResponse.getStatusCode() != "200")
                    {
                        std::cout << "非304或200直接发送响应返回浏览器 " << std::endl;
                        sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
                        return;
                    }
                    if (!cache.isResForbiden(webResponse))
                    {
                        std::cout << "响应不禁止缓存，存入缓存" << std::endl;
                        if (cache.isResMustRevalid(webResponse))
                        {
                            proxyLog.writeResCacheLogLine(webResponse, newRequest, "re-validation");
                        }
                        else
                        {
                            proxyLog.writeResCacheLogLine(webResponse, newRequest, "cached expired");
                        }
                        cache.put(webResponse.getRawResponseText(), cacheKey);
                        std::cout << "将缓存中的响应返回浏览器 " << std::endl;
                        std::string cachedResponse = cache.get(cacheKey);
                        sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
                        proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
                    }
                    else
                    {
                        std::cout << "响应禁止缓存，直接发送响应" << std::endl;
                        proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                        sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
                    }
                }
            }
        }
        else
        {
            std::cout << "代理中没有缓存 ";
            proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "not in cache", "");
            HttpResponse webResponse = sendMsgToWebserver(newHttpRequest, newRequest);
            std::cout << "605行:" << std::endl;
            if (webResponse.getStatusCode() == "400")
            {
                sendFormedHttpResponse(webResponse, newHttpRequest, newRequest);
                return;
            }
            if (webResponse.checkIsChunked())
            {
                proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                return;
            }
            if (webResponse.getStatusCode() != "304" && webResponse.getStatusCode() != "200")
            {
                std::cout << "非304或200直接发送响应返回浏览器 " << std::endl;
                sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
                return;
            }
            if (!cache.isResForbiden(webResponse))
            {
                std::cout << "响应不禁止缓存，存入缓存" << std::endl;
                // std::cout << "是否禁止：" << cache.isResForbiden(webResponse.getRawResponseText()) << std::endl;
                if (cache.isResMustRevalid(webResponse))
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "re-validation");
                }
                else
                {
                    proxyLog.writeResCacheLogLine(webResponse, newRequest, "cached expired");
                }
                cache.put(webResponse.getRawResponseText(), cacheKey);
                std::cout << "将缓存中的响应返回浏览器 " << std::endl;
                std::string cachedResponse = cache.get(cacheKey);
                sendMsgFromProxy(sockfd, cachedResponse.c_str(), cachedResponse.size());
                proxyLog.writeResponseLogLine(std::string("HTTP/1.1 200 OK"), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
            }
            else
            {
                std::cout << "响应禁止缓存，直接发送响应返回浏览器 " << std::endl;
                proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
                sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
            }
        }
    }
    else
    {
        std::cout << "请求禁用缓存 ";
        proxyLog.writeReqCacheLogLine(newHttpRequest, newRequest, "not in cache", "");
        HttpResponse webResponse = sendMsgToWebserver(newHttpRequest, newRequest);
        if (webResponse.checkIsChunked())
        {
            proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
            return;
        }
        std::cout << "直接发送响应返回浏览器 " << std::endl;
        proxyLog.writeResCacheLogLine(webResponse, newRequest, "not cacheable");
        sendMsgFromProxy(sockfd, webResponse.getRawResponseText().c_str(), webResponse.getRawResponseText().size());
    }
}

void Proxy::sendFormedHttpResponse(HttpResponse &formed_response, HttpRequest &newHttpRequest, void *newRequest)
{
    proxyLog.writeResponseLogLine(formed_response.getStartLine(), newRequest, newHttpRequest.getHost(), "from_proxy_to_browser");
    size_t raw_reponse_size = formed_response.getRawResponseText().size();
    sendMsgFromProxy(((Request *)newRequest)->getClientFd(), formed_response.getRawResponseText().c_str(), raw_reponse_size);
    return;
}

HttpResponse Proxy::getFormedHttpResponse(const int status_code)
{
    if (status_code == 400)
    {
        const char *HttpResponse_raw = "HTTP/1.1 400 Bad Request\r\n"
                                       "Content-Type: text/html; charset=UTF-8\r\n"
                                       "Date: Mon, 27 Feb 2023 18:53:18 GMT\r\n"
                                       "Content-Length: 82\r\n"
                                       "Sozu-Id: 01GTA3HNM6SFS02WVXDF112N5N\r\n\r\n"
                                       "<html><h2>Proxy Server send you a 400 bad Request(malformed Request)</h2></html>\r\n";
        return HttpResponse(std::string(HttpResponse_raw, strlen(HttpResponse_raw)));
    }
    else if (status_code == 502)
    {
        const char *HttpResponse_raw = "HTTP/1.1 502 Bad Gateway\r\n"
                                       "Content-Type: text/html; charset=UTF-8\r\n"
                                       "Date: Mon, 27 Feb 2023 19:16:41 GMT\r\n"
                                       "Content-Length: 83\r\n"
                                       "Sozu-Id: 01GTA4WDEGJF478FBYEC9BMZ7W\r\n\r\n"
                                       "<html><h2>Proxy Server send you a 502 Bad Gateway(corrupted Response)</h2></html>\r\n";
        return HttpResponse(std::string(HttpResponse_raw, strlen(HttpResponse_raw)));
    }
    else
    {
        exit(EXIT_FAILURE);
        std::cout << "wrong input status code" << std::endl;
    }
}