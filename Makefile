serialization_files = ./src/serialization/mount/mount.pb-c.c \
	./src/serialization/rpc/rpc.pb-c.c \
	./src/serialization/google_protos/any.pb-c.c \
	./src/serialization/google_protos/empty.pb-c.c

rpc_program_server_files = ./src/common_rpc/server_common_rpc.c \
	./src/common_rpc/common_rpc.c

rpc_program_client_files = ./src/common_rpc/client_common_rpc.c \
	./src/common_rpc/common_rpc.c

# the Mount and Nfs clients that the REPL uses to do RPCs
repl_clients = ./src/rpc_programs/mount/client/mount_client.c

# files used by mount server
mount_server_files = ./src/rpc_programs/mount/server/mount_list.c

test_framework_files = ./tests/unity/unity.c

all: mount_server repl

serialization_library: google_protos rpc mount
google_protos: any empty
any: ./src/serialization/google_protos/any.proto
	protoc --c_out=. ./src/serialization/google_protos/any.proto
empty: ./src/serialization/google_protos/empty.proto
	protoc --c_out=. ./src/serialization/google_protos/empty.proto
rpc: ./src/serialization/rpc/rpc.proto
	protoc --c_out=. ./src/serialization/rpc/rpc.proto
mount: ./src/serialization/mount/mount.proto
	protoc --c_out=. ./src/serialization/mount/mount.proto

mount_server: ./src/rpc_programs/mount/server/mount.c
# -I flag adds the project root dir to include paths (so that we can include in our files as serialization/mount/mount.pb-c.h e.g.)
	gcc ./src/rpc_programs/mount/server/mount.c ${serialization_files} ${rpc_program_server_files} ${mount_server_files} -I . -o ./build/mount_server -l protobuf-c

repl: ./src/repl/repl.c
	gcc ./src/repl/repl.c ${serialization_files} ${rpc_program_client_files} ${repl_clients} -I . -o ./build/repl -l protobuf-c


tests: ./tests/mount/test_mount.c
	gcc ./tests/mount/test_mount.c ${serialization_files} ${rpc_program_client_files} ${repl_clients} -I . -o ./build/test_mount -l criterion -l protobuf-c

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