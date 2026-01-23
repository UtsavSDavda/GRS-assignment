CC = gcc
# -std=gnu99: Enables C99 + POSIX extensions (fixes implicit declaration of fileno)
# -Wno-unused-parameter: Suppresses harmless warning about unused 'arg' in thread wrapper
CFLAGS = -Wall -Wextra -O2 -std=gnu99 -Wno-unused-parameter
LDFLAGS = -pthread

all: progA progB

progA: A.c
	$(CC) $(CFLAGS) -o progA A.c

progB: B.c
	$(CC) $(CFLAGS) -o progB B.c $(LDFLAGS)

clean:
	rm -f progA progB *.o io_*.dat results.csv *.png temp_*.log

run: all
	chmod +x MT25088_bench.sh
	./MT25088_bench.sh