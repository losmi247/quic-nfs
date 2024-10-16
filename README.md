To generate Serialization and Deserialization code for your data.proto:
    protoc --c_out=. data.proto



So now we know how we're going to be serialising all the structures that need to be sent over the network.
Next: implement Mount RPC msg bodies using this. Send them from client and do them at server.