serialization_files = ./serialization/mount/mount.pb-c.c ./serialization/rpc/rpc.pb-c.c ./serialization/google_protos/any.pb-c.c ./serialization/google_protos/empty.pb-c.c

rpc_program_server_files = ./common_rpc/server_common_rpc.c ./common_rpc/common_rpc.c

rpc_program_client_files = ./common_rpc/client_common_rpc.c ./common_rpc/common_rpc.c

# the Mount and Nfs clients that the REPL uses to do RPCs
repl_clients = ./rpc_programs/mount/mount_client.c

all: serialization_library mount_server repl

all_but_serialization: mount_server repl

serialization_library: google_protos rpc mount
google_protos: any empty
any: ./serialization/google_protos/any.proto
	protoc --c_out=. ./serialization/google_protos/any.proto
empty: ./serialization/google_protos/empty.proto
	protoc --c_out=. ./serialization/google_protos/empty.proto
rpc: ./serialization/rpc/rpc.proto
	protoc --c_out=. ./serialization/rpc/rpc.proto
mount: ./serialization/mount/mount.proto
	protoc --c_out=. ./serialization/mount/mount.proto

mount_server: ./rpc_programs/mount/mount.c
# -I flag adds the project root dir to include paths (so that we can include in our files as serialization/mount/mount.pb-c.h e.g.)
	gcc ./rpc_programs/mount/mount.c ${serialization_files} ${rpc_program_server_files} -I . -o ./build/mount_server -l protobuf-c

repl: ./repl/repl.c
	gcc ./repl/repl.c ${serialization_files} ${rpc_program_client_files} ${repl_clients} -I . -o ./build/repl -l protobuf-c

clean:
	rm -rf ./build/*

clean-all:
	rm -rf ./build/*
	rm -rf ./serialization/google_protos/any.pb-c.c
	rm -rf ./serialization/google_protos/any.pb-c.h
	rm -rf ./serialization/google_protos/empty.pb-c.h
	rm -rf ./serialization/google_protos/empty.pb-c.c
	rm -rf ./serialization/mount/mount.pb-c.h
	rm -rf ./serialization/mount/mount.pb-c.c
	rm -rf ./serialization/rpc/rpc.pb-c.h
	rm -rf ./serialization/rpc/rpc.pb-c.c