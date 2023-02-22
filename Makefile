CFLAGS=-std=gnu++11 -pedantic -Wall -ggdb3
PROGS=testclient proxy_daemon
# PROGS=testHttpRequest ## DEBUG HttpRequest.cpp
OBJS=$(patsubst %,%.o,$(PROGS)) *.o

all: $(PROGS)
proxy_daemon: proxy_daemon.cpp Proxy.cpp Server.cpp Request.cpp httprequest.cpp client.cpp
	g++ -g $(CFLAGS) -o proxy_daemon proxy_daemon.cpp Proxy.cpp Server.cpp Request.cpp httprequest.cpp client.cpp -lpthread
testclient: testclient.cpp Server.cpp Request.cpp httprequest.cpp client.cpp
	g++ -g $(CFLAGS) -o testclient testclient.cpp Server.cpp Request.cpp httprequest.cpp client.cpp -lpthread

## DEBUG HttpRequest.cpp
testHttpRequest: testHttpRequest.cpp httprequest.cpp
	g++ -g $(CFLAGS) -o testHttpRequest testHttpRequest.cpp httprequest.cpp

.PHONY: clean
clean:
	rm -f *~ $(PROGS) $(OBJS)