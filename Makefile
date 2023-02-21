CFLAGS=-std=gnu++98 -pedantic -Wall -Werror -ggdb3 
PROGS=proxy_daemon testclient


all: $(PROGS)
proxy_daemon: proxy_daemon.cpp
	g++ $(CFLAGS) -o $@ $<
testclient: testclient.cpp
	g++ $(CFLAGS) -o $@ $<
.PHONY: clean
clean:
	rm -f *~ $(PROGS)

