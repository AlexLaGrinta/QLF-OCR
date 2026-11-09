# QLF — Word Search Solver

An OCR-based word search solver written in C for the EPITA OCR project.
Given a photo or scan of a word search grid, the program detects the letters,
recognizes them with a neural network, then finds and circles the words from
the word list directly on the image.

## Table of contents

- [How it works](#how-it-works)
- [Dependencies](#dependencies)
- [Building](#building)
- [Usage](#usage)
- [Project structure](#project-structure)
- [Standalone modules](#standalone-modules)
- [Neural network](#neural-network)
- [Reports](#reports)
- [Authors](#authors)

## How it works

The `ocr` binary runs the following steps:

1. **Pre-filtering**: grayscale conversion, then an optional median filter to reduce noise.
2. **Automatic rotation**: detects the skew angle and straightens the image (bilinear interpolation).
3. **Binarization**: automatic thresholding using Otsu's method.
4. **Segmentation**: detects the grid and the word list, then extracts each letter.
5. **Recognition**: each letter (28×28 px) is fed to the neural network, which predicts a character from `A` to `Z`.
6. **Solving**: searches for each word in the grid in all 8 directions, tolerating some recognition errors.
7. **Rendering**: found words are circled in color on the straightened image.

## Dependencies

- `gcc` and `make`
- `pkg-config`
- SDL2 and SDL2_image
- gdk-pixbuf 2.0
- GTK+ 3 (graphical interface only)
- Python 3, Pillow and NumPy (dataset generation only)

On Arch Linux:

```bash
sudo pacman -S gcc make pkgconf sdl2 sdl2_image gdk-pixbuf2 gtk3
```

On Debian / Ubuntu:

```bash
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-image-dev libgdk-pixbuf-2.0-dev libgtk-3-dev
```

## Building

All commands are run from the repository root.

| Command      | Result                                          |
| ------------ | ----------------------------------------------- |
| `make all`   | Builds both the CLI and the graphical interface |
| `make cli`   | Builds only the `ocr` binary                    |
| `make gui`   | Builds the `ocr_gui` graphical interface        |
| `make clean` | Removes the binaries and the `output/` folder   |

## Usage

> The binaries must be run **from the repository root**: they use the `temp/`
> folder and the network weights file, and they write to `output/`.

### Command line

```bash
./ocr <image> <median_filter>
```

- `<image>`: path to the grid image (PNG, JPG, BMP).
- `<median_filter>`: `1` to apply the median filter, `0` otherwise.

Example with a provided image:

```bash
./ocr temp/level_1_image_1.png 0
```

The program prints the recognized grid and words and the coordinates of each
word found, then saves the result to `output/solved.bmp`.

### Graphical interface

```bash
./ocr_gui
```

The interface lets you:

- open an image;
- enable or disable the median filter;
- run the OCR and display the solved grid;
- display the segmentation visualization;
- save the result.

The interface calls `./ocr` internally, so both must be built (`make all`).

### Sample images

Test images are available in `temp/`:

- `level_1_image_1.png`
- `level_2_image_1.png`
- `level_2_image_2.png`
- `level_3_image_1.png`

## Project structure

```
.
├── main.c               # Entry point: full OCR pipeline
├── Makefile             # CLI + GUI build
├── Preprocessing/       # Grayscale, median filter, binarization (Otsu)
├── RotationPNG/         # Skew detection and automatic rotation
├── segmentation/        # Grid / word list detection and letter extraction
├── NewNeuralNetwork/    # Neural network used by the OCR + trained weights
├── NeuralNetwork/       # First version of the neural network
├── XNORNetwork/         # Proof of concept: network learning the XNOR function
├── solver-cli/          # Word search solver (8-direction search)
├── OcrGui/              # GTK graphical interface
├── dataset/             # Fonts and Python dataset generation script
├── temp/                # Sample images and intermediate files
├── rapport_1.pdf        # First defense report
├── rapport_2.pdf        # Final defense report
└── AUTHORS
```

## Standalone modules

Each module has its own `Makefile` and can be built and tested on its own
from its folder.

| Module              | Binary         | Usage                                                 |
| ------------------- | -------------- | ----------------------------------------------------- |
| `Preprocessing/`    | `ocr`          | `./ocr <image>`                                       |
| `RotationPNG/`      | `rotate_png`   | `./rotate_png <input.png> <angle\|auto> <output.bmp>` |
| `segmentation/`     | `segmentation` | `./segmentation <image.bmp>`                          |
| `solver-cli/`       | `solver`       | `./solver <grid_file> <word> [max_mismatches]`        |
| `NewNeuralNetwork/` | `test_nn`      | `./test_nn` (training and evaluation)                 |
| `XNORNetwork/`      | `xor`          | `./xor`                                               |

Example with the solver alone:

```bash
cd solver-cli
make
./solver grid HORIZONTAL
```

The grid file contains one line of uppercase letters per row.
The solver prints the start and end coordinates of the word: `(x1,y1)(x2,y2)`.

## Neural network

- **Architecture**: multilayer perceptron 784 → 128 → 128 → 26
- **Input**: binarized 28×28 letter image
- **Output**: one class per uppercase letter (`A` to `Z`)
- **Training**: backpropagation with momentum and learning rate decay
- **Weights**: `NewNeuralNetwork/new_model_weights.bin`

### Generating the dataset

The `dataset/make_dataset.py` script generates letter images from the fonts in
`dataset/fonts/`, with slight variations in size and rotation:

```bash
cd dataset
pip install pillow numpy
python3 make_dataset.py
```

Images are generated in `dataset/dataset_uppercase/`.

## Reports

- [`rapport_1.pdf`](rapport_1.pdf): first defense report (French)
- [`rapport_2.pdf`](rapport_2.pdf): final defense report (French)

## Authors

Team **QLF**:

- Alexandre Pern
- Jean-Philippe Vonthron
- Eliot Soumeillan
- Mathis Lafage
