# QLF — Word Search Solver

Solveur de mots mêlés par OCR, écrit en C dans le cadre du projet OCR d'EPITA.
À partir d'une photo ou d'un scan de grille, le programme détecte les lettres,
les reconnaît avec un réseau de neurones, puis trouve et entoure les mots de la
liste directement sur l'image.

## Sommaire

- [Fonctionnement](#fonctionnement)
- [Dépendances](#dépendances)
- [Compilation](#compilation)
- [Utilisation](#utilisation)
- [Structure du projet](#structure-du-projet)
- [Modules autonomes](#modules-autonomes)
- [Réseau de neurones](#réseau-de-neurones)
- [Rapports](#rapports)
- [Auteurs](#auteurs)

## Fonctionnement

Le binaire `ocr` enchaîne les étapes suivantes :

1. **Pré-filtrage** : passage en niveaux de gris, puis filtre médian (optionnel) pour réduire le bruit.
2. **Rotation automatique** : détection de l'angle d'inclinaison et redressement de l'image (interpolation bilinéaire).
3. **Binarisation** : seuillage automatique par la méthode d'Otsu.
4. **Segmentation** : détection de la grille et de la liste de mots, puis découpage de chaque lettre.
5. **Reconnaissance** : chaque lettre (28×28 px) passe dans le réseau de neurones, qui prédit un caractère de `A` à `Z`.
6. **Résolution** : recherche de chaque mot dans la grille, dans les 8 directions, avec une tolérance aux erreurs de reconnaissance.
7. **Rendu** : les mots trouvés sont entourés en couleur sur l'image redressée.

## Dépendances

- `gcc` et `make`
- `pkg-config`
- SDL2 et SDL2_image
- gdk-pixbuf 2.0
- GTK+ 3 (interface graphique uniquement)
- Python 3, Pillow et NumPy (génération du dataset uniquement)

Exemple sous Arch Linux :

```bash
sudo pacman -S gcc make pkgconf sdl2 sdl2_image gdk-pixbuf2 gtk3
```

Exemple sous Debian / Ubuntu :

```bash
sudo apt install build-essential pkg-config libsdl2-dev libsdl2-image-dev libgdk-pixbuf-2.0-dev libgtk-3-dev
```

## Compilation

Toutes les commandes se lancent à la racine du dépôt.

| Commande    | Résultat                                             |
| ----------- | ---------------------------------------------------- |
| `make all`  | Compile la version CLI et l'interface graphique      |
| `make cli`  | Compile uniquement le binaire `ocr`                  |
| `make gui`  | Compile l'interface graphique `ocr_gui`              |
| `make clean`| Supprime les binaires et le dossier `output/`        |

## Utilisation

> Les binaires doivent être lancés **depuis la racine du dépôt** : ils utilisent
> le dossier `temp/`, le fichier de poids du réseau et écrivent dans `output/`.

### Ligne de commande

```bash
./ocr <image> <filtre_median>
```

- `<image>` : chemin vers l'image de la grille (PNG, JPG, BMP).
- `<filtre_median>` : `1` pour appliquer le filtre médian, `0` sinon.

Exemple avec une image fournie :

```bash
./ocr temp/level_1_image_1.png 0
```

Le programme affiche la grille et les mots reconnus, les coordonnées de chaque
mot trouvé, puis enregistre le résultat dans `output/solved.bmp`.

### Interface graphique

```bash
./ocr_gui
```

L'interface permet de :

- ouvrir une image ;
- activer ou non le filtre médian ;
- lancer l'OCR et afficher la grille résolue ;
- afficher la visualisation de la segmentation ;
- sauvegarder le résultat.

L'interface appelle `./ocr` en interne : il faut donc compiler les deux (`make all`).

### Images d'exemple

Des images de test sont disponibles dans `temp/` :

- `level_1_image_1.png`
- `level_2_image_1.png`
- `level_2_image_2.png`
- `level_3_image_1.png`

## Structure du projet

```
.
├── main.c               # Point d'entrée : pipeline complet de l'OCR
├── Makefile             # Build CLI + GUI
├── Preprocessing/       # Niveaux de gris, filtre médian, binarisation (Otsu)
├── RotationPNG/         # Détection de l'inclinaison et rotation automatique
├── segmentation/        # Détection de la grille / liste de mots et découpage des lettres
├── NewNeuralNetwork/    # Réseau de neurones utilisé par l'OCR + poids entraînés
├── NeuralNetwork/       # Première version du réseau de neurones
├── XNORNetwork/         # Preuve de concept : réseau apprenant la fonction XNOR
├── solver-cli/          # Solveur de mots mêlés (recherche dans 8 directions)
├── OcrGui/              # Interface graphique GTK
├── dataset/             # Polices et script Python de génération du dataset
├── temp/                # Images d'exemple et fichiers intermédiaires
├── rapport_1.pdf        # Rapport de la première soutenance
├── rapport_2.pdf        # Rapport de la soutenance finale
└── AUTHORS
```

## Modules autonomes

Chaque module possède son propre `Makefile` et peut être compilé et testé
séparément depuis son dossier.

| Module             | Binaire        | Utilisation                                         |
| ------------------ | -------------- | --------------------------------------------------- |
| `Preprocessing/`   | `ocr`          | `./ocr <image>`                                     |
| `RotationPNG/`     | `rotate_png`   | `./rotate_png <input.png> <angle\|auto> <output.bmp>` |
| `segmentation/`    | `segmentation` | `./segmentation <image.bmp>`                        |
| `solver-cli/`      | `solver`       | `./solver <grid_file> <mot> [max_erreurs]`          |
| `NewNeuralNetwork/`| `test_nn`      | `./test_nn` (entraînement et évaluation)            |
| `XNORNetwork/`     | `xor`          | `./xor`                                             |

Exemple avec le solveur seul :

```bash
cd solver-cli
make
./solver grid HORIZONTAL
```

Le fichier de grille contient une ligne de lettres majuscules par rangée.
Le solveur affiche les coordonnées de début et de fin du mot : `(x1,y1)(x2,y2)`.

## Réseau de neurones

- **Architecture** : perceptron multicouche 784 → 128 → 128 → 26
- **Entrée** : image de lettre binarisée en 28×28 pixels
- **Sortie** : une classe par lettre majuscule (`A` à `Z`)
- **Entraînement** : rétropropagation avec momentum et décroissance du taux d'apprentissage
- **Poids** : `NewNeuralNetwork/new_model_weights.bin`

### Générer le dataset

Le script `dataset/make_dataset.py` génère des images de lettres à partir des
polices de `dataset/fonts/`, avec de légères variations de taille et de rotation :

```bash
cd dataset
pip install pillow numpy
python3 make_dataset.py
```

Les images sont générées dans `dataset/dataset_uppercase/`.

## Rapports

- [`rapport_1.pdf`](rapport_1.pdf) : rapport de la première soutenance
- [`rapport_2.pdf`](rapport_2.pdf) : rapport de la soutenance finale

## Auteurs

Équipe **QLF** :

- Alexandre Pern
- Jean-Philippe Vonthron
- Eliot Soumeillan
- Mathis Lafage
