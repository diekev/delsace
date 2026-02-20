/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2026 Kévin Dietrich. */

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

#include "utilitaires/algorithmes.hh"
#include "utilitaires/log.hh"

enum struct Étape_Échouant : int8_t {
    INCONNUE,
    COMPILATION,
    EXÉCUTION,
};

struct Info_Test_Échouant {
    kuri::chaine_statique coulisse = "";
    kuri::chaine_statique chemin = "";
    Étape_Échouant étape_échouant{};
};

struct Résultat_Exécution_Test {
    int32_t nombre_de_tests = 0;
    int32_t nombre_de_tests_ignorés = 0;
    kuri::tableau<Info_Test_Échouant> tests_échouant{};
};

static bool doit_ignorer_test(kuri::chaine_statique chemin, kuri::chaine_statique coulisse)
{
    if (coulisse == "c") {
        // [-Werror=type-limits]
        if (chemin == "coulisses/test-021-logique_entier.kuri") {
            return true;
        }
        // [-Werror=overflow]
        if (chemin == "coulisses/test-022-négation_binaire.kuri") {
            return true;
        }
        // [-Werror=type-limits]
        if (chemin == "coulisses/test-023-test_décalage_binaire.kuri") {
            return true;
        }
    }
    return false;
}

static void compile_fichiers_pour_coulisse(kuri::tableau_statique<kuri::chemin_systeme> chemins,
                                           kuri::chaine_statique coulisse,
                                           Résultat_Exécution_Test *résultat_exécution)
{
    POUR (chemins) {
        résultat_exécution->nombre_de_tests += 1;

        if (doit_ignorer_test(it, coulisse)) {
            résultat_exécution->nombre_de_tests_ignorés += 1;
            continue;
        }

        Enchaineuse enchaineuse;
        enchaineuse.ajoute("kuri --coulisse ");
        enchaineuse.ajoute(coulisse);
        enchaineuse.ajoute(" ");
        if (coulisse == "asm") {
            enchaineuse.ajoute("--débogage-ne-compile-que-nécessaire ");
        }
        enchaineuse.ajoute(it);
        enchaineuse.ajoute(" > /dev/null");
        enchaineuse << '\0';

        auto nom_exécutable = it.remplace_extension("");
        if (kuri::chemin_systeme::existe(nom_exécutable)) {
            if (!kuri::chemin_systeme::supprime(nom_exécutable)) {
                dbg() << "Impossible de supprimer " << nom_exécutable;
            }
        }

        auto commande = enchaineuse.chaine();

        info() << "Compilation de '" << it << "'";
        auto résultat = system(commande.pointeur());
        if (résultat == 0) {
            auto commande = enchaine("./", nom_exécutable, '\0');

            info() << "Exécution de '" << nom_exécutable << "'";
            résultat = system(commande.pointeur());
            if (résultat != 0) {
                Info_Test_Échouant info;
                info.chemin = it;
                info.coulisse = coulisse;
                info.étape_échouant = Étape_Échouant::EXÉCUTION;
                résultat_exécution->tests_échouant.ajoute(info);
            }

            if (kuri::chemin_systeme::existe(nom_exécutable)) {
                if (!kuri::chemin_systeme::supprime(nom_exécutable)) {
                    dbg() << "Impossible de supprimer " << nom_exécutable;
                }
            }
        }
        else {
            Info_Test_Échouant info;
            info.chemin = it;
            info.coulisse = coulisse;
            info.étape_échouant = Étape_Échouant::COMPILATION;
            résultat_exécution->tests_échouant.ajoute(info);
        }
    }
}

static bool possède_coulisse(kuri::tableau_statique<kuri::chaine_statique> coulisses,
                             kuri::chaine_statique coulisse)
{
    POUR (coulisses) {
        if (it == coulisse) {
            return true;
        }
    }
    return false;
}

int main(int argc, char **argv)
{
    kuri::tablet<kuri::chaine_statique, 3> coulisses;
    kuri::tablet<kuri::chemin_systeme, 16> tablet_chemins;

    for (int i = 1; i < argc; i++) {
        auto arg = kuri::chaine_statique(argv[i]);

        if (arg == "-c") {
            if (!possède_coulisse(coulisses, arg)) {
                coulisses.ajoute("c");
            }
        }
        else if (arg == "-asm") {
            if (!possède_coulisse(coulisses, arg)) {
                coulisses.ajoute("asm");
            }
        }
        else if (arg == "-llvm") {
            if (!possède_coulisse(coulisses, arg)) {
                coulisses.ajoute("llvm");
            }
        }
        else if (kuri::chemin_systeme::est_fichier_kuri(arg)) {
            tablet_chemins.ajoute(arg);
        }
        else {
            dbg() << "Argument inconnu : " << arg;
            return 1;
        }
    }

    if (coulisses.est_vide()) {
        coulisses.ajoute("asm");
        coulisses.ajoute("c");
        coulisses.ajoute("llvm");
    }

    if (tablet_chemins.est_vide()) {
        tablet_chemins = kuri::chemin_systeme::fichiers_du_dossier("coulisses");
    }

    auto chemins = kuri::tableau_statique<kuri::chemin_systeme>(tablet_chemins);

    kuri::tri_rapide(chemins,
                     [](kuri::chemin_systeme a, kuri::chemin_systeme b) -> bool { return a < b; });

    Résultat_Exécution_Test résultat_exécution;

    POUR (coulisses) {
        compile_fichiers_pour_coulisse(chemins, it, &résultat_exécution);
    }

    info() << "";

    auto tests_échouants = int32_t(résultat_exécution.tests_échouant.taille());
    auto tests_ignorés = résultat_exécution.nombre_de_tests_ignorés;

    info() << "Nombre de tests : " << résultat_exécution.nombre_de_tests;
    info() << "      réussites : "
           << résultat_exécution.nombre_de_tests - tests_échouants - tests_ignorés;
    info() << "         échecs : " << tests_échouants;
    info() << "        ignorés : " << tests_ignorés;

    info() << "";

    auto résultat = 0;

    if (résultat_exécution.tests_échouant.taille()) {
        kuri::chaine_statique coulisse_courante = "";
        POUR (résultat_exécution.tests_échouant) {
            if (it.coulisse != coulisse_courante) {
                info() << "\nPour la coulisse '" << it.coulisse << "'";
                coulisse_courante = it.coulisse;
            }

            if (it.étape_échouant == Étape_Échouant::COMPILATION) {
                info() << "    Échec de la compilation de " << it.chemin;
            }
            else if (it.étape_échouant == Étape_Échouant::EXÉCUTION) {
                info() << "    Échec de l'exécution de " << it.chemin;
            }
            else {
                info() << "    Échec inconnu de " << it.chemin;
            }
        }

        info() << "\n[ ÉCHEC ]";
        résultat = 1;
    }
    else {
        info() << "\n[ SUCCÈS ]";
    }

    return 0;
}
