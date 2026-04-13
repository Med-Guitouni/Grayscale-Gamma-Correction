CC =  gcc
CFLAGS = -O3 -std=gnu11 -g -msse4.1 -pthread -Wall -Wextra -pedantic
LDFLAGS = -lrt -lm

SRC_MAIN = main.c gammaCorr.c
SRC_TESTING = testing.c gammaCorr.c
TARGET_MAIN = main
TARGET_TESTING = testing

.PHONY: all install clean

all: $(TARGET_MAIN) $(TARGET_TESTING)

$(TARGET_MAIN): $(SRC_MAIN)
	$(CC) $(CFLAGS) -o $@ $^

$(TARGET_TESTING): $(SRC_TESTING)
	$(CC) $(CFLAGS) -o $@ $^

install: $(TARGET_MAIN) $(TARGET_TESTING)
	cp $(TARGET_MAIN) /usr/local/bin/
	cp $(TARGET_TESTING) /usr/local/bin/

clean:
	rm -f $(TARGET_MAIN) $(TARGET_TESTING) *.ppm


