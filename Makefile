TQUIC_DIR = ./deps/tquic
TQUIC_LIB_DIR = $(TQUIC_DIR)/target/release

TRANSPORT_PROTOCOL_CFLAGS_TCP = -D TEST_TRANSPORT_PROTOCOL=TRANSPORT_PROTOCOL_TCP
TRANSPORT_PROTOCOL_CFLAGS_QUIC = -D TEST_TRANSPORT_PROTOCOL=TRANSPORT_PROTOCOL_QUIC
# -I flag adds the project root dir to include paths (so that we can include libraries in our files as serialization/mount/mount.pb-c.h e.g.)
CFLAGS = -I . -I $(TQUIC_DIR)/include -I $(TQUIC_DIR)/deps/boringssl/src/include -I/usr/include/fuse3 -pthread -lfuse3
SANITIZER_FLAGS = -fsanitize=address -fsanitize=undefined -g
LIBS = $(TQUIC_LIB_DIR)/libtquic.a -l ev -l dl -l m -l protobuf-c
DEBUG_FLAGS = ${SANITIZER_FLAGS}

ERROR_HANDLING_SRCS = ./src/error_handling/error_handling.c
PARSING_SRCS = ./src/parsing/parsing.c
PATH_BUILDING_SRCS = ./src/path_building/path_building.c
AUTHENTICATION_SRCS = ./src/authentication/authentication.c
COMMON_PERMISSIONS_SRCS = ./src/common_permissions/common_permissions.c
MESSAGE_VALIDATION_SRCS = ./src/message_validation/message_validation.c

FILEHANDLE_MANAGEMENT_SRCS = ./src/filehandle_management/filehandle_management.c
FILESYSTEM_DAG_SRCS = ./src/repl/filesystem_dag/filesystem_dag.c
SOFT_LINKS_SRCS = ./src/repl/soft_links/soft_links.c

SERIALIZATION_SRCS = ./src/serialization/mount/mount.pb-c.c \
	./src/serialization/nfs/nfs.pb-c.c \
	./src/serialization/nfs_fh/nfs_fh.pb-c.c \
	./src/serialization/rpc/rpc.pb-c.c \
	./src/serialization/google_protos/any.pb-c.c \
	./src/serialization/google_protos/empty.pb-c.c

RPC_PROGRAM_COMMON_SERVER_SRCS = ./src/common_rpc/server_common_rpc.c \
	./src/common_rpc/common_rpc.c
RPC_PROGRAM_COMMON_CLIENT_SRCS = ./src/common_rpc/client_common_rpc.c \
	./src/common_rpc/common_rpc.c \
	./src/common_rpc/rpc_connection_context.c

TCP_RPC_PROGRAM_SERVER_SRCS = ./src/transport/tcp/tcp_record_marking.c \
	./src/transport/tcp/tcp_rpc_server.c
TCP_RPC_PROGRAM_CLIENT_SRCS = ./src/transport/tcp/tcp_record_marking.c \
	./src/transport/tcp/tcp_rpc_client.c

QUIC_RPC_PROGRAM_SERVER_SRCS =  ./src/transport/quic/quic_record_marking.c \
	./src/transport/quic/quic_rpc_server.c
QUIC_RPC_PROGRAM_CLIENT_SRCS = ./src/transport/quic/quic_record_marking.c \
	./src/transport/quic/quic_rpc_client.c

CLIENTS_SRCS = ./src/nfs/clients/mount_client.c ./src/nfs/clients/nfs_client.c

# files used by Nfs+Mount server
COMMON_MOUNT_AND_NFS_SERVER_SRCS = ./src/nfs/server/mount.c ./src/nfs/server/procedures/mntproc_*.c \
	./src/nfs/server/nfs.c ./src/nfs/server/procedures/nfsproc_*.c \
	./src/nfs/server/mount_list.c \
	./src/nfs/server/inode_cache.c \
	./src/nfs/server/file_management.c \
	./src/nfs/server/directory_reading.c \
	./src/nfs/server/mount_messages.c \
	./src/nfs/server/nfs_messages.c \
	./src/nfs/server/nfs_permissions.c \
	${SERIALIZATION_SRCS} ${PARSING_SRCS} ${ERROR_HANDLING_SRCS} ${PATH_BUILDING_SRCS} ${AUTHENTICATION_SRCS} ${COMMON_PERMISSIONS_SRCS} ${FILEHANDLE_MANAGEMENT_SRCS} ${RPC_PROGRAM_COMMON_SERVER_SRCS}
MOUNT_AND_NFS_SERVER_SRCS = ${COMMON_MOUNT_AND_NFS_SERVER_SRCS} ${TCP_RPC_PROGRAM_SERVER_SRCS} ${QUIC_RPC_PROGRAM_SERVER_SRCS}

# files used by the Tests
COMMON_TESTS_SRCS = ./tests/procedures/test_*.c \
	./tests/test_common.c ./tests/validation/common_validation.c ./tests/validation/procedure_validation.c \
	${CLIENTS_SRCS} ${SERIALIZATION_SRCS} ${PARSING_SRCS} ${ERROR_HANDLING_SRCS} ${FILEHANDLE_MANAGEMENT_SRCS} ${AUTHENTICATION_SRCS} ${RPC_PROGRAM_COMMON_CLIENT_SRCS}
