serialization_files = ./serialization/mount/mount.pb-c.c ./serialization/rpc/rpc.pb-c.c ./serialization/google_protos/any.pb-c.c ./serialization/google_protos/empty.pb-c.c
rpc_programs_files = ./rpc_programs/mount/mount.c

all: serialization_library rpc_server rpc_client

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

rpc_server: ./rpc_server.c
	gcc rpc_server.c ${serialization_files} ${rpc_programs_files} -o ./build/rpc_server -l protobuf-c

rpc_client: ./rpc_client.c
	gcc rpc_client.c ${serialization_files} ${rpc_programs_files} -o ./build/rpc_client -l protobuf-c

clean:
	rm -rf ./build/*