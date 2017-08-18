UNAME_S := $(shell uname -s)

CPPFLAGS=-O3 -march=native -std=c++11 -I$(WELD_HOME)/c
LDFLAGS=-L$(WELD_HOME)/target/release
LIBS=-lweld

.PHONY: all clean

all:
	clang++ ${LDFLAGS} ${LIBS} tpch_q1.cpp ${CPPFLAGS} -o bench

clean:
	rm -f bench