TESTS_SRCS = ${COMMON_TESTS_SRCS} ${TCP_RPC_PROGRAM_CLIENT_SRCS} ${QUIC_RPC_PROGRAM_CLIENT_SRCS}

# files used by the Repl
COMMON_REPL_SRCS = ./src/repl/handlers/*.c \
	${CLIENTS_SRCS} ${SERIALIZATION_SRCS} ${PARSING_SRCS} ${ERROR_HANDLING_SRCS} ${PATH_BUILDING_SRCS} ${FILESYSTEM_DAG_SRCS} ${AUTHENTICATION_SRCS} ${COMMON_PERMISSIONS_SRCS} ${FILEHANDLE_MANAGEMENT_SRCS} ${SOFT_LINKS_SRCS} ${MESSAGE_VALIDATION_SRCS} ${RPC_PROGRAM_COMMON_CLIENT_SRCS}
REPL_SRCS = ${COMMON_REPL_SRCS} ${TCP_RPC_PROGRAM_CLIENT_SRCS} ${QUIC_RPC_PROGRAM_CLIENT_SRCS}

# files used by the FUSE file system
COMMON_FUSE_FS_SRCS = ./src/fuse/handlers/handlers.c ./src/fuse/path_resolution.c ./src/fuse/handlers/fuse_*.c \
	${CLIENTS_SRCS} ${SERIALIZATION_SRCS} ${PARSING_SRCS} ${ERROR_HANDLING_SRCS} ${AUTHENTICATION_SRCS} ${COMMON_PERMISSIONS_SRCS} ${FILEHANDLE_MANAGEMENT_SRCS} ${MESSAGE_VALIDATION_SRCS} ${RPC_PROGRAM_COMMON_CLIENT_SRCS}
FUSE_FS_SRCS = ${COMMON_FUSE_FS_SRCS} ${TCP_RPC_PROGRAM_CLIENT_SRCS} ${QUIC_RPC_PROGRAM_CLIENT_SRCS}

all: create-build-dir mount-and-nfs-server repl fuse-fs
all-debug: create-build-dir mount-and-nfs-server-debug repl-debug fuse-fs-debug

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

# server, test, and REPL targets
mount-and-nfs-server: ./src/nfs/server/server.c create-build-dir ${MOUNT_AND_NFS_SERVER_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
# $< is the first prerequisite (./src/nfs/server/server.c), $@ is the name of the rule
	gcc $< ${MOUNT_AND_NFS_SERVER_SRCS} ${CFLAGS} -o ./build/mount_and_nfs_server ${LIBS}

test: create-build-dir test-tcp test-quic
test-tcp: create-build-dir ${TESTS_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc ${TESTS_SRCS} ${CFLAGS} ${TRANSPORT_PROTOCOL_CFLAGS_TCP} -o ./build/test_tcp ${LIBS} -l criterion
test-quic: create-build-dir ${TESTS_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc ${TESTS_SRCS} ${CFLAGS} ${TRANSPORT_PROTOCOL_CFLAGS_QUIC} -o ./build/test_quic ${LIBS} -l criterion

repl: ./src/repl/repl.c create-build-dir ${REPL_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc $< ${REPL_SRCS} ${CFLAGS} -o ./build/repl ${LIBS}

fuse-fs: ./src/fuse/nfs_fuse.c create-build-dir ${FUSE_FS_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc $< ${FUSE_FS_SRCS} ${CFLAGS} -D_FILE_OFFSET_BITS=64 -o ./build/fuse_fs ${LIBS} -l fuse3

# debugging versions of all targets
mount-and-nfs-server-debug: ./src/nfs/server/server.c create-build-dir ${MOUNT_AND_NFS_SERVER_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc $< ${MOUNT_AND_NFS_SERVER_SRCS} ${CFLAGS} -o ./build/mount_and_nfs_server ${DEBUG_FLAGS} ${LIBS}

test-debug: create-build-dir test-tcp-debug test-quic-debug
test-tcp-debug: create-build-dir ${TESTS_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc ${TESTS_SRCS} ${CFLAGS} ${TRANSPORT_PROTOCOL_CFLAGS_TCP} -o ./build/test_tcp ${DEBUG_FLAGS} ${LIBS} -l criterion
test-quic-debug: create-build-dir ${TESTS_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc ${TESTS_SRCS} ${CFLAGS} ${TRANSPORT_PROTOCOL_CFLAGS_QUIC} -o ./build/test_quic ${DEBUG_FLAGS} ${LIBS} -l criterion

repl-debug: ./src/repl/repl.c create-build-dir ${REPL_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc $< ${REPL_SRCS} ${CFLAGS} -o ./build/repl ${DEBUG_FLAGS} ${LIBS}

fuse-fs-debug: ./src/fuse/nfs_fuse.c create-build-dir ${FUSE_FS_SRCS} $(TQUIC_LIB_DIR)/libtquic.a
	gcc $< ${FUSE_FS_SRCS} ${CFLAGS} -D_FILE_OFFSET_BITS=64 -o ./build/fuse_fs ${DEBUG_FLAGS} ${LIBS} -l fuse3

$(TQUIC_LIB_DIR)/libtquic.a:
	git submodule update --init --recursive && \
	cd $(TQUIC_DIR) && cargo build --release -F ffi

create-build-dir:
	mkdir -p ./build

# clean
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