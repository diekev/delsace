Kuri · クリ
==========

Langage de programmation basé sur le français.

Le but est d'avoir un langage simple, sécurisé, rapide à compiler, et à exécuter, pour remplacer C, C++, et d'autres langages essayant d'être un peu trop malin vis-à-vis de la gestion de la mémoire.

Voir https://delsace.fr/technologie/kuri/ pour plus d'informations.

Pour compiler ce projet seul, utilisez la commande CMake -DSEULEMENT_KURI=ON.

## Dependances
Pour pouvoir compiler Kuri, il faudra plusieurs outils et bibliothèques
- C++ 17, avec la bibliothèque filesystem
- CMake 3.2
- (optionnellement) LLVM 6.0

## Structure du dossier
#### bibliotheques
Bibliothèque standarde du langage. Contient les modules déjà écrits.

#### compilation
Les sources du compilateur.

#### description
Divers fichiers pour décrire ou documenter le langage. Inclus un script générant des fichiers C++ contenant les mots-clés.

#### executable
Les exécutables générés par la compilation de ce projet, incluant le compilateur, et divers outils pour manipuler les sources logiciels écrites en Kuri.

#### exemples
Divers fichiers démontrant comment écrire des programmes en Kuri.

#### outils
Outils pour VSCode et Vim.

#### propositions
Diverses propositions pour faire évoluer le langage.

#### tests
Tests du compilateur.

