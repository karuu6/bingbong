CC=g++
FLAGS=-std=c++11 -O3 -D_REENTRANT -Wall -Wno-sign-compare -fno-strict-aliasing -Wpointer-arith -Winline -Wno-deprecated -Wno-write-strings
INCLUDES=-Iinclude
LIBS=-Llib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib -lz

# g++ -O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing -I../include -o SampleOrder ../samples/SampleOrder.cpp -L../linux-gnu-4.18-x86_64/lib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -lpthread -lrt -ldl

bin/md: src/md.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/md src/md.cpp $(LIBS)

bin/latency: src/latency.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/latency src/latency.cpp $(LIBS)

md: bin/md
latency: bin/latency