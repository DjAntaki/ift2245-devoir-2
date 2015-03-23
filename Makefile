all: client server

CXXFLAGS += -g -lconfig++ -pthread

client: client.cpp

server: server.cpp

rapport.pdf: rapport.md
	pandoc --latex-engine=xelatex $< -o $@

clean:
	rm -f client server
