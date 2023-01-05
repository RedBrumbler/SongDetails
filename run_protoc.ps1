& protoc --cpp_out=./include --proto_path=./include ./include/SongProto.proto
move-item ./include/SongProto.pb.cc ./src/SongProto.pb.cc -Force