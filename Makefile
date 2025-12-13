CC = gcc
CFLAGS = -O2 -Wall -Wextra `pkg-config --cflags gdk-pixbuf-2.0` -DNO_MAIN_SOLVER
LIBS = `pkg-config --libs gdk-pixbuf-2.0` -lm -lSDL2 -lSDL2_image

TARGET_CLI = ocr
SRC_CLI = main.c \
      Preprocessing/preprocessing.c \
      RotationPNG/rotate_png.c \
      NewNeuralNetwork/new_neural_network.c \
      NewNeuralNetwork/bmp_converter.c \
      segmentation/segmentation.c \
      segmentation/image_segmentation.c \
      segmentation/char_segmentation.c \
      solver-cli/solver.c

.PHONY: all cli gui clean

all: cli gui

cli: $(TARGET_CLI)

$(TARGET_CLI): $(SRC_CLI)
	$(CC) $(CFLAGS) -o $(TARGET_CLI) $(SRC_CLI) $(LIBS)

gui:
	$(MAKE) -C OcrGui
	mv OcrGui/ocr_gui ./

clean:
	rm -rf $(TARGET_CLI) ocr_gui output/
	$(MAKE) -C OcrGui clean
