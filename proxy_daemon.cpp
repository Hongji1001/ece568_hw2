#include <cstdio>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vector>
#include <cstring>
#include <string>
#include <stdlib.h>


int main(){
    LOG(INFO)<<"testDaemon start";
    if(daemon(1, 0) == -1){
        std::cout<<"error\n"<<std::endl;
        exit(-1);
    }

    LOG(INFO)<<"testDaemon after daemon";
    int count=0;
    while (true){
        sleep(1);
        LOG(INFO)<<++count<<" : testDaemon ...";
//        std::cout<<"testDaemon..."<<std::endl;
    }
}

}