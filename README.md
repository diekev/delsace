Kuri · クリ
==========

Langage de programmation basé sur le français avec un nom japonais et quelque mot-clé alsacien.

Le but est d'avoir un langage simple, sécurisé, et rapide à compiler, et à exécuter.

Voir https://delsace.fr/technologie/kuri/ pour plus d'informations.

### Dependances

| Nom                         | Version |
|-----------------------------|---------|
| C++                         | 17      |
| CMake                       | 3.2     |
| Bibliothèque C++ de Delsace | 1.0     |
| LLVM (optionel)             | 6.0     |

Branches
--------

### suppression_point_virgule

Test pour supprimer les points virgules à la fin des expressions. L'idée est que ce point virgule qui délimite les expressions est redondant avec la vérification de la validité des expressions, donc on pourrait s'en passer.

### table_hachage

Tentative de dévelopement d'une table de hachage avec un indexage plus rapide : puisque nous ne faisons qu'insérer des valeurs dans les tables de hachages, nous pourrions les optimiser pour supprimer les contraintes des algorithmes prenant en compte la possible suppression d'alvéoles.


### Mot-clés

Recherche pour les mot-clés.

| Nom anglais  | Mot français | Notes                            |
|--------------|--------------|----------------------------------|
| loop         | boucle       | sans condition                   |
| for...in     | pour...dans  |                                  |
| for...not in | pour...hors  | À FAIRE                          |
| while        | tantque      | avec condition                   |
| continue     | continue     |                                  |
| break        | arrête       | ou cesse                         |
| N/A          | sansarrêt    | si la boucle n'a pas été arrêtée |
| if           | si           |                                  |
| else         | sinon        |                                  |
| unless       | saufsi       | À FAIRE                          |
| defere       | diffère      |                                  |
| return       | retourne     | ou renvoie                       |
| array        | tableau      |                                  |
| map          | table        |                                  |
| list         | liste        |                                  |
| stack        | pile         |                                  |
| struct       | structure    |                                  |
| function     | fonction     | ou routine, avec coroutine       |
| any          | eini         | 's Elsàss                        |
| new/malloc   | loge         | donne une adresse                |
| realloc      | reloge       | donne une nouvelle adresse       |
| delete/free  | déloge       | libère l'adress                  |

