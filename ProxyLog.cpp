#include "ProxyLog.hpp"

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
void ProxyLog::openLogFile(std::string path){
    if(!logfile.is_open()){
        logfile.open(path, std::ios::out | std::ios::trunc);
    }
}

void ProxyLog::writeLogFile(std::string logLine){
    pthread_mutex_lock(&lock);
    logfile << logLine << std::endl;
    pthread_mutex_unlock(&lock);
}

void ProxyLog::closeLogFile(){
    if (logfile.is_open()){
        logfile.close();
    }
}