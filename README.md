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

# Tests

Tests are written https://github.com/Snaipe/Criterion testing framework. 

To run the tests:
- build the Mount+Nfs server using ```make all```
- build the tests using ```make test```
- build Docker images for the server and the client using ```./tests/build_images```
- finally, run the tests using ```./tests/run_tests```

# Procedure Support Progress

|  N  | Procedure      | Description                                  |  Server procedure   |  Client-side function |        Tests       |
|-----|----------------|----------------------------------------------|---------------------|-----------------------|--------------------|
|  0  | NULL           | do nothing                                   |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  1  | GETATTR        | get attributes                               |   done &#10004;     |   done &#10004;       |   done &#10004;    | 
|  2  | SETATTR        | set attributes                               |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  3  | ROOT           | get root NFS fhandle                         |     obsolete        |     obsolete          |     obsolete       |
|  4  | LOOKUP         | get NFS fhandle and attributes of a file     |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  5  | READLINK       | read from symbolic link                      |                     |                       |                    |
|  6  | READ           | read from file                               |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  7  | WRITECACHE     | write to cache                               |      NFSv3          |     NFSv3             |        NFSv3       |
|  8  | WRITE          | write to file                                |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  9  | CREATE         | create file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 10  | REMOVE         | remove file                                  |                     |                       |                    |
| 11  | RENAME         | rename file                                  |                     |                       |                    |
| 12  | LINK           | create link to file                          |                     |                       |                    |
| 13  | SYMLINK        | create symbolic link to file                 |                     |                       |                    |
| 14  | MKDIR          | create directory                             |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 15  | RMDIR          | remove directory                             |                     |                       |                    |
| 16  | READDIR        | read from directory                          |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 17  | STATFS         | get filesystem attributes                    |                     |                       |                    |