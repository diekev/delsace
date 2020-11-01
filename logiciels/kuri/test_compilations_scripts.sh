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
# À FAIRE : "exemples/tests/test_tri.kuri" \
# À FAIRE : "exemples/tests/test_fichier.kuri" \
FICHIERS=("exemples/applications/base64.kuri" \
          "exemples/applications/calendrier_républicain.kuri" \
          "exemples/applications/cliente.kuri" \
          "exemples/applications/client_smtp.kuri" \
          "exemples/applications/crée_test_compilateur.kuri" \
          "exemples/applications/json.kuri" \
          "exemples/applications/labyrinthe.kuri" \
          "exemples/applications/serveuse.kuri" \
          "exemples/applications/serveuse_ssl.kuri" \
          "exemples/applications/sudoku.kuri" \
          "exemples/demos/demo_expansion_argument_variadique.kuri" \
          "exemples/demos/demo_info_type_fonction.kuri" \
          "exemples/demos/demo_logement.kuri" \
          "exemples/demos/demo_operateur_crochet.kuri" \
          "exemples/demos/demo_operateur_point.kuri" \
          "exemples/demos/demo_reference.kuri" \
          "exemples/demos/demo_retour_multiple.kuri" \
          "exemples/demos/demo_surcharge_fonctions.kuri" \
          "exemples/demos/demo_syntax_appel_uniforme.kuri" \
          "exemples/demos/demo_typage.kuri" \
          "exemples/tests/test_alloc.kuri" \
          "exemples/tests/test_assert_dans_fonction.kuri" \
          "exemples/tests/test_annotations.kuri" \
          "exemples/tests/test_boucle_pour.kuri" \
          "exemples/tests/test_corps_texte.kuri" \
          "exemples/tests/test_chaines_littérales.kuri" \
          "exemples/tests/test_couleur_terminal.kuri" \
          "exemples/tests/test_dessin_texte.kuri" \
          "exemples/tests/test_emploi.kuri" \
          "exemples/tests/test_énum_drapeau.kuri" \
          "exemples/tests/test_erreur.kuri" \
          "exemples/tests/test_ghtml.kuri" \
          "exemples/tests/test_logement.kuri" \
          "exemples/tests/test_memoire.kuri" \
          "exemples/tests/test_metaprogrammation.kuri" \
          "exemples/tests/test_metaprogramme_imprime.kuri" \
          "exemples/tests/test_moultfilage.kuri" \
          "exemples/tests/test_operateur.kuri" \
          "exemples/tests/test_plage_boucle.kuri" \
          "exemples/tests/test_polymorphisme.kuri" \
          "exemples/tests/test_ppm.kuri" \
          "exemples/tests/test_recherche.kuri" \
          "exemples/tests/test_routage.kuri" \
          "exemples/tests/test_structure_poly.kuri" \
          "exemples/tests/test_sys_fichier.kuri" \
          "exemples/tests/test_table_hachage.kuri" \
          "exemples/tests/test_unicode.kuri")

for fichier in ${FICHIERS[@]}; do
    echo "Compilation de :" $fichier
    kuri $fichier > /dev/null
done
