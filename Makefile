CC=g++
FLAGS=-std=c++11 -O3 -D_REENTRANT -Wall -Wno-sign-compare -fno-strict-aliasing -Wpointer-arith -Winline -Wno-deprecated -Wno-write-strings
INCLUDES=-Iinclude
LIBS=-Llib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib -lz

bin/md: src/md.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/md src/md.cpp $(LIBS)

bin/luper: src/luper.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/luper src/luper.cpp $(LIBS)

md: bin/md
luper: bin/luper
