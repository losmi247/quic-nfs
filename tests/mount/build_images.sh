#!/bin/bash

docker image rm "mount-server:latest"
docker image rm "mount-test:latest"

docker build --tag mount-server --file ./tests/mount/Dockerfile.server .

docker build --tag mount-test --file ./tests/mount/Dockerfile.test_mount .