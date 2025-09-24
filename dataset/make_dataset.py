import random
from pathlib import Path
import sys

import numpy as np
from PIL import Image, ImageFont, ImageOps, ImageDraw

# ------------------ Settings ------------------
CHARS = "ABCDEFGHIJKLMNOPQRSTUVWXYZ"

IMG_SIZE = 28
N_SAMPLES_PER_FONT = 50
OUTPUT_DIR = Path("dataset_uppercase")
FONTS_DIR = Path("fonts")      # .ttf
SEED = 42

# Contrôles visuels
MARGIN = 2                     # marge autour du glyphe
FONT_MIN = 24                  # plus gros par défaut
FONT_MAX = 27
ANGLE_MAX = 2.0                # rotation max (très faible)
ROTATE_PROB = 0.20             # 20% seulement ont une rotation
INVERT = False                 # False = texte noir, fond blanc
# ---------------------------------------------

random.seed(SEED)
np.random.seed(SEED)

images_root = OUTPUT_DIR
images_root.mkdir(parents=True, exist_ok=True)

font_paths = [p for p in FONTS_DIR.glob("**/*.ttf")]
if not font_paths:
    raise FileNotFoundError("No font found")

def try_load_font(font_path: Path, size: int):
    try:
        return ImageFont.truetype(str(font_path), size=size)
    except Exception:
        return None

def render_char_binary(ch: str, font: ImageFont.FreeTypeFont, angle: float = 0.0) -> Image.Image:
    bg = 1 if not INVERT else 0  
    fg = 0 if not INVERT else 1

    base = Image.new("1", (IMG_SIZE, IMG_SIZE), color=bg)
    draw = ImageDraw.Draw(base)

    try:
        bbox = font.getbbox(ch)  # (x0, y0, x1, y1)
    except Exception:
        return None
    if not bbox:
        return None

    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]

    if w > IMG_SIZE - 2*MARGIN or h > IMG_SIZE - 2*MARGIN:
        scale_w = (IMG_SIZE - 2*MARGIN) / max(1, w)
        scale_h = (IMG_SIZE - 2*MARGIN) / max(1, h)
        scale = max(0.5, min(scale_w, scale_h))
        new_size = max(8, int(font.size * scale))
        font = try_load_font(Path(font.path), new_size)
        if font is None:
            return None
        bbox = font.getbbox(ch)
        w = bbox[2] - bbox[0]
        h = bbox[3] - bbox[1]

    x = (IMG_SIZE - w) // 2 - bbox[0]
    y = (IMG_SIZE - h) // 2 - bbox[1]

    draw.text((x, y), ch, font=font, fill=fg)

    if abs(angle) > 1e-6:
        pad = 4
        padded = Image.new("1", (IMG_SIZE + 2*pad, IMG_SIZE + 2*pad), color=bg)
        padded.paste(base, (pad, pad))
        base = padded.rotate(angle, resample=Image.NEAREST, expand=True, fillcolor=bg)
        base = ImageOps.fit(base, (IMG_SIZE, IMG_SIZE), method=Image.NEAREST, centering=(0.5, 0.5))

    if base.size != (IMG_SIZE, IMG_SIZE):
        base = ImageOps.fit(base, (IMG_SIZE, IMG_SIZE), method=Image.NEAREST, centering=(0.5, 0.5))
    out = base.convert("L")  
    return out

def ensure_char_dir(ch: str) -> Path:
    d = images_root / ch
    d.mkdir(parents=True, exist_ok=True)
    return d

def start_dataset(n_per_font: int):
    counters = {ch: 0 for ch in CHARS}
    total = len(font_paths) * len(CHARS) * n_per_font
    print(f"Génération de {total} images 28x28 binaire...")

    for font_path in font_paths:
        for ch in CHARS:
            samples = 0
            trials = 0
            while samples < n_per_font and trials < n_per_font * 3:
                trials += 1

                font_size = random.randint(FONT_MIN, FONT_MAX)
                font = try_load_font(font_path, size=font_size)
                if font is None:
                    break

                angle = 0.0
                if random.random() < ROTATE_PROB:
                    angle = random.uniform(-ANGLE_MAX, ANGLE_MAX)

                img = render_char_binary(ch, font, angle=angle)
                if img is None:
                    continue

                ch_dir = ensure_char_dir(ch)
                idx = counters[ch]
                fname = f"{idx:06d}.png"
                img.save(ch_dir / fname, format="PNG", optimize=True)

                counters[ch] += 1
                samples += 1

def main():
    global N_SAMPLES_PER_FONT
    if len(sys.argv) >= 2:
        nb_per_font = int(sys.argv[1])
        if nb_per_font > 0:
            N_SAMPLES_PER_FONT = nb_per_font
    start_dataset(N_SAMPLES_PER_FONT)

if __name__ == "__main__":
    main()
