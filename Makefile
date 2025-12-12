CC = gcc
CFLAGS = -O2 -Wall -Wextra `pkg-config --cflags gdk-pixbuf-2.0` -DNO_MAIN_SOLVER
LIBS = `pkg-config --libs gdk-pixbuf-2.0` -lm -lSDL2 -lSDL2_image

TARGET = selem
SRC = main.c \
      Preprocessing/preprocessing.c \
      RotationPNG/rotate_png.c \
      NewNeuralNetwork/new_neural_network.c \
      NewNeuralNetwork/bmp_converter.c \
      segmentation/segmentation.c \
      segmentation/image_segmentation.c \
      segmentation/char_segmentation.c \
      solver-cli/solver.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -rf $(TARGET) output/

