CC=g++
INCLUDES=-Iinclude

FLAGS=-O3 -DLINUX -D_REENTRANT -Wall -Wno-sign-compare -Wno-write-strings -Wpointer-arith -Winline -Wno-deprecated -fno-strict-aliasing
LIBS=-Llib -lRApiPlus-optimize -lOmneStreamEngine-optimize -lOmneChannel-optimize -lOmneEngine-optimize -l_api-optimize -l_apipoll-stubs-optimize -l_kit-optimize -lssl -lcrypto -L/usr/lib64 -lz -lpthread -lrt -ldl -larmadillo

bin/q: src/q.cpp include/RApiPlus.h
	$(CC) $(FLAGS) $(INCLUDES) -o bin/q src/q.cpp $(LIBS)
bin/train: model/train.cpp
	$(CC) -o bin/train model/train.cpp -larmadillo
bin/test: model/test.cpp
	$(CC) -o bin/test model/test.cpp -larmadillo

q: bin/q
train: bin/train
test: bin/test