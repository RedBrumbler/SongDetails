# make sure protoc is on your PATH (v3.21.11 was used)
& protoc --cpp_out=./include --proto_path=./include ./include/SongProto.proto
move-item ./include/SongProto.pb.cc ./src/SongProto.pb.cc -Force