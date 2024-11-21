This is a user-space implementation of NFSv2 client-server pair ([RFC 1094](https://datatracker.ietf.org/doc/html/rfc1094)) over QUIC.

# Build Instructions

Clone the directory

```
git clone https://github.com/losmi247/quic-nfs.git
```

and:

1. Install [**protobuf**](https://github.com/protocolbuffers/protobuf), [**protobuf-c**](https://github.com/protobuf-c/protobuf-c), **criterion**, and other project dependencies using:

    ```
    ./install_dependencies
    ```
2. Run ```make serialization_library``` to generate serialization code
3. Run ```make all``` to build the server and client
4. First run ```./build/mount_and_nfs_server``` and then ```./build/repl```

The Nfs and Mount server are implemented as a single process, to allowed efficient sharing of the cache containing mappings of inode numbers to files/directories.

# Serialization

After changing ```.proto``` files, regenerate serialization code by running ```make serialization_library```. After doing this, some of the ```*.pb-c.h``` files that use Empty and Any Google protobuf messages will will need to have e.g. ```#include "google/protobuf/any.pb-c.h"``` replaced with ```#include "src/serialization/google_protos/any.pb-c.h"```.

# Tests

Tests are written https://github.com/Snaipe/Criterion testing framework. 

To run the tests:
- build the Mount+Nfs server using ```make all```
- build the tests using ```make test```
- build Docker images for the server and the client using ```./tests/build_images```
- finally, run the tests using ```./tests/run_tests```