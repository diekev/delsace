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
# À FAIRE : exemples/demos/demo_coroutine.kuri
# À FAIRE : exemples/demos/demo_coro_nombre_premier.kuri
FICHIERS=("exemples/applications/base64.kuri" \
          "exemples/applications/calendrier_républicain.kuri" \
          "exemples/applications/cliente.kuri" \
          "exemples/applications/client_smtp.kuri" \
          "exemples/applications/crée_test_compilateur.kuri" \
          "exemples/applications/json.kuri" \
          "exemples/applications/labyrinthe.kuri" \
          "exemples/applications/serveuse.kuri" \
          "exemples/applications/serveuse_ssl.kuri" \
          "exemples/demos/demo_expansion_argument_variadique.kuri" \
          "exemples/demos/demo_gabarit.kuri" \
          "exemples/demos/demo_info_type_fonction.kuri" \
          "exemples/demos/demo_logement.kuri" \
          "exemples/demos/demo_operateur_crochet.kuri" \
          "exemples/demos/demo_operateur_point.kuri" \
          "exemples/demos/demo_reference.kuri" \
          "exemples/demos/demo_retour_multiple.kuri" \
          "exemples/demos/demo_surcharge_fonctions.kuri" \
          "exemples/demos/demo_syntax_appel_uniforme.kuri" \
          "exemples/demos/demo_typage.kuri" \
          "exemples/demos/demo_var_emploiee.kuri" \
          "exemples/doc_kuri_dls/ex01_bonjour.kuri" \
          "exemples/doc_kuri_dls/ex02_inference.kuri" \
          "exemples/doc_kuri_dls/ex03_constance.kuri" \
          "exemples/doc_kuri_dls/ex04_tableau.kuri" \
          "exemples/doc_kuri_dls/ex05_fonc_variadique.kuri" \
          "exemples/doc_kuri_dls/ex06_introspection.kuri" \
          "exemples/doc_kuri_dls/ex07_coroutine.kuri")

for fichier in ${FICHIERS[@]}; do
    echo "Compilation de :" $fichier
    kuri $fichier > /dev/null
done
