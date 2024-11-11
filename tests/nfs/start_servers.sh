#!/bin/bash

# start the Mount server in the background
./build/mount_server &

sleep 1

# start the Nfs server in foreground
./build/nfs_server