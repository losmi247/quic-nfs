This is a user-space implementation of NFSv2 client-server pair.

# Build Instructions

Clone the directory

```
https://github.com/losmi247/quic-nfs.git
```

and:

1. Install **protobuf-c** from https://github.com/protobuf-c/protobuf-c - for generating serialization code
2. Run ```make all``` to generate serialization code and build the server and client
3. First run ```./build/mount_server``` and then ```./build/repl```