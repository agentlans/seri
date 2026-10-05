CC = gcc
CFLAGS = -Wall -Wextra -O3 -std=c99 -fPIC
TARGET_STATIC = libseri.a
TARGET_SHARED = libseri.so
TARGET_TEST = test_seri

all: $(TARGET_STATIC) $(TARGET_SHARED) $(TARGET_TEST)

$(TARGET_STATIC): seri.o
	ar rcs $@ $^

$(TARGET_SHARED): seri.o
	$(CC) -shared -o $@ $^

seri.o: seri.c seri.h
	$(CC) $(CFLAGS) -c seri.c

test_seri.o: test_seri.c seri.h
	$(CC) $(CFLAGS) -c test_seri.c

$(TARGET_TEST): test_seri.o $(TARGET_STATIC)
	$(CC) $(CFLAGS) -o $@ $^ -lm

test: $(TARGET_TEST)
	./$(TARGET_TEST)

clean:
	rm -f *.o $(TARGET_STATIC) $(TARGET_SHARED) $(TARGET_TEST)

.PHONY: all test clean
