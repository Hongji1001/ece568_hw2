#!/bin/bash
make clean
make
echo 'start running proxy server...'
./proxy_daemon &
while true ; do continue ; done