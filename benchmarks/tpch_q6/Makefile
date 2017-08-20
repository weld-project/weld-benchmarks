UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC=gcc-6 -fopenmp
endif

ifeq ($(UNAME_S),Linux)
    CC=gcc -fopenmp
endif

CPPFLAGS=-O3 -march=native -std=c++11 -I$(WELD_HOME)/c
LDFLAGS=-L$(WELD_HOME)/target/release
LIBS=-lweld

.PHONY: all clean

all:
	${CC} ${LDFLAGS} ${LIBS} tpch_q6.cpp ${CPPFLAGS} -o bench

clean:
	rm -f bench
