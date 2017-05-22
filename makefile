CC=g++
CFLAGS=-Wall -O3 -fopenmp -std=c++0x
LIB=-lboost_system -lboost_program_options -lboost_filesystem -lboost_date_time -lgomp

HEADER=src/genpacket.hpp src/queue.hpp src/packet.hpp
UTIL=src/io.hpp src/logging.hpp src/stringutil.hpp

all: bin/queuesim

bin/queuesim: bin/packet.o bin/genpacket.o bin/queue.o bin/queuesim.o
	$(CC) bin/packet.o bin/genpacket.o bin/queue.o bin/queuesim.o $(LIB) -o bin/queuesim

bin/queuesim.o: src/queuesim.cpp
	$(CC) $(CFLAGS) -c src/queuesim.cpp -o bin/queuesim.o

bin/packet.o: src/packet.cpp src/packet.hpp
	$(CC) $(CFLAGS) -c src/packet.cpp -o bin/packet.o

bin/queue.o: src/queue.cpp src/queue.hpp
	$(CC) $(CFLAGS) -c src/queue.cpp -o bin/queue.o

bin/genpacket.o: src/genpacket.cpp src/genpacket.hpp
	$(CC) $(CFLAGS) -c src/genpacket.cpp -o bin/genpacket.o

clean:
	rm -rf bin/*.o bin/queuesim
