Delsace
=======

Ensemble de mes sources logiciels écrites au fil des ans, et rassemblées au sein d'un même dépôt afin de mieux partager le code entre différents projets et simplifier la maintenance vis-à-vis de l'utilisation de bibliothèques partagées.

### Dossiers

biblexternes
------------

Bibliothèques logiciels externes, écrites par d'autres personnes, incluses pour simplifier la compilation des sources, et éviter d'avoir trop de dépendances externes.

biblinternes
------------

Bibliothèques logiciels internes, écrites pour soutenir les différents logiciels présents dans ce dépôt de sources.

cmake
-----

Outils CMake, notamment pour trouver certaines bibliothèques systèmes.

ebauches
--------

Ensemble de fichiers C/C++ écrits pour essayer certaines idées sans pour autant créer une bibliothèque ou un logiciel, et conserver pour une possible réutilisation ou comme base dans des projets ultérieurs.

logiciels
---------

Ensemble de logiciels écrits au fil des ans.

tests
-----

Tests unitaires pour certaines bibliothèques internes.

### Dépendances

Les dépendances dépendent du projet, mais la configuration minimale requise est :

| Nom                         | Version |
|-----------------------------|---------|
| C++                         | 20      |
| CMake                       | 3.2     |
| Linux                       | n/a     |

D'autres dépendances, selon les projets sont (non-exhaustif) :

| Nom                         | Version |
|-----------------------------|---------|
| Alembic                     |         |
| Bullet                      |         |
| LLVM                        | 6.0     |
| OpenColorIO                 |         |
| OpenEXR                     |         |
| OpenSubDiv                  |         |
| OpenVDB                     |         |
| PTex                        |         |
| Qt                          | 5.5     |
