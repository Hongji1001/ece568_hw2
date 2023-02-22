#ifndef __PROXYCACHE_HPP__
#define __PROXYCACHE_HPP__
#include <map>
#include <string>
#include "HttpResponse.hpp"
class Cache
{
private:
    unsigned int CAPACITY;                       // max number of nodes in cache
    unsigned int size;                           // current number of nodes in cache
    std::map<std::string, CacheNode *> cacheMap; // map of URI and reponse (metadata)
    CacheNode *head;
    CacheNode *tail;
    // 按照response_time的实践顺序排列cache key
    // 节点中存cache key, max-age, reponseTime, Date首部, age_value(如果响应里有就用响应里的，否则用0), request_time(导致本次响应的请求发出的时间)
    // request_time不好搞啊
    // 利用响应时间来实现LRU缓存策略
    void put(){};                             // 强制缓存,放入链表头,加入cacheMap
    std::string get() const {};               // 从cacheMap中拿出缓存的响应
    void moveToHead(CacheNode *nodeToMove){}; // 协商缓存更新etag后，要移动到链表头
    void removeFromTail(){};                  // 缓存满了后，要从链表尾部删除缓存节点
    bool isCached(std::string){};             // 判断请求是否已经被缓存
    bool validation(std::string){};           // 验证缓存是否过期,前提是cacheMap中包含cachekey
};

class CacheNode
{
public:
    std::string rawResponse; // max-age, raw reponse, date/age_value can be parsed from it
    std::string responseTime;
    std::string requestTime; // to be determined
    CacheNode *prev;
    CacheNode *next;
    CacheNode(){}; // 放入响应体，同时自动记录放入响应体的时间
};

#endif