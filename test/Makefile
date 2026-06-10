CC = gcc
CFLAGS = -Wall -Wextra -I. -I./unity/src -g -DTEST_ENV
TARGET = test_watchface
SRCS = test_watchface.c pebble_mock.c unity/src/unity.c

all: $(TARGET)

$(TARGET): $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) -o $(TARGET)

clean:
	rm -f $(TARGET)

test: $(TARGET)
	./$(TARGET)
