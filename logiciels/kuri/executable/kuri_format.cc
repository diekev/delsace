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

static void formatte_fichier(kuri::chemin_systeme const chemin_fichier)
{
    auto arguments = ArgumentsCompilatrice{};
    arguments.importe_kuri = false;
    auto compilatrice = Compilatrice("", arguments);

    /* Création du module et du fichier. */
    auto &sys_module = compilatrice.sys_module;
    auto module = sys_module->trouve_ou_crée_module(nullptr, chemin_fichier.chemin_parent());
    auto résultat_fichier = sys_module->trouve_ou_crée_fichier(
        module, chemin_fichier.nom_fichier(), chemin_fichier);
    auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat_fichier));

    /* Chargement fichier. */
    auto tampon = charge_contenu_fichier({chemin_fichier.pointeur(), chemin_fichier.taille()});
    fichier->charge_tampon(lng::tampon_source(std::move(tampon)));

    /* Lexage. */
    auto lexeuse = Lexeuse(compilatrice.contexte_lexage(nullptr), fichier, INCLUS_COMMENTAIRES);
    lexeuse.performe_lexage();

    if (compilatrice.possède_erreur()) {
        return;
    }

    /* Syntaxage du fichier. */
    auto unité = UniteCompilation(compilatrice.espace_de_travail_defaut);
    unité.fichier = fichier;

    auto tacheronne = Tacheronne(compilatrice);
    auto syntaxeuse = Syntaxeuse(tacheronne, &unité);
    syntaxeuse.analyse();

    if (compilatrice.possède_erreur()) {
        return;
    }

    assert(module->bloc);

    /* Formattage de l'arbre. */
    Enchaineuse enchaineuse;
    imprime_arbre_formatté_bloc_module(enchaineuse, module->bloc);

    /* Écriture fichier. */
    auto résultat = enchaineuse.chaine();
    auto const &source = fichier->tampon().chaine();

    if (source != résultat) {
        auto os = std::ofstream(vers_std_path(chemin_fichier));
        os << résultat;
    }
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        dbg() << "Utilisation : " << argv[0] << " [FICHIER|DOSSIER]";
        return 1;
    }

    auto chemin_fichier = kuri::chemin_systeme(argv[1]);
    chemin_fichier = kuri::chemin_systeme::absolu(chemin_fichier);

    if (kuri::chemin_systeme::est_dossier(chemin_fichier)) {
        auto chemins = kuri::chemin_systeme::fichiers_du_dossier_recursif(chemin_fichier);
        POUR (chemins) {
            formatte_fichier(it);
        }
    }
    else if (kuri::chemin_systeme::est_fichier_kuri(chemin_fichier)) {
        formatte_fichier(chemin_fichier);
    }
    else {
        dbg() << "Le chemin " << chemin_fichier
              << " ne pointe ni vers un dossier ni vers un fichier Kuri.";
        return 1;
    }

    return 0;
}
