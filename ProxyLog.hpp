#include <iostream>
#include <fstream>
#include <string>
#include <pthread.h>

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

class ProxyLog{
    private:
        std::ofstream logfile;
    public:
        void openLogFile(std::string path);
        void writeLogFile(std::string logLine);
        void closeLogFile();
};