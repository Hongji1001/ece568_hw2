#include "Cache.hpp"
void Cache::addFromHead(CacheNode *nodeToAdd)
{
    if (head == NULL)
    {
        head = nodeToAdd;
        tail = nodeToAdd;
        size++;
        return;
    }
    nodeToAdd->next = head->next;
    head->next->prev = nodeToAdd;
    head = nodeToAdd;
    size++;
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
        addFromHead(nodeToMove);
        return;
    }
    tail = nodeToMove->prev;
    nodeToMove->prev->next = NULL;
    nodeToMove->prev = NULL;
    addFromHead(nodeToMove);
}

void Cache::removeTail()
{
    if (tail == NULL)
    {
        return;
    }
    CacheNode *tempTail = tail;
    if (head == tail)
    {
        head = NULL;
    }
    tail = tail->prev;
    delete tempTail;
    size--;
};

Cache::Cache(unsigned int cap) : CAPACITY(cap), size(0), head(NULL), tail(NULL) {}

void Cache::put(const HttpResponse &response, const std::string &cacheKey)
{
    // 如果是协商缓存
    if (isCached(cacheKey))
    {
        // 更新CacheNode的Etag和responseTime
        CacheNode *cachedRes = cacheMap[cacheKey];
        cachedRes->responseTime = Time::getLocalUTC();
        cachedRes->Etag = response.getHeadmap()["Etag"];
        // 如果状态码是304, 不更新消息体，只更新行和头
        if (response.getStatusCode() == "304")
        {
        }
        // 否则，只更新消息体以及行和头
        else if (response.getStatusCode() == "200")
        {
            cachedRes->rawResponseBody = response.getMsgBody();
        }
        else
        {
            // TODO:如果是其他的状态码，怎么办？
        }
        // 更新响应行和头
        cachedRes->rawResponseStartLine = response.getStartLine();
        cachedRes->rawResponseHead = response.getHead();
        // 移至链表最前方
        moveToHead(cachedRes);
        return;
    }
    // 如果是强制缓存
    // TODO:检查缓存策略的工作交给proxy做吧
    // 缓存满了,删除最后一个结点
    if (isFull())
    {
        removeTail();
    }
    // 新建一个CacheNode加入双向链表，也加入cacheMap
    CacheNode *newCache = new CacheNode(response);
    // 将新建的CacheNode添加到头部
    addFromHead(newCache);
    // 加入cacheMap
    cacheMap[cacheKey] = newCache;
}

std::string Cache::get(const std::string &cacheKey)
{
    CacheNode *res = cacheMap[cacheKey];
    return res->getFullResponse();
}

// danger log: do not take multiple response into consideration
bool Cache::isCached(const std::string &cacheKey)
{
    return cacheMap.count(cacheKey) != 0;
};

bool Cache::isFull()
{
    return size >= CAPACITY;
}

bool Cache::isFresh(const std::string &cacheKey, const std::string &requestTime)
{
    // 不检查no-cache, no-store字段，这两个字段的检查交给proxy完成
    // isFresh方法仅查找max-age字段
    CacheNode *resToCheck = cacheMap[cacheKey];
    // 这里需要把所有三个东西加起来再解析一遍
    HttpResponse tempResponse = HttpResponse(resToCheck->getFullResponse());
    // danger log: 默认响应中存在Date字段
    // Date需要转换为UTC时间
    std::string Date = Time::gmtToUTC(tempResponse.getHeapMap()["Date"]);
    // 这里要实现找到max-age字段
    size_t maxAge = tempResponse.getMaxAge();
    // danger log: 默认响应中不存在Age字段
    size_t age = 0;
    // std::string requestTime = resToCheck->requestTime;
    std::string responseTime = resToCheck->responseTime;
    size_t responseDelay = Time::calTimeDiff(responseTime, requestTime);
    std::string now = Time::getLocalUTC();
    size_t residentTime = Time::calTimeDiff(now, responseTime);
    size_t apparentAge = std::max(0, int(Time::calTimeDiff(responseTime, Date)));
    size_t correctAgeValue = age + responseDelay;
    size_t correctedInitialAge = std::max(apparentAge, correctAgeValue);
    size_t currentAge = correctedInitialAge + residentTime;
    return currentAge < maxAge;
}

CacheNode::CacheNode(const HttpResponse &response)
{
    rawResponseStartLine = response.getStartLine();
    rawResponseHead = response.getHead();
    rawResponseBody = response.getMsgBody();
    Etag = response.getHeadmap()["Etag"];
    responseTime = Time::getLocalUTC();
    prev = NULL;
    next = NULL;
}

std::string CacheNode::getFullResponse()
{
    return rawResponseStartLine + rawResponseHead + rawResponseBody;
}
