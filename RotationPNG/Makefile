CC = gcc
CFLAGS = -O2 -Wall -Wextra `pkg-config --cflags gdk-pixbuf-2.0`
LIBS = `pkg-config --libs gdk-pixbuf-2.0` -lm
TARGET = rotate_png
SRC = rotate_png.c

all: $(TARGET)
$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)
clean:
	rm -f $(TARGET)
run:
	./$(TARGET) input.png 45 output.bmp

