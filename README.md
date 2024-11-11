This is a user-space implementation of NFSv2 client-server pair.

# Build Instructions

Clone the directory

```
https://github.com/losmi247/quic-nfs.git
```

and:

1. Install **protobuf-c** from https://github.com/protobuf-c/protobuf-c - for generating serialization code (follow the commands performed in the docker image for testing to install all the libraries)
2. Run ```make serialization_library``` to generate serialization code
3. Run ```make all``` to build the server and client
4. First run ```./build/mount_server``` and then ```./build/repl```

# Serialization

After changing ```.proto``` files, regenerate serialization code by running ```make serialization_library```. After doing this, some of the ```*.pb-c.h``` files that use Empty and Any Google protobuf messages will will need to have e.g. ```#include "google/protobuf/any.pb-c.h"``` replaced with ```#include "src/serialization/google_protos/any.pb-c.h"```.

# Tests

To run the tests, build the Docker images for server and client using the ```./tests/mount/build_images.sh``` script, and then run them as Docker containers using ```./tests/mount/run_tests.sh```.