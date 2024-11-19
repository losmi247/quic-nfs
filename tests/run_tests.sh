#!/bin/bash

docker container rm -f mount-and-nfs-server
docker container rm -f mount-and-nfs-test
docker network rm nfs-test-net

docker network create --driver bridge --subnet 192.168.0.0/16 nfs-test-net

docker run --detach --name mount-and-nfs-server --network=nfs-test-net --ip=192.168.100.1 mount-and-nfs-server:latest
sleep 1
docker run --name mount-and-nfs-test --network=nfs-test-net --ip=192.168.100.2 mount-and-nfs-test:latest