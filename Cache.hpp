#ifndef __PROXYCACHE_HPP__
#define __PROXYCACHE_HPP__
#include <map>
#include <string>
#include "httprequest.hpp"
#include "HttpResponse.hpp"
#include "Time.hpp"
#define CACHE_CAPACITY 100
class CacheNode
{
public:
    std::string rawResponseStartLine;
    std::string rawResponseHead; // max-age, raw reponse, date/age_value can be parsed from it
    std::string rawResponseBody;
    std::string responseTime;
    std::string ETag;
    std::string LastModified;
    CacheNode *prev;
    CacheNode *next;
    CacheNode(const HttpResponse &response); // 放入响应体，同时自动记录放入响应体的时间
    std::string getFullResponse();
};

class Cache
{
private:
    unsigned int CAPACITY;                       // max number of nodes in cache
    unsigned int size;                           // current number of nodes in cache
    std::map<std::string, CacheNode *> cacheMap; // map of URI and reponse (metadata)
    CacheNode *head;
    CacheNode *tail;
    void addFromHead(CacheNode *nodeToAdd); // 新增缓存的话，将其添加到头部，并将size+1
    void moveToHead(CacheNode *nodeToMove); // 协商缓存更新etag后，要移动到链表头
    void removeTail();                      // 缓存满了后，要从链表尾部删除缓存节点
    // 按照response_time的时间顺序排列cache key
    // 节点中存cache key, max-age, reponseTime, Date首部, age_value(如果响应里有就用响应里的，否则用0), request_time(导致本次响应的请求发出的时间)
    // request_time不好搞啊
    // 利用响应时间来实现LRU缓存策略
public:
    Cache();
    Cache(unsigned int cap);
    void put(const HttpResponse &Response, const std::string &cacheKey); // 放入缓存,放入链表头,加入cacheMap

    std::string get(const std::string &cacheKey);                              // 从cacheMap中拿出缓存的响应,必须经过验证才行
    bool isCached(const std::string &cacheKey);                                // 判断请求是否已经被缓存
    bool isFull();                                                             // 判断缓存是否已经满了
    bool isFresh(const std::string &cacheKey, const std::string &requestTime); // 检验新鲜度
    bool isReqForbiden(const HttpRequest &request);                            // 检查请求是否禁用缓存
    bool isResForbiden(const HttpResponse &response);                          // 检查响应是否禁用缓存
    bool isReqMustRevalid(HttpRequest &request);                               // 检查请求是否强制验证
    bool isResMustRevalid(const std::string &cacheKey);                        // 检查缓存是否强制验证
    // 验证缓存是否过期,前提是cacheMap中包含cachekey
    std::map<std::string, CacheNode *> getCacheMap() const;
    ~Cache();
};

#endif