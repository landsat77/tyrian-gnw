# Tyrian — installation

Requires Retro-Go SD **v2.0.0-rc1 or later** on the Game & Watch.

The game data is not included, but it is free: Tyrian 2.1 is freeware.
Download `tyrian21.zip` (https://camanis.net/tyrian/tyrian21.zip) and unzip
it.

1. Build the data pack (Python 3, nothing else to install). Keep the
   `ty_render` folder next to `ty_pack.py`: it renders the music.

       python3 ty_pack.py Tyrian.ty /path/to/tyrian21

   On Windows:

       py ty_pack.py Tyrian.ty C:\Games\tyrian21

   The pack is about 26 MB. Smaller: `--no-music` (about 10 MB).

2. Copy to the SD card, side by side, with the same name:

       /homebrews/Tyrian.bin
       /homebrews/Tyrian.ty

3. Start "Tyrian" from the Homebrew tab. The first start copies the pack to
   the external flash: it takes a minute or two.

Controls: D-pad = fly / move in the menus, A = fire (validate in the menus),
B = sidekicks (back in the menus), TIME = rear weapon mode, GAME = game
menu / back, PAUSE/SET = Retro-Go menu (savestates; Options: music, sound,
swap A/B). A savestate goes back to the game menu before the level being
played, with the ship, weapons and money of then (Tyrian saves between the
levels).

---

# Tyrian — installation (français)

Nécessite Retro-Go SD **v2.0.0-rc1 ou plus récent** sur la Game & Watch.

Les données du jeu ne sont pas fournies, mais elles sont gratuites : Tyrian
2.1 est un freeware. Télécharger `tyrian21.zip`
(https://camanis.net/tyrian/tyrian21.zip) et le décompresser.

1. Créer le pack de données (Python 3, rien d'autre à installer). Garder le
   dossier `ty_render` à côté de `ty_pack.py` : c'est lui qui calcule la
   musique.

       python3 ty_pack.py Tyrian.ty /chemin/vers/tyrian21

   Sous Windows :

       py ty_pack.py Tyrian.ty C:\Jeux\tyrian21

   Le pack fait environ 26 Mo. Plus petit : `--no-music` (environ 10 Mo).

2. Copier sur la carte SD, côte à côte, avec le même nom :

       /homebrews/Tyrian.bin
       /homebrews/Tyrian.ty

3. Lancer « Tyrian » depuis l'onglet Homebrew. Au premier lancement, le pack
   est copié dans la flash externe : cela prend une ou deux minutes.

Commandes : croix = piloter / se déplacer dans les menus, A = tirer (valider
dans les menus), B = armes latérales (retour dans les menus), TIME = mode de
l'arme arrière, GAME = menu du jeu / retour, PAUSE/SET = menu Retro-Go
(sauvegardes ; Options : musique, son, inverser A/B). Une sauvegarde ramène
au menu du jeu avant le niveau en cours, avec le vaisseau, les armes et
l'argent de ce moment (Tyrian sauvegarde entre les niveaux).
