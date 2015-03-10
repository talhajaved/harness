BINARIES := cachetest 
all: $(BINARIES)

CFLAGS := $(CFLAGS) -g -Wall -Werror -D_POSIX_THREAD_SEMANTICS
LDFLAGS := $(CFLAGS) -lpthread -lrt -lm

CTHREADLIBS := sthread.o

clean:
	rm -f *.o $(BINARIES)

tags:
	etags *.h *.c *.cc

%.o: %.cc
	g++ -c $(CFLAGS) $< -o $@

%.o: %.c
	gcc -c $(CFLAGS) $< -o $@

cachetest: cachetest.o $(CTHREADLIBS)
	gcc $(LDFLAGS) $^ -o $@

