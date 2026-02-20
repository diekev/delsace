/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2026 Kévin Dietrich. */

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/log.hh"

static void compile_fichiers_pour_coulisse(kuri::tableau_statique<kuri::chemin_systeme> chemins,
                                           kuri::chaine_statique coulisse)
{
    POUR (chemins) {
        Enchaineuse enchaineuse;
        enchaineuse.ajoute("kuri --coulisse ");
        enchaineuse.ajoute(coulisse);
        enchaineuse.ajoute(" ");
        if (coulisse == "asm") {
            enchaineuse.ajoute("--débogage-ne-compile-que-nécessaire ");
        }
        enchaineuse.ajoute(it);
        enchaineuse << '\0';

        auto nom_exécutable = it.remplace_extension("");
        if (kuri::chemin_systeme::existe(nom_exécutable)) {
            if (!kuri::chemin_systeme::supprime(nom_exécutable)) {
                dbg() << "Impossible de supprimer " << nom_exécutable;
            }
        }

        auto commande = enchaineuse.chaine();

        auto résultat = system(commande.pointeur());
        if (résultat == 0) {
            auto commande = enchaine("./", nom_exécutable, '\0');

            info() << "Exécution de '" << nom_exécutable << "'";
            résultat = system(commande.pointeur());
            if (résultat != 0) {
                dbg() << "Erreur lors de l'exécution de '" << nom_exécutable << "'";
            }

            if (kuri::chemin_systeme::existe(nom_exécutable)) {
                if (!kuri::chemin_systeme::supprime(nom_exécutable)) {
                    dbg() << "Impossible de supprimer " << nom_exécutable;
                }
            }
        }
        else {
            dbg() << "Erreur lors de la compilation de " << it;
        }
    }
}

int main()
{
    auto tablet_chemins = kuri::chemin_systeme::fichiers_du_dossier("coulisses");
    auto chemins = kuri::tableau_statique<kuri::chemin_systeme>(tablet_chemins);

    kuri::tri_rapide(chemins,
                     [](kuri::chemin_systeme a, kuri::chemin_systeme b) -> bool { return a < b; });

    compile_fichiers_pour_coulisse(chemins, "asm");
    compile_fichiers_pour_coulisse(chemins, "c");
    compile_fichiers_pour_coulisse(chemins, "llvm");

    return 0;
}
