CC = gcc
CFLAGS = -O2 -Wall -Wextra `pkg-config --cflags gdk-pixbuf-2.0` -g -fsanitize=address
LIBS = `pkg-config --libs gdk-pixbuf-2.0` -lm -lSDL2 -lSDL2_image

TARGET = selem
SRC = main.c \
      Preprocessing/preprocessing.c \
      RotationPNG/rotate_png.c \
      NeuralNetwork/neural_network.c \
      NeuralNetwork/bmp_converter.c \
      segmentation/segmentation.c \
      segmentation/image_segmentation.c \
      segmentation/char_segmentation.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -rf $(TARGET) output/

