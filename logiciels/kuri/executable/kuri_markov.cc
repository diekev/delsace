/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include <iostream>

#include "biblinternes/outils/gna.hh"
#include "biblinternes/structures/matrice_eparse.hh"

#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"

#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "options.hh"

#include "structures/chemin_systeme.hh"

using type_scalaire = double;
using type_matrice_ep = matrice_colonne_eparse<double>;

static auto idx_depuis_id(GenreLexème id)
{
    return static_cast<int>(id);
}

static auto id_depuis_idx(int id)
{
    return static_cast<GenreLexème>(id);
}

void test_markov_id_simple(kuri::tableau<Lexème, int> const &lexemes)
{
    static constexpr auto _0 = static_cast<type_scalaire>(0);
    static constexpr auto _1 = static_cast<type_scalaire>(1);

    /* construction de la matrice */
    auto nombre_id = static_cast<int>(GenreLexème::COMMENTAIRE) + 1;
    auto matrice = type_matrice_ep(type_ligne(nombre_id), type_colonne(nombre_id));

    for (auto i = 0; i < lexemes.taille() - 1; ++i) {
        auto idx0 = idx_depuis_id(lexemes[i].genre);
        auto idx1 = idx_depuis_id(lexemes[i + 1].genre);

        matrice(type_ligne(idx0), type_colonne(idx1)) += _1;
    }

    tri_lignes_matrice(matrice);
    converti_fonction_repartition(matrice);

    auto gna = GNA();
    auto mot_courant = GenreLexème::STRUCT;
    auto nombre_phrases = 5;

    while (nombre_phrases > 0) {
        std::cerr << chaine_du_lexème(mot_courant);

        if (est_mot_clé(mot_courant)) {
            std::cerr << ' ';
        }

        if (mot_courant == GenreLexème::ACCOLADE_FERMANTE) {
            std::cerr << '\n';
            nombre_phrases--;
        }

        /* prend le vecteur du mot_courant */
        auto idx = idx_depuis_id(mot_courant);

        /* génère un mot */
        auto prob = gna.uniforme(_0, _1);

        auto &ligne = matrice.lignes[idx];

        for (auto n : ligne) {
            if (prob <= n->valeur) {
                mot_courant = id_depuis_idx(static_cast<int>(n->colonne));
                break;
            }
        }
    }
}

int main(int argc, const char **argv)
{
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "Utilisation : " << argv[0] << " FICHIER\n";
        return 1;
    }

    auto const &chemin_racine_kuri = getenv("RACINE_KURI");

    if (chemin_racine_kuri == nullptr) {
        std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
        std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement "
                     "'RACINE_KURI' soit définie !\n";
        return 1;
    }

    auto const chemin_fichier = argv[1];

    if (chemin_fichier == nullptr) {
        std::cerr << "Aucun fichier spécifié !\n";
        return 1;
    }

    if (!kuri::chemin_systeme::existe(chemin_fichier)) {
        std::cerr << "Impossible d'ouvrir le fichier : " << chemin_fichier << '\n';
        return 1;
    }

    {
        auto chemin = kuri::chemin_systeme::absolu(chemin_fichier);
        auto compilatrice = Compilatrice("", {});
        auto donnees_fichier = Fichier();
        auto tampon = charge_contenu_fichier({chemin.pointeur(), chemin.taille()});
        donnees_fichier.charge_tampon(lng::tampon_source(std::move(tampon)));

        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(nullptr), &donnees_fichier);
        lexeuse.performe_lexage();

        test_markov_id_simple(donnees_fichier.lexèmes);
    }

    return 0;
}
