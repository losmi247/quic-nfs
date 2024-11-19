#!/bin/bash

docker image rm "mount-and-nfs-server:latest"
docker image rm "mount-and-nfs-test:latest"

docker build --tag mount-and-nfs-server --file ./tests/Dockerfile.server .

docker build --tag mount-and-nfs-test --file ./tests/Dockerfile.test_nfs .