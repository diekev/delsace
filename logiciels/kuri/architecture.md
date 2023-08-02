Kuri
----
ADN
---

Le langage permettant de comuniquer entre les programmes compilés et la Compilatrice, il est important d'avoir un système permettant de passer des structures entre eux.

Ces structures doivent être tenues synchronisées entre la Compilatrice et le langage.

Pour ce faire, nous générons du code lors de la compilation de la Compilatrice à partir de définitions écrites dans un langage dédié. Ce système s'appel l'ADN.

Nous générons lors de la compilation les définitions C++ et Kuri de structures clés, ainsi que des fonctions auxilliaires afin de réduire le coup de maintenance du langage, et évite les bugs dûs à des problèmes d'ABI.

Le code généré contient :
- les lexèmes
- la table d'empreinte parfaite pour déterminer si un identifiant est un mot-clé ou non
- les noeuds syntaxiques pour la Compilatrice (NoeudExpression) et pour Kuri (NoeudCode)
- l'allocation des noeuds, et la conversion entre les NoeudExpression et Kuri
- l'assembleuse de l'arbre syntaxiques
- la copie d'arbre syntaxique, principalement utilisée pour monomorphiser une fonction ou une structure
- l'impression d'arbre syntaxique
- le calcul de l'étendue d'un arbre syntaxique dans le code source, utile pour sur- ou sousligner le code source dans les messages d'erreurs
- plusieurs fonctions pour les impressions de débogage
- les messages passés entre la Compilatrice et les programmes, code C++ et Kuri
- l'IPA de la Compilatrice pour les métaprogrammes
- les options de compilations

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
