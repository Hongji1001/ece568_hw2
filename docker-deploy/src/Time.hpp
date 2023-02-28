#ifndef __HTTP_TIME_HPP__
#define __HTTP_TIME_HPP__
#include <ctime>
#include <string>
#include <iostream>
#include <cstdio>

class Time
{
public:
    static std::string getLocalUTC();
    static std::string gmtToUTC(const std::string &gmtTime);
    static size_t calTimeDiff(const std::string &time1, const std::string &time2);
};
#endif