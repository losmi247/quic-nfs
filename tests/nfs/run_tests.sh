#!/bin/bash

docker container rm -f nfs-and-mount-server
docker container rm -f nfs-test
docker network rm mount-test-net
docker network rm nfs-test-net

docker network create --driver bridge --subnet 192.168.0.0/16 nfs-test-net

docker run --detach --name nfs-and-mount-server --network=nfs-test-net --ip=192.168.100.1 nfs-and-mount-server:latest
sleep 1
docker run --name nfs-test --network=nfs-test-net --ip=192.168.100.2 nfs-test:latest