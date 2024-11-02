#!/bin/bash

docker container rm -f mount-server
docker container rm -f mount-test
docker network rm mount-test-net

docker network create --driver bridge --subnet 192.168.0.0/16 mount-test-net

docker run --detach --name mount-server --network=mount-test-net --ip=192.168.100.1 mount-server:latest
sleep 1
docker run --name mount-test --network=mount-test-net --ip=192.168.100.2 mount-test:latest


#sleep 3

# Stop the server
#docker stop mount-server
