This is a user-space implementation of a **NFSv2 client-server pair** ([RFC 1094](https://datatracker.ietf.org/doc/html/rfc1094)) over QUIC.

# Directions For Use

After cloning the directory:

1. Install [**protobuf**](https://github.com/protocolbuffers/protobuf), [**protobuf-c**](https://github.com/protobuf-c/protobuf-c), [**criterion**](https://github.com/Snaipe/Criterion), and other project dependencies by running:
    ```
    ./install_dependencies
    ```
2. Run ```make serialization-library``` to generate serialization code
3. Run ```make all``` to build the Nfs+Mount server and the REPL Nfs client
4. Create an arbitrary directory you want to export for NFS, e.g. ```/nfs_share```, and create a file ```exports``` with the following content:
    ```
    /nfs_share *(rw)
    ```
   and place it in the same cw-directory from where you are going to run the NFS server in the next step
5. Run ```./build/mount_and_nfs_server 3000``` to start the NFS+MOUNT server on the server machine at port ```3000``` (e.g. IPv4 ```192.168.100.1```)
6. On the client machine, run ```./build/repl``` to start the NFS client Read-Eval-Print Loop, and use it to mount the exported NFS share as:
   ```
    mount 192.168.100.1 3000 /nfs_share
   ```

Note that the Nfs and Mount server are implemented as a single process, to allow efficient sharing of the cache containing mappings of inode numbers to files/directories.

# NFS Server

The NFSv2 server currently supports the following NFSv2 procedures:

|  **N**  | **Procedure**      | **Description**                                  |  **Server procedure**   |  **Client-side function** |        **Tests**       |
|-----|----------------|----------------------------------------------|---------------------|-----------------------|--------------------|
|  0  | **NULL**           | do nothing                                   |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  1  | **GETATTR**        | get attributes                               |   done &#10004;     |   done &#10004;       |   done &#10004;    | 
|  2  | **SETATTR**        | set attributes                               |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  3  | **ROOT**           | get root NFS fhandle                         |     obsolete        |     obsolete          |     obsolete       |
|  4  | **LOOKUP**         | get NFS fhandle and attributes of a file     |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  5  | **READLINK**       | read from symbolic link                      |                     |                       |                    |
|  6  | **READ**           | read from file                               |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  7  | **WRITECACHE**     | write to cache                               |      NFSv3          |     NFSv3             |        NFSv3       |
|  8  | **WRITE**          | write to file                                |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  9  | **CREATE**         | create file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 10  | **REMOVE**         | remove file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 11  | **RENAME**         | rename file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 12  | **LINK**           | create link to file                          |                     |                       |                    |
| 13  | **SYMLINK**        | create symbolic link to file                 |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 14  | **MKDIR**          | create directory                             |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 15  | **RMDIR**          | remove directory                             |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 16  | **READDIR**        | read from directory                          |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 17  | **STATFS**         | get filesystem attributes                    |   done &#10004;     |   done &#10004;       |   done &#10004;    |

# NFS Client as a User-Space REPL

The NFSv2 client is implemented as a user-space application that acts as a Read-Eval-Print Loop that parses Unix-like file management commands (```mount```, ```ls```, ```cat```, etc.) and carries them out by sending an appropriate sequence of Remote Procedure Calls (RPC) to the server.

The client REPL currently supports the following set of Unix-like commands:

| **Command** | **Description**                                                                 |
|-------------|---------------------------------------------------------------------------------|
| `mount <server_ip> <port> <remote_path>`     | Mounts a remote file system to the client REPL.                           |
| `ls`        | Lists all entries in the current working directory on the currently mounted remote file system.            |
| `cd <directory name>`  | change cwd to the given directory name which is in the current working directory                |

# Authentication

Currently supported authentication flavors are:

| **Command** | **Description**                                                                 |
|-------------|---------------------------------------------------------------------------------|
| `AUTH_NONE`     | no authentication                           |
| `AUTH_SYS`        | Unix-style authentication            |
| `AUTH_SHORT`  | not supported yet                |

# Tests

Tests are written using [**Criterion**](https://github.com/Snaipe/Criterion) testing framework.

To run the tests:
- build the MOUNT+NFS server using ```make all```
- build the tests using ```make test```
- build Docker images for the server and the client using ```./tests/build_images```
- finally, run the tests using ```./tests/run_tests```