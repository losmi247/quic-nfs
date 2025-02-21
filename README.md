This is a user-space implementation of a **NFSv2 client-server pair** ([RFC 1094](https://datatracker.ietf.org/doc/html/rfc1094)) over QUIC.

# Directions For Use

After cloning the directory:

1. Install [**Protobuf**](https://github.com/protocolbuffers/protobuf), [**Protobuf-C**](https://github.com/protobuf-c/protobuf-c), [**Criterion**](https://github.com/Snaipe/Criterion), [**TQUIC**](https://github.com/Tencent/tquic), [**FUSE**](https://github.com/libfuse/libfuse), and other project dependencies by running:
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
5. On the server machine, run: 
   ```
   sudo ./build/mount_and_nfs_server <port> --proto=<transport_protocol>
   ``` 
   to start the NFS+MOUNT server at port ```port``` (e.g. ```3000```), where ```transport_protocol``` is either ```tcp``` or ```quic```. The **NFS server always runs as root**.
6. To run the NFS client, please follow the instruction either in the *NFS Client as a FUSE File System* or in *NFS Client as a User-Space REPL*  
   
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
|  5  | **READLINK**       | read from symbolic link                      |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  6  | **READ**           | read from file                               |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  7  | **WRITECACHE**     | write to cache                               |      NFSv3          |     NFSv3             |        NFSv3       |
|  8  | **WRITE**          | write to file                                |   done &#10004;     |   done &#10004;       |   done &#10004;    |
|  9  | **CREATE**         | create file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 10  | **REMOVE**         | remove file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 11  | **RENAME**         | rename file                                  |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 12  | **LINK**           | create hard link to file                     |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 13  | **SYMLINK**        | create symbolic link to file                 |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 14  | **MKDIR**          | create directory                             |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 15  | **RMDIR**          | remove directory                             |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 16  | **READDIR**        | read from directory                          |   done &#10004;     |   done &#10004;       |   done &#10004;    |
| 17  | **STATFS**         | get filesystem attributes                    |   done &#10004;     |   done &#10004;       |   done &#10004;    |

# NFS Client

The NFSv2 client was implemented in two similar flavours - as a FUSE file system, and as a custom user-space read-eval-print-loop.

## NFS Client as a FUSE File System

[**FUSE**](https://github.com/libfuse/libfuse) was used to expose the mounted remote filsystem (backed by the NFS server implementation) as a user-space file system on the local machine.

To start this FUSE file system, after *step 5* in the *Directions For Use* section (i.e. after the server is started) do the following on the client machine:

```
make fuse-fs
```

followed by

```
./build/nfs_fuse <server IPv4 address> <port number> --proto=<tcp or quic> <remote absolute path> <local mount point>
```

Make sure to select the same transport protocol as the one that your server is using.

For example, for a QUIC server running at port ```3000``` at ```192.168.100.1```, exporting the file system ```/nfs_share```, to mount this as a FUSE file system at ```~/Desktop/mountpoint```, you need to do:

```
./build/nfs_fuse 192.168.100.1 3000 --proto=quic /nfs_share ~/Desktop/mountpoint
```

After this, you can use the filesystem as you would normally, using any Linux file management commands.

## NFS Client as a User-Space REPL

Another implementation of the NFSv2 client is implemented as a user-space application that acts as a Read-Eval-Print Loop that parses Unix-like file management commands (```mount```, ```ls```, ```cat```, etc.) and carries them out by sending an appropriate sequence of Remote Procedure Calls (RPC) to the server.

To start the REPL, after *step 5* in the *Directions For Use* section (i.e. after the server is started) do the following on the client machine:

```
./build/repl
```

This will prompt you to **select a transport protocol** - make sure to select the same transport protocol as the one chosen for the NFS server. 
Finally, use the REPL to mount the exported NFS share as ```mount 192.168.100.1 3000 /nfs_share``` (assuming the server machine is at IPv4 192.168.100.1).

The client REPL currently supports the following set of Unix-like commands:

| **Command** | **Description**                                                                 |
|-------------|---------------------------------------------------------------------------------|
| `mount <server ip> <port> <remote path>`     | Mounts a remote file system to the client REPL.                           |
| `ls`        | Lists all entries in the current working directory on the currently mounted remote file system.            |
| `cd <directory name>`  | change cwd to the given directory name which is in the current working directory                |
| `touch <file name>`  | create a file in the current working directory                |
| `mkdir <directory name>`  | create a directory in the current working directory           |
| `cat <file name>`  | prints out the contents of a file in the current working directory           |
| `echo '<text>' >> <file name>`  | appends the given text to the end of a file in the current working directory, in a new line          |
| `rm <file name>`  | removes a file in the current working directory        |
| `rmdir <directory name>`  | removes a directory in the current working directory        |

When the REPL is started, the user is able to select between **TCP** and **QUIC** for transport.


# Transport

This NFS implementation can operate over TCP or QUIC. 

The **TCP interface** was built using the standard Linux sockets API.

The **QUIC interface** was built using Tencent's implementation of QUIC - [**TQUIC**](https://github.com/Tencent/tquic), combined with Linux sockets API for UDP transport.

# Authentication

Currently supported RPC ([**RFC 5531**](https://datatracker.ietf.org/doc/html/rfc5531)) authentication flavors are:

| **Command** | **Description**                                                                 |
|-------------|---------------------------------------------------------------------------------|
| `AUTH_NONE`     | no authentication                           |
| `AUTH_SYS`        | Unix-style authentication            |
| `AUTH_SHORT`  | not supported yet                |

# Tests

Tests are written using [**Criterion**](https://github.com/Snaipe/Criterion) testing framework.

To run the tests:
- build Docker images for the server and the tests (client) over TCP/QUIC using ```./tests/build_images_tcp``` and ```./tests/build_images_quic``` respectively
- run the tests for NFS over TCP/QUIC using ```./tests/run_tests_tcp``` or ```./tests/run_tests_quic``` respectively
# Benchmarks

[**FIO**](https://github.com/axboe/fio) was used for benchmarking the performances of different versions of NFSv2. The *benchmarks* folder contains *.fio* benchmark files and their respective logs.

In order to run a FIO benchmark file we write:

```fio <.fio benchmark file> --output=<output file>```

## Physical Link Benchmarks (With Simulated Latency and Packet Loss)

These benchmarks were set up so that the NFS client was a Raspberry PI with its ethernet port at static address 192.168.1.1, connected 
by a physical link to a laptop's ethernet port at static address 192.168.1.2. The laptop is running a TCP NFS server at port 3000 and
a QUIC NFS server at port 3001.

The directories exported from the two servers were mounted on the PI as a FUSE file system:

```./build/fuse_fs 192.168.1.2 3000 --proto=tcp /home/milos/Desktop/nfs_share ~/Desktop/mountpoint_tcp```
```./build/fuse_fs 192.168.1.2 3001 --proto=quic /home/milos/Desktop/nfs_share ~/Desktop/mountpoint_quic```

The FIO benchmarks were rerun in different environment where **simulated latency and packet loss** were applied on the
PI's network interface ```eth0```,  using ```tc``` as:

```sudo tc qdisc add dev eth0 root netem delay 30ms loss random 1%``` (add a 30ms delay, and a 1% probability of a packet being dropped)
```sudo tc qdisc del dev eth0 root``` (remove any quality of service settings).

The ```./benchmarks/run_benchmark.py``` script can be used as:

```
python3 ./benchmarks/run_benchmark.py 
    --benchmark_name <benchmark name>  
    --fio_file_path <path to the .fio file>
    --traffic_setting_unit <ms (milliseconds delay) or % (packet loss)> 
    --traffic_setting <loss or latency>
    --traffic_setting_values <space-separated list of integers>
```

to run a given FIO benchmark over TCP and QUIC, trying out values of the specified traffic setting values (using ```tc```),
and save the plot of the average IOPS (IO operations per second) and bandwidth (in KiB/s) over 10 runs (at a fixed traffic setting value)
in the ```./benchmarks/graphs/<benchmark name>``` directory. For example:

```
python3 ./benchmarks/run_benchmark.py 
    --benchmark_name delay_benchmark  
    --fio_file_path ./benchmarks/1_mb_read_tcp.fio 
    --traffic_setting_unit ms 
    --traffic_setting latency 
    --traffic_setting_values 0 1 2 5 10
```
 
### Packet loss

### Latency


