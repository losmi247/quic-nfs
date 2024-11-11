#!/bin/bash

docker image rm "nfs-and-mount-server:latest"
docker image rm "nfs-test:latest"

docker build --tag nfs-and-mount-server --file ./tests/nfs/Dockerfile.server .

docker build --tag nfs-test --file ./tests/nfs/Dockerfile.test_nfs .