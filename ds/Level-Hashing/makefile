CXX_FLAG=-I../shm/include

clevel: ycsb.o level_hashing.o hash.o
	g++ $(CXX_FLAG) -o clevel ycsb.o level_hashing.o hash.o -lm -lpthread

ycsb.o: ycsb.cc level_hashing.h spinlock.h
	g++ $(CXX_FLAG) -c ycsb.cc -lm

level_hashing.o : level_hashing.cc level_hashing.h spinlock.h
	g++ $(CXX_FLAG) -c level_hashing.cc -lm

hash.o : hash.cc hash.h
	g++ $(CXX_FLAG) -c hash.cc -lm

clean:
	rm *.o clevel
