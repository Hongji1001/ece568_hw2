#include "Cache.hpp"
void Cache::addFromHead(CacheNode *nodeToAdd, int mode)
{
    if (head == NULL)
    {
        head = nodeToAdd;
        tail = nodeToAdd;
        if (mode == 0)
        {
            size++;
        }
        return;
    }
    nodeToAdd->next = head;
    head->prev = nodeToAdd;
    head = nodeToAdd;
    if (mode == 0)
    {
        size++;
    }
}

void Cache::moveToHead(CacheNode *nodeToMove)
{
    if (nodeToMove == head)
    {
        return;
    }
    if (nodeToMove != tail)
    {
        nodeToMove->next->prev = nodeToMove->prev;
        nodeToMove->prev->next = nodeToMove->next;
        nodeToMove->next = NULL;
        nodeToMove->prev = NULL;
        addFromHead(nodeToMove, 1);
        return;
    }
    tail = nodeToMove->prev;
    nodeToMove->prev->next = NULL;
    nodeToMove->prev = NULL;
    addFromHead(nodeToMove, 1);
}

void Cache::removeTail()
{
    if (tail == NULL)
    {
        return;
    }
    CacheNode *tempTail = tail;
    std::string cacheKey = tempTail->cacheKey;
    if (head == tail)
    {
        head = NULL;
    }
    tail = tail->prev;
    delete tempTail;
    std::cout << "start to reduce size" << std::endl;
    size--;
    std::cout << "start to remove" << std::endl;
    // std::lock_guard<std::mutex> lock(mtx);
    std::cout << "start to lock" << std::endl;
    auto it = cacheMap.find(cacheKey);
    if (it != cacheMap.end())
    {
        cacheMap.erase(it);
    }
    std::cout << "unlock" << std::endl;
}
Cache::Cache() : CAPACITY(CACHE_CAPACITY), size(0), head(NULL), tail(NULL) {}
Cache::Cache(unsigned int cap) : CAPACITY(cap), size(0), head(NULL), tail(NULL) {}

void Cache::put(std::string rawResponse, const std::string &cacheKey)
{
    /// std::cout << rawResponse << std::endl;
    HttpResponse response(rawResponse);
    std::cout << "成功进入缓存，缓存容量为 " << this->CAPACITY << " 当前大小为 " << this->size << std::endl;
    // 验证响应是否禁用缓存
    // 如果是协商缓存
    if (isCached(cacheKey))
    {
        // 更新CacheNode的Etag和responseTime
        // std::lock_guard<std::mutex> lock(mtx);
        CacheNode *cachedRes = cacheMap[cacheKey];
        cachedRes->responseTime = Time::getLocalUTC();
        if (response.getHeaderMap().count("etag") != 0)
        {
            cachedRes->ETag = response.getHeaderMap()["etag"];
        }
        if (response.getHeaderMap().count("last-modified") != 0)
        {
            cachedRes->LastModified = response.getHeaderMap()["last-modified"];
        }
        if (response.getStatusCode() == "304")
        {
            // 如果是304状态码且是条件请求
            // 储存更新为200状态码的响应行,
            // 储存发回响应的头，响应体不用更新
            cachedRes->rawResponseStartLine = response.getHttpVersion() + " 200 OK";
        }
        else
        {
            // 如果是其他状态码
            // 储存发回响应的行，头，体
            cachedRes->rawResponseStartLine = response.getStartLine();
            cachedRes->rawResponseBody = response.getMsgBody();
        }
        // 储存发回响应的头
        // cachedRes->rawResponseStartLine = response.getStartLine();
        cachedRes->rawResponseHead = response.getHead();
        // 移至链表最前方
        moveToHead(cachedRes);
        return;
    }
    // 如果是强制缓存
    // std::lock_guard<std::mutex> lock(mtx);
    std::cout << "初次缓存" << std::endl;
    // 缓存满了,删除最后一个结点
    if (isFull())
    {
        std::cout << "enter into remove tail func" << std::endl;
        if (tail == NULL)
            std::cout << "tail pointer is NULL and enter to remove tail" << std::endl;
        removeTail();
    }
    // 新建一个CacheNode加入双向链表，也加入cacheMap
    std::cout << std::endl;
    // std::cout << response.getMsgBody() << std::endl;
    std::cout << "接收到的响应内容：" << std::endl;
    CacheNode *newCache = new CacheNode(response, cacheKey);
    std::cout << response.getHead() << std::endl;
    std::cout << "HTTP version: " << response.getHttpVersion() << std::endl;
    std::cout << "ETag: " << newCache->ETag << std::endl;
    std::cout << "LastModified: " << newCache->LastModified << std::endl;
    std::cout << "responseTime: " << newCache->responseTime << std::endl;
    newCache->rawResponseStartLine = response.getHttpVersion() + " 200 OK";
    std::cout << newCache->rawResponseStartLine << std::endl;
    // 将新建的CacheNode添加到头部
    std::cout << "开始将响应存入缓存：" << std::endl;
    addFromHead(newCache, 0);
    std::cout << "已将新缓存添加到头部" << std::endl;
    // 加入cacheMap
    cacheMap[cacheKey] = newCache;
    std::cout << "加入缓存的响应行为：" << cacheMap[cacheKey]->rawResponseStartLine << std::endl;
    // std::cout << "加入缓存的响应体为：" << cacheMap[cacheKey]->rawResponseBody << std::endl;
}

std::string Cache::get(std::string &cacheKey)
{
    return getCacheNodeFullResponse(cacheKey);
}

// danger log: do not take multiple response into consideration
bool Cache::isCached(const std::string &cacheKey)
{
    // std::lock_guard<std::mutex> lock(mtx);
    bool flag = cacheMap.count(cacheKey) != 0;
    return flag;
}

bool Cache::isFull()
{
    return size >= CAPACITY;
}

bool Cache::isFresh(std::string &cacheKey, const std::string &requestTime)
{
    // 不检查no-cache, no-store字段，这两个字段的检查交给proxy完成
    // isFresh方法仅查找max-age字段
    // 这里需要把所有三个东西加起来再解析一遍
    HttpResponse tempResponse = HttpResponse(getCacheNodeFullResponse(cacheKey));
    // danger log: 默认响应中存在Date字段
    // Date需要转换为UTC时间
    std::string Date = Time::gmtToUTC(tempResponse.getHeaderMap()["date"]);
    // 这里要实现找到max-age字段
    size_t maxAge = tempResponse.getMaxAge();
    // danger log: 默认响应中不存在Age字段
    size_t age = 0;
    // std::string requestTime = resToCheck->requestTime;
    std::string responseTime = getCacheNodeResTime(cacheKey);
    size_t responseDelay = Time::calTimeDiff(responseTime, requestTime);
    std::string now = Time::getLocalUTC();
    size_t residentTime = Time::calTimeDiff(now, responseTime);
    size_t apparentAge = std::max(0, int(Time::calTimeDiff(responseTime, Date)));
    size_t correctAgeValue = age + responseDelay;
    size_t correctedInitialAge = std::max(apparentAge, correctAgeValue);
    size_t currentAge = correctedInitialAge + residentTime;
    return currentAge < maxAge;
}

bool Cache::isReqForbiden(const HttpRequest &request)
{
    return !(request.getHeaderMap().count("cache-control") == 0 ||
             (request.getHeaderMap().count("cache-control") != 0 &&
              (request.getHeaderMap()["cache-control"].find("private") == std::string::npos && request.getHeaderMap()["cache-control"].find("no-store") == std::string::npos)));
}

bool Cache::isResForbiden(const HttpResponse &response)
{
    return !(response.getHeaderMap().count("cache-control") == 0 ||
             (response.getHeaderMap().count("cache-control") != 0 &&
              (response.getHeaderMap()["cache-control"].find("private") == std::string::npos && response.getHeaderMap()["cache-control"].find("no-store") == std::string::npos)));
}

bool Cache::isReqMustRevalid(HttpRequest &request)
{
    return !(request.getHeaderMap().count("cache-control") == 0 ||
             (request.getHeaderMap()["cache-control"].find("no-cache") == std::string::npos &&
              request.getHeaderMap()["cache-control"].find("must-revalidate") == std::string::npos &&
              request.getHeaderMap()["cache-control"].find("max-age=0") == std::string::npos));
}

bool Cache::isResMustRevalid(std::string &cacheKey)
{
    HttpResponse fullResponse(getCacheNodeFullResponse(cacheKey));
    return !(fullResponse.getHeaderMap().count("cache-control") == 0 ||
             (fullResponse.getHeaderMap()["cache-control"].find("no-cache") == std::string::npos &&
              fullResponse.getHeaderMap()["cache-control"].find("must-revalidate") == std::string::npos &&
              fullResponse.getHeaderMap()["cache-control"].find("max-age=0") == std::string::npos));
}

bool Cache::isResMustRevalid(HttpResponse &fullResponse)
{
    return !(fullResponse.getHeaderMap().count("cache-control") == 0 ||
             (fullResponse.getHeaderMap()["cache-control"].find("no-cache") == std::string::npos &&
              fullResponse.getHeaderMap()["cache-control"].find("must-revalidate") == std::string::npos &&
              fullResponse.getHeaderMap()["cache-control"].find("max-age=0") == std::string::npos));
}

std::string Cache::getCacheNodeFullResponse(std::string &cacheKey)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::string full = cacheMap[cacheKey]->getFullResponse();
    return full;
}

std::string Cache::getCacheNodeETag(std::string &cacheKey)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::string ETag = cacheMap[cacheKey]->ETag;
    return ETag;
}

std::string Cache::getCacheNodeLastModified(std::string &cacheKey)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::string LM = cacheMap[cacheKey]->LastModified;
    return LM;
}

std::string Cache::getCacheNodeResTime(std::string &cacheKey)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::string RT = cacheMap[cacheKey]->responseTime;
    return RT;
}

std::string Cache::getCacheRawResHead(std::string &cacheKey)
{
    // std::lock_guard<std::mutex> lock(mtx);
    std::string RRH = cacheMap[cacheKey]->rawResponseHead;
    return RRH;
}

Cache::~Cache()
{
    for (auto it = cacheMap.begin(); it != cacheMap.end(); ++it)
    {
        delete it->second;
    }
    std::cout << "缓存已释放" << std::endl;
}

CacheNode::CacheNode(const HttpResponse &response, const std::string &cacheKey)
{
    rawResponseStartLine = response.getStartLine();
    rawResponseHead = response.getHead();
    rawResponseBody = response.getMsgBody();
    this->cacheKey = cacheKey;
    if (response.getHeaderMap().count("etag") != 0)
    {
        ETag = response.getHeaderMap()["etag"];
    }
    if (response.getHeaderMap().count("last-modified") != 0)
    {
        LastModified = response.getHeaderMap()["last-modified"];
    }
    responseTime = Time::getLocalUTC();
    prev = NULL;
    next = NULL;
}

std::string CacheNode::getFullResponse()
{
    return rawResponseStartLine + "\r\n" + rawResponseHead + "\r\n" + rawResponseBody;
}
