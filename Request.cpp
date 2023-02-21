#include "Request.hpp"


Request::Request(char* msg){
    raw_request_line = msg;
    request_id = next_request_id;
    next_request_id ++ ;
}



