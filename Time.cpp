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