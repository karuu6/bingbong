CC=g++
FLAGS=-O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing
INCLUDES=-Iinclude
LIBS=-Llib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -lpthread -lrt -ldl

bin/md: src/md.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/md src/md.cpp $(LIBS)

bin/gg: src/gg.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/gg src/gg.cpp $(LIBS)

bin/latency: src/latency.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/latency src/latency.cpp $(LIBS)

md: bin/md
gg: bin/gg
latency: bin/latency
