all: client server

CXXFLAGS += -lconfig++ -pthread

client: clientThread.cpp client.cpp

server: serverThreads.cpp server.cpp
