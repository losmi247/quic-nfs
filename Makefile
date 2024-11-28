# -I flag adds the project root dir to include paths (so that we can include libraries in our files as serialization/mount/mount.pb-c.h e.g.)
CFLAGS = -I .
SANITIZER_FLAGS = -fsanitize=address -fsanitize=undefined -g
LDFLAGS = -l protobuf-c
DEBUG_LDFLAGS = ${SANITIZER_FLAGS} ${LDFLAGS}

ERROR_HANDLING_SRCS = ./src/error_handling/error_handling.c

SERIALIZATION_SRCS = ./src/serialization/mount/mount.pb-c.c \
	./src/serialization/nfs/nfs.pb-c.c \
	./src/serialization/nfs_fh/nfs_fh.pb-c.c \
	./src/serialization/rpc/rpc.pb-c.c \
	./src/serialization/google_protos/any.pb-c.c \
	./src/serialization/google_protos/empty.pb-c.c

RPC_PROGRAM_COMMON_SERVER_SRCS = ./src/common_rpc/server_common_rpc.c \
	./src/common_rpc/common_rpc.c
RPC_PROGRAM_COMMON_CLIENT_SRCS = ./src/common_rpc/client_common_rpc.c \
	./src/common_rpc/common_rpc.c

CLIENTS_SRCS = ./src/nfs/clients/mount_client.c ./src/nfs/clients/nfs_client.c

# files used by Nfs+Mount server
MOUNT_AND_NFS_SERVER_SRCS = ./src/nfs/server/mount.c ./src/nfs/server/nfs.c \
	./src/nfs/server/mount_list.c \
	./src/nfs/server/inode_cache.c \
	./src/nfs/server/file_management.c \
	./src/nfs/server/mount_messages.c \
	./src/nfs/server/nfs_messages.c \
	${SERIALIZATION_SRCS} ${RPC_PROGRAM_COMMON_SERVER_SRCS} ${ERROR_HANDLING_SRCS}

# files used by the Tests
TESTS_SRCS = ./tests/test_common.c \
	${CLIENTS_SRCS} ${SERIALIZATION_SRCS} ${RPC_PROGRAM_COMMON_CLIENT_SRCS} ${ERROR_HANDLING_SRCS}

# files used by the Repl
REPL_SRCS = ${CLIENTS_SRCS} ${SERIALIZATION_SRCS} ${RPC_PROGRAM_COMMON_CLIENT_SRCS} ${ERROR_HANDLING_SRCS}

all: create-build-dir mount-and-nfs-server repl
all-debug: create-build-dir mount-and-nfs-server-debug repl-debug
create-build-dir:
	mkdir -p ./build

serialization-library: google-protos rpc-serialization nfs-filehandle-serialization mount-serialization nfs-serialization
google-protos: any empty
any: ./src/serialization/google_protos/any.proto
	protoc --c_out=. ./src/serialization/google_protos/any.proto
empty: ./src/serialization/google_protos/empty.proto
	protoc --c_out=. ./src/serialization/google_protos/empty.proto
rpc-serialization: ./src/serialization/rpc/rpc.proto
	protoc --c_out=. ./src/serialization/rpc/rpc.proto
nfs-filehandle-serialization: ./src/serialization/nfs_fh/nfs_fh.proto
	protoc --c_out=. ./src/serialization/nfs_fh/nfs_fh.proto
mount-serialization: ./src/serialization/mount/mount.proto
	protoc --c_out=. ./src/serialization/mount/mount.proto
nfs-serialization: ./src/serialization/nfs/nfs.proto
	protoc --c_out=. ./src/serialization/nfs/nfs.proto

mount-and-nfs-server: ./src/nfs/server/server.c ${MOUNT_AND_NFS_SERVER_SRCS}
# $< is the first prerequisite (./src/nfs/server/server.c), $@ is the name of the rule
	gcc $< ${MOUNT_AND_NFS_SERVER_SRCS} ${CFLAGS} -o ./build/mount_and_nfs_server ${LDFLAGS}

test: ./tests/test_nfs.c ${TESTS_SRCS}
	gcc $< ${TESTS_SRCS} ${CFLAGS} -o ./build/$@ ${LDFLAGS} -l criterion

repl: ./src/repl/repl.c ${REPL_SRCS}
	gcc $< ${REPL_SRCS} ${CFLAGS} -o ./build/$@ ${LDFLAGS}

mount-and-nfs-server-debug: ./src/nfs/server/server.c ${MOUNT_AND_NFS_SERVER_SRCS}
	gcc $< ${MOUNT_AND_NFS_SERVER_SRCS} ${CFLAGS} -o ./build/mount_and_nfs_server ${DEBUG_LDFLAGS}

test-debug: ./tests/test_nfs.c ${TESTS_SRCS}
	gcc $< ${TESTS_SRCS} ${CFLAGS} -o ./build/test ${DEBUG_LDFLAGS} -l criterion

repl-debug: ./src/repl/repl.c ${REPL_SRCS}
	gcc $< ${REPL_SRCS} ${CFLAGS} -o ./build/repl ${DEBUG_LDFLAGS}

clean:
	rm -rf ./build/*
clean-all:
	rm -rf ./build/*
	rm -rf ./src/serialization/google_protos/any.pb-c.c
	rm -rf ./src/serialization/google_protos/any.pb-c.h
	rm -rf ./src/serialization/google_protos/empty.pb-c.h
	rm -rf ./src/serialization/google_protos/empty.pb-c.c
	rm -rf ./src/serialization/rpc/rpc.pb-c.h
	rm -rf ./src/serialization/rpc/rpc.pb-c.c
	rm -rf ./src/serialization/mount/mount.pb-c.h
	rm -rf ./src/serialization/mount/mount.pb-c.c
	rm -rf ./src/serialization/nfs/nfs.pb-c.h
	rm -rf ./src/serialization/nfs/nfs.pb-c.c
	rm -rf ./src/serialization/nfs_fh/nfs_fh.pb-c.h
	rm -rf ./src/serialization/nfs_fh/nfs_fh.pb-c.c