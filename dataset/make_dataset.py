import os
import random
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageDraw, ImageFont, ImageFilter

# ------------------ Settings ------------------
CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

IMG_SIZE = 64                  
N_SAMPLES_PER_FONT = 50        
OUTPUT_DIR = Path("dataset_uppercase")
FONTS_DIR = Path("fonts")      # ttf only
SEED = 42
# ---------------------------------------------

random.seed(SEED)
np.random.seed(SEED)

images_root = OUTPUT_DIR
images_root.mkdir(parents=True, exist_ok=True)


font_paths = [p for p in FONTS_DIR.glob("**/*.ttf")]
if not font_paths:
    raise FileNotFoundError("Aucune police .ttf trouvée dans fonts/. Ajoute au moins une police.")

def try_load_font(font_path: Path, size: int) -> ImageFont.FreeTypeFont | None:
    try:
        return ImageFont.truetype(str(font_path), size=size)
    except Exception:
        return None

def render_char(ch: str, font: ImageFont.FreeTypeFont, invert=False,
                angle=0.0, dx=0, dy=0, noise_level=0.0) -> Image.Image:
    bg = 0 if invert else 255
    fg = 255 if invert else 0

    img = Image.new("L", (IMG_SIZE, IMG_SIZE), color=bg)
    draw = ImageDraw.Draw(img)

    try:
        bbox = font.getbbox(ch)
    except Exception:
        return None
    if bbox is None:
        return None
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]

    x = (IMG_SIZE - w) // 2 - bbox[0] + dx
    y = (IMG_SIZE - h) // 2 - bbox[1] + dy

    try:
        draw.text((x, y), ch, font=font, fill=fg)
    except Exception:
        return None

    if abs(angle) > 0.01:
        img = img.rotate(angle, resample=Image.BICUBIC, expand=True, fillcolor=bg)
        W, H = img.size
        left = max(0, (W - IMG_SIZE) // 2)
        top = max(0, (H - IMG_SIZE) // 2)
        img = img.crop((left, top, left + IMG_SIZE, top + IMG_SIZE))
        if img.size != (IMG_SIZE, IMG_SIZE):
            base = Image.new("L", (IMG_SIZE, IMG_SIZE), color=bg)
            base.paste(img, ((IMG_SIZE - img.size[0]) // 2, (IMG_SIZE - img.size[1]) // 2))
            img = base

    if random.random() < 0.2:
        img = img.filter(ImageFilter.GaussianBlur(radius=random.uniform(0.2, 0.8)))
    if random.random() < 0.2:
        img = img.filter(ImageFilter.UnsharpMask(radius=1, percent=150, threshold=2))

    if noise_level > 0:
        arr = np.array(img, dtype=np.float32)
        arr += np.random.normal(0, noise_level, arr.shape)
        arr = np.clip(arr, 0, 255).astype(np.uint8)
        img = Image.fromarray(arr, mode="L")

    return img

def ensure_char_dir(ch: str) -> Path:
    d = images_root / ch
    d.mkdir(parents=True, exist_ok=True)
    return d


def start_dataset():
    counters = {ch: 0 for ch in CHARS}
       
    for font_path in font_paths:
        for ch in CHARS:
            samples = 0
            trials = 0
            while samples < N_SAMPLES_PER_FONT and trials < N_SAMPLES_PER_FONT * 3:
                trials += 1

                font_size = random.randint(int(IMG_SIZE * 0.50), int(IMG_SIZE * 0.90))
                angle = random.uniform(-10.0, 10.0)
                dx = random.randint(-3, 3)
                dy = random.randint(-3, 3)
                invert = random.random() < 0.1
                noise_level = random.uniform(0.0, 8.0)

                font = try_load_font(font_path, size=font_size)
                if font is None:
                    break  

                img = render_char(ch, font, invert=invert, angle=angle, dx=dx, dy=dy, noise_level=noise_level)
                if img is None:
                    continue

                ch_dir = ensure_char_dir(ch)

                idx = counters[ch]
                fname = f"{idx:06d}.png"
                rel_path = f"{ch}/{fname}"
                img.save(ch_dir / fname, format="PNG", optimize=True)

                counters[ch] += 1
                samples += 1


def main():
    if len(sys.argv) < 2:
        print("Usage: python3 make_dataset.py <sample_per_font, 0 = default (50)>")
        sys.exit(1)
    nb_per_font = int(sys.argv[1])
    if nb_per_font != 0:
        N_SAMPLES_PER_FONT = nb_per_font
    start_dataset()

if __name__ == "__main__":
    main()

