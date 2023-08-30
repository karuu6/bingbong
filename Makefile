CC=g++
FLAGS=-std=c++11 -O3 -D_REENTRANT -Wall -Wno-sign-compare -fno-strict-aliasing -Wpointer-arith -Winline -Wno-deprecated -Wno-write-strings
INCLUDES=-Iinclude
LIBS=-Llib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib -lz

LFLAGS=-O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing
LLIBS=-Llib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -lpthread -lrt -ldl

bin/md: src/md.cpp include/RApiPlus.h
	$(CC) $(LFLAGS) $(INCLUDES) -o bin/md src/md.cpp $(LLIBS)

bin/latency: src/latency.cpp include/RApiPlus.h
	$(CC) $(LFLAGS) $(INCLUDES) -o bin/latency src/latency.cpp $(LLIBS)

bin/gg: src/gg.cpp include/RApiPlus.h
	$(CC) $(LFLAGS) $(INCLUDES) -o bin/gg src/gg.cpp $(LLIBS)

bin/q: src/q.cpp include/RApiPlus.h
	$(CC) $(LFLAGS) $(INCLUDES) -o bin/q src/q.cpp $(LLIBS)


q: bin/q
gg: bin/gg
md: bin/md
latency: bin/latency