all: client server

CXXFLAGS += -g -lconfig++ -pthread

client: client.cpp

server: server.cpp

clean:
	rm -f client server
