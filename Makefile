serialization_files = ./src/serialization/mount/mount.pb-c.c \
	./src/serialization/nfs/nfs.pb-c.c \
	./src/serialization/rpc/rpc.pb-c.c \
	./src/serialization/google_protos/any.pb-c.c \
	./src/serialization/google_protos/empty.pb-c.c

rpc_program_common_server_files = ./src/common_rpc/server_common_rpc.c \
	./src/common_rpc/common_rpc.c
rpc_program_common_client_files = ./src/common_rpc/client_common_rpc.c \
	./src/common_rpc/common_rpc.c

# the Mount and Nfs clients that the REPL uses to do RPCs
repl_clients = ./src/nfs/clients/mount_client.c \
	./src/nfs/clients/nfs_client.c

# files used by Nfs+Mount server
mount_and_nfs_server_files = ./src/nfs/server/mount.c ./src/nfs/server/nfs.c \
	./src/nfs/server/mount_list.c \
	./src/nfs/server/inode_cache.c \
	./src/nfs/server/mount_messages.c \
	./src/nfs/server/nfs_messages.c

tests_files = ./tests/test_common.c

# implementations of file management functions
file_management_library = ./src/file_management/file_management.c

all: create_build_dir mount_and_nfs_server repl
create_build_dir:
	mkdir -p ./build

serialization_library: google_protos rpc_serialization mount_serialization nfs_serialization
google_protos: any empty
any: ./src/serialization/google_protos/any.proto
	protoc --c_out=. ./src/serialization/google_protos/any.proto
empty: ./src/serialization/google_protos/empty.proto
	protoc --c_out=. ./src/serialization/google_protos/empty.proto
rpc_serialization: ./src/serialization/rpc/rpc.proto
	protoc --c_out=. ./src/serialization/rpc/rpc.proto
mount_serialization: ./src/serialization/mount/mount.proto
	protoc --c_out=. ./src/serialization/mount/mount.proto
nfs_serialization: ./src/serialization/nfs/nfs.proto
	protoc --c_out=. ./src/serialization/nfs/nfs.proto

mount_and_nfs_server: ./src/nfs/server/server.c
# -I flag adds the project root dir to include paths (so that we can include libraries in our files as serialization/mount/mount.pb-c.h e.g.)
	gcc ./src/nfs/server/server.c \
		${serialization_files} ${rpc_program_common_server_files} ${mount_and_nfs_server_files} ${file_management_library} \
		-I . -o ./build/mount_and_nfs_server -l protobuf-c

repl: ./src/repl/repl.c
	gcc ./src/repl/repl.c ${serialization_files} ${rpc_program_common_client_files} ${repl_clients} -I . -o ./build/repl -l protobuf-c

test: ./tests/test_nfs.c
	gcc ./tests/test_nfs.c \
		${serialization_files} ${rpc_program_common_client_files} ${repl_clients} ${tests_files} \
		-I . -o ./build/test_nfs -l criterion -l protobuf-c

clean:
	rm -rf ./build/*

clean-all:
	rm -rf ./build/*
	rm -rf ./src/serialization/google_protos/any.pb-c.c
	rm -rf ./src/serialization/google_protos/any.pb-c.h
	rm -rf ./src/serialization/google_protos/empty.pb-c.h
	rm -rf ./src/serialization/google_protos/empty.pb-c.c
	rm -rf ./src/serialization/mount/mount.pb-c.h
	rm -rf ./src/serialization/mount/mount.pb-c.c
	rm -rf ./src/serialization/rpc/rpc.pb-c.h
	rm -rf ./src/serialization/rpc/rpc.pb-c.c