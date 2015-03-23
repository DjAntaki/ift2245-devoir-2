all: client server

CXXFLAGS += -g -lconfig++ -pthread

client: client.cpp

server: serverThreads.cpp server.cpp

clean:
	rm -f client server
