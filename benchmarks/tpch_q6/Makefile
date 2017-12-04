UNAME_S := $(shell uname -s)
ifeq ($(UNAME_S),Darwin)
    CC=gcc-6 -fopenmp
endif

ifeq ($(UNAME_S),Linux)
    CC=g++ -fopenmp
endif

CPPFLAGS=-O0 -DDEBUG -ggdb -march=native -std=c++11 -I$(WELD_HOME)/c
LDFLAGS=-L$(WELD_HOME)/target/release
LIBS=-lweld

.PHONY: all clean

all:
	${CC} ${LDFLAGS} tpch_q6.cpp ${CPPFLAGS} -o bench ${LIBS}

gen:
	${CC} ${LDFLAGS} generated.cpp ${CPPFLAGS} -o gen ${LIBS}

clean:
	rm -f bench
