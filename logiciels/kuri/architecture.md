Kuri
----

Compilatrice
------------

La compilatrice supervise les Espaces de Travail, l'ouverture et la fermeture de fichier et modules, et la gestion des erreurs.
Les bibliothèques externes.

Espace De Travail
-----------------

Les espaces de travail sont là pour compiler séparement des cibles.


Compilation
-----------

Unité de compilation.
---------------------

Ordonnanceuse
-------------

Lexage
------

Découpe le texte d'un fichier en lexème. Création d'un tableau unique de lexèmes pour chaque fichier.
Un fichier, peut importe dans combien d'espace de travail il est utilisé, n'est lexer qu'une seule fois.
Contrairement à d'autres langages de compilation, nous séparons le lexage du syntaxage afin de pouvoir améliorer la cohérence de cache et la vitesse d'exécution.

Syntaxage
---------

Les fichiers sont syntaxés pour chaque espace de travail en ce moment.

Simplification de l'Arbre syntaxique
------------------------------------

Après le syntaxage, les arbres syntaxiques sont simplifiés afin de transformer des expressions complexes en expressions plus simples.

Les boucles « pour », « tantque », et « répète » sont transformées en boucles sans borne.
Les discriminations en branches si/sinon.
Les références à des constantes sont transformées en des noeuds littéraux.
Les opérateurs binaires surchargées en appel de fonction.

Génération de RI
----------------

La représentation intermédiaire est une structure utilisée pour simplifier la génération de code final depuis l'arbre syntaxique.

Génération de Code Binaire
--------------------------

Pour exécuter le code lors de la compilation, la RI est transformée en un code binaire plus facile à exécuter, évitant les problèmes liés à la cohérence de cache notamment.

Génération de Code Coulisse
---------------------------

À la fin de la compilation, le code final pour la cible de compilation (C, LLVM) est généré depuis la RI.

Métaprogrammation
-----------------

Messagerie
----------

Structure des fichiers
----------------------

compilation
representation_intermédiarie
exécutable

lanceur.cc contient le code pour la fonction principale du compilateur
parcer.cc  contient du code utilisant l'IPA de Clang pour générer du code Kuri depuis du code C ou C++
