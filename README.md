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
| LLVM                        | 6.0     |

Branches
--------

### introspection

Implémentation d'outils pour inspecter les types de variables lors de l'exécution des programmes. Introduction d'un type générique 'eini', similaire à std::any, mais permettant de connaître les données du type de la variable contenu, avec un dévelopement automatique du type envelopé.

Requiers de travailler sur, réparer, les chaînes de caractères.

### nommage_enum

Force la déclaration des énumaration à avoir un nom, similairement à une structure, pour enforcer un typage stricte des variables.

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
| while        | ?            | À FAIRE                          |
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
| delete/free  | déloge       | porc de capitaliste              |

