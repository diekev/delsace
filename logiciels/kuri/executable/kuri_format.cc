/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2024 Kévin Dietrich. */

#include <fstream>

#include "arbre_syntaxique/impression.hh"

#include "compilation/compilatrice.hh"
#include "compilation/syntaxeuse.hh"
#include "compilation/tacheronne.hh"

#include "parsage/lexeuse.hh"

#include "structures/chemin_systeme.hh"

#include "utilitaires/log.hh"

int main(int argc, char **argv)
{
    if (argc < 2) {
        dbg() << "Utilisation : " << argv[0] << " FICHIER";
        return 1;
    }

    auto chemin_fichier = kuri::chemin_systeme(argv[1]);
    chemin_fichier = kuri::chemin_systeme::absolu(chemin_fichier);

    auto arguments = ArgumentsCompilatrice{};
    arguments.importe_kuri = false;
    auto compilatrice = Compilatrice("", arguments);
    auto tacheronne = Tacheronne(compilatrice);
    auto module = Module(chemin_fichier.chemin_parent());
    auto donnees_fichier = Fichier();
    donnees_fichier.module = &module;
    auto tampon = charge_contenu_fichier({chemin_fichier.pointeur(), chemin_fichier.taille()});
    donnees_fichier.charge_tampon(lng::tampon_source(std::move(tampon)));

    auto lexeuse = Lexeuse(compilatrice.contexte_lexage(nullptr), &donnees_fichier);
    lexeuse.performe_lexage();

    auto unité = UniteCompilation(compilatrice.espace_de_travail_defaut);
    unité.fichier = &donnees_fichier;

    auto syntaxeuse = Syntaxeuse(tacheronne, &unité);
    syntaxeuse.analyse();

    assert(module.bloc);

    std::optional<int> dernière_ligne_lexème;

    Enchaineuse enchaineuse;
    POUR (*module.bloc->expressions.verrou_lecture()) {
        /* Essaie de préserver les séparations dans le texte originel. */
        if (dernière_ligne_lexème.has_value()) {
            if (it->lexème->ligne > (dernière_ligne_lexème.value() + 1)) {
                enchaineuse << "\n";
            }
        }

        imprime_arbre_formatté(enchaineuse, it);

        if (!expression_eu_bloc(it)) {
            enchaineuse << "\n";
        }

        dernière_ligne_lexème = it->lexème->ligne;
    }

    auto os = std::ofstream(vers_std_path(chemin_fichier));
    os << enchaineuse.chaine();

    return 0;
}
