#include "Time.hpp"
std::string Time::getLocalUTC()
{
    std::time_t now = time(0);
    std::tm *tm_gmt = gmtime(&now);
    std::string localUTCTime(asctime(tm_gmt));
    return localUTCTime;
}

std::string Time::gmtToUTC(const std::string &gmtTime)
{
    // covert to tm struct
    std::tm tm = {};
    strptime(gmtTime.c_str(), "%a, %d %b %Y %H:%M:%S %Z", &tm);

    // covert to UTC time
    std::time_t time = std::mktime(&tm);
    std::tm *utc_tm = std::gmtime(&time);
    std::string UTCTime(asctime(utc_tm));
    return UTCTime;
}

size_t Time::calTimeDiff(const std::string &time1, const std::string &time2)
{
    std::tm tm1 = {};
    std::tm tm2 = {};
    strptime(time1.c_str(), "%a %b %d %H:%M:%S %Y", &tm1);
    strptime(time2.c_str(), "%a %b %d %H:%M:%S %Y", &tm2);
    std::time_t time_t1 = std::mktime(&tm1);
    std::time_t time_t2 = std::mktime(&tm2);
    std::time_t diff = time_t1 - time_t2;
    return static_cast<size_t>(diff);
}