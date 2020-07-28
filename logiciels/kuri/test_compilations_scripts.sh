#!/bin/bash

# cas pour les dossiers

DOSSIER_COURANT=`pwd`

DOSSIERS=("exemples/applications/BDD" \
          "exemples/applications/EditeurTexte" \
          "exemples/applications/Eliza" \
          "exemples/applications/Jeu" \
          "exemples/applications/MoteurRendu" \
          "exemples/applications/Site")

for dossier in ${DOSSIERS[@]}; do
    cd "$DOSSIER_COURANT/$dossier"

    echo "Compilation de :" $dossier
    kuri principale.kuri > /dev/null
done

cd $DOSSIER_COURANT

# cas pour les fichiers simples

# À FAIRE : exemples/applications/jeu_chaos.kuri
# À FAIRE : "exemples/applications/client_smtp.kuri" \ tente piège err n'a pas de type
# À FAIRE : "exemples/applications/serveuse_ssl.kuri" \ tente piège err n'a pas de type
# À FAIRE : "exemples/applications/cliente.kuri" \ tente piège err retourn mauvais type
FICHIERS=("exemples/applications/base64.kuri" \
          "exemples/applications/calendrier_républicain.kuri" \
          "exemples/applications/crée_test_compilateur.kuri" \
          "exemples/applications/json.kuri" \
          "exemples/applications/labyrinthe.kuri" \
          "exemples/applications/serveuse.kuri")

for fichier in ${FICHIERS[@]}; do
    echo "Compilation de :" $fichier
    kuri $fichier > /dev/null
done
