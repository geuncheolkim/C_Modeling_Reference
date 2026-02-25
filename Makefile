CC = gcc
CFLAGS = -Wall -Wextra -O2
TARGET = image_io_test
SOURCES = image_io.c config.c

all: $(TARGET)

$(TARGET): $(SOURCES)
	$(CC) $(CFLAGS) -o $(TARGET) $(SOURCES)

clean:
	rm -f $(TARGET) $(TARGET).exe

test: $(TARGET)
	./$(TARGET) test_img/1080x2392/CT_W.bmp

test-gray: $(TARGET)
	./$(TARGET)

test-ppm: $(TARGET)
	./$(TARGET) --test-ppm

test-rgbg: $(TARGET)
	./$(TARGET) --verify-rgbg

.PHONY: all clean test
