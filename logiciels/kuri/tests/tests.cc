/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#include "test_decoupage.h"

#include <fstream>
#include <sys/wait.h>
#include <unistd.h>

#include "biblinternes/chrono/outils.hh"

#include "arbre_syntaxique/assembleuse.hh"

#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"
#include "compilation/espace_de_travail.hh"
#include "compilation/tacheronne.hh"

#include "parsage/modules.hh"

#include "structures/chemin_systeme.hh"

struct Test {
    const char *cas = "";
    const char *source = "";
    erreur::Genre résultat_attendu = erreur::Genre::AUCUNE_ERREUR;
};

static Test tests_unitaires[] = {
    // À FAIRE : désactivation de ces tests pour le moment car la fonctionnalité est brisée depuis
    // le réusinage de l'arbre
    //	{
    //		"",
    //		"fichiers/test_module_correcte.kuri",
    //		erreur::Genre::AUCUNE_ERREUR
    //	},
    //	{
    //		"",
    //		"fichiers/test_fonction_inconnue_module.kuri",
    //		erreur::Genre::FONCTION_INCONNUE
    //	},
    {"", "fichiers/test_lexage_nombre_correcte.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_lexage_nombre_erreur.kuri", erreur::Genre::LEXAGE},
    {"", "fichiers/test_module_inconnu.kuri", erreur::Genre::MODULE_INCONNU},
    {"", "fichiers/test_utilisation_module_inconnu.kuri", erreur::Genre::VARIABLE_INCONNUE},
    {"", "fichiers/test_type_retour_rien.kuri", erreur::Genre::ASSIGNATION_RIEN},
    {"", "fichiers/test_assignation_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_assignation_invalide.kuri", erreur::Genre::ASSIGNATION_INVALIDE},
    {"",
     "fichiers/test_assignation_types_differents.kuri",
     erreur::Genre::ASSIGNATION_MAUVAIS_TYPE},
    {"", "fichiers/test_erreur_syntaxage.kuri", erreur::Genre::SYNTAXAGE},
    {"", "fichiers/test_boucle_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_boucle_erreur_types_differents.kuri", erreur::Genre::TYPE_DIFFERENTS},
    {"", "fichiers/test_boucle_erreur_controle_invalide.kuri", erreur::Genre::CONTROLE_INVALIDE},
    {"", "fichiers/test_boucle_erreur_variable_inconnue.kuri", erreur::Genre::VARIABLE_INCONNUE},
    {"",
     "fichiers/test_appel_fonction_erreur_nombre_argument.kuri",
     erreur::Genre::NOMBRE_ARGUMENT},
    {"",
     "fichiers/test_appel_fonction_erreur_fonction_inconnue.kuri",
     erreur::Genre::FONCTION_INCONNUE},
    {"",
     "fichiers/test_appel_fonction_erreur_argument_inconnu.kuri",
     erreur::Genre::ARGUMENT_INCONNU},
    {"", "fichiers/test_appel_fonction_erreur_type_argument.kuri", erreur::Genre::TYPE_ARGUMENT},
    {"",
     "fichiers/test_appel_fonction_erreur_argument_redefini.kuri",
     erreur::Genre::ARGUMENT_REDEFINI},
    {"", "fichiers/test_declaration_fonctin_erreur_normale.kuri", erreur::Genre::NORMAL},
    {"", "fichiers/test_expression_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_condition_controle_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_condition_controle_types_differents.kuri", erreur::Genre::TYPE_DIFFERENTS},
    {"", "fichiers/test_expression_types_differents.kuri", erreur::Genre::TYPE_DIFFERENTS},
    {"", "fichiers/test_operateurs_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_operateurs_types_differents.kuri", erreur::Genre::TYPE_DIFFERENTS},
    {"", "fichiers/test_fonction_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_fonction_types_differents.kuri", erreur::Genre::TYPE_DIFFERENTS},
    {"", "fichiers/test_structure_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_structure_membre_inconnu.kuri", erreur::Genre::MEMBRE_INCONNU},
    {
        "",
        "fichiers/test_structure_redefinie.kuri",
        erreur::Genre::VARIABLE_REDEFINIE  // À FAIRE : considère plutôt SYMBOLE_REDIFINI, et
                                           // fusionne tous les tests
    },
    {"", "fichiers/test_type_inconnu.kuri", erreur::Genre::TYPE_INCONNU},
    {"", "fichiers/test_structure_variable_inconnue.kuri", erreur::Genre::VARIABLE_INCONNUE},
    {"", "fichiers/test_structure_types_differents.kuri", erreur::Genre::TYPE_DIFFERENTS},
    {"", "fichiers/test_structure_membre_redefini.kuri", erreur::Genre::MEMBRE_REDEFINI},
    {"", "fichiers/test_tableau_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_tableau_type_argument.kuri", erreur::Genre::TYPE_ARGUMENT},
    {"", "fichiers/test_transtypage_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"",
     "fichiers/test_transtypage_assignation_mauvais_type.kuri",
     erreur::Genre::ASSIGNATION_MAUVAIS_TYPE},
    {"", "fichiers/test_variable_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
    {"", "fichiers/test_variable_inconnue.kuri", erreur::Genre::VARIABLE_INCONNUE},
    {"", "fichiers/test_variable_redefinie.kuri", erreur::Genre::VARIABLE_REDEFINIE},
    {"", "fichiers/test_variable_redefinie.kuri", erreur::Genre::VARIABLE_REDEFINIE},
    {"", "fichiers/test_appel_fonction_aucune_erreur.kuri", erreur::Genre::AUCUNE_ERREUR},
};

static erreur::Genre lance_test(lng::tampon_source &tampon)
{
    auto chemin_courant = kuri::chemin_systeme::chemin_courant();
    kuri::chemin_systeme::change_chemin_courant("/opt/bin/kuri/fichiers_tests/fichiers/");

    auto compilatrice = Compilatrice(getenv("RACINE_KURI"), {});

    auto espace = compilatrice.espace_defaut_compilation();

    /* Charge d'abord le module basique, car nous en avons besoin pour le type ContexteProgramme.
     */
    compilatrice.importe_module(espace, "Kuri", {});

    /* Ne nomme pas le module, car c'est le module racine. */
    auto module = compilatrice.trouve_ou_crée_module(ID::chaine_vide, "");
    auto résultat = compilatrice.trouve_ou_crée_fichier(module, "", "", false);
    auto fichier = static_cast<Fichier *>(std::get<FichierNeuf>(résultat));
    fichier->charge_tampon(std::move(tampon));

    compilatrice.gestionnaire_code->requiers_lexage(espace, fichier);

    auto tacheronne = Tacheronne(compilatrice);
    tacheronne.gère_tâche();

    kuri::chemin_systeme::change_chemin_courant(chemin_courant);
    return compilatrice.code_erreur();
}

static auto decoupe_tampon(lng::tampon_source const &tampon)
{
    kuri::tableau<lng::tampon_source> résultat;

    auto debut_cas = 0ul;
    auto fin_cas = 1ul;

    for (auto i = 0ul; i < tampon.nombre_lignes(); ++i) {
        auto ligne = dls::chaine(tampon[static_cast<int64_t>(i)]);

        if (ligne.sous_chaine(0, 8) == "// Cas :") {
            debut_cas = i + 1;
        }

        if (ligne.sous_chaine(0, 8) == "// -----") {
            fin_cas = i;

            if (debut_cas < fin_cas) {
                auto sous_tampon = tampon.sous_tampon(debut_cas, fin_cas);
                résultat.ajoute(sous_tampon);
            }
        }
    }

    fin_cas = static_cast<size_t>(tampon.nombre_lignes());

    if (debut_cas < fin_cas) {
        auto sous_tampon = tampon.sous_tampon(debut_cas, fin_cas);
        résultat.ajoute(sous_tampon);
    }

    return résultat;
}

enum {
    ECHEC_CAR_CRASH,
    ECHEC_POUR_RAISON_INCONNUE,
    ECHEC_CAR_MAUVAIS_CODE_ERREUR,
    ECHEC_CAR_BOUCLE_INFINIE,
};

struct ResultatTest {
    dls::chaine fichier_origine{};
    kuri::chemin_systeme chemin_fichier{};
    int raison_echec{};
    erreur::Genre erreur_attendue{};
    erreur::Genre erreur_recue{};
};

static auto ecris_fichier_tmp(dls::chaine const &source, int index)
{
    auto nom_fichier = enchaine("echec_test", index, ".kuri");
    auto chemin_fichier = kuri::chemin_systeme::chemin_temporaire(nom_fichier);

    std::ofstream of;
    of.open(vers_std_path(chemin_fichier));
    of.write(source.c_str(), source.taille());

    return chemin_fichier;
}

int main()
{
    auto test_passes = 0;
    auto test_echoues = 0;

    auto résultats_tests = kuri::tableau<ResultatTest>();

    POUR (tests_unitaires) {
        auto chemin = kuri::chemin_systeme("fichiers_tests/") / it.source;

        if (kuri::chemin_systeme::existe(chemin)) {
            auto compilatrice = Compilatrice("", {});
            auto contenu_fichier = charge_contenu_fichier({chemin.pointeur(), chemin.taille()});
            auto tampon = lng::tampon_source(std::move(contenu_fichier));

            if (tampon.nombre_lignes() == 0) {
                // std::cerr << "Le fichier " << chemin << " est vide\n";
                continue;
            }

            auto cas = decoupe_tampon(tampon);

            for (auto &c : cas) {
                auto pid = fork();

                if (pid == 0) {
                    auto res = lance_test(c);
                    return static_cast<int>(res);
                }
                else if (pid > 0) {
                    auto debut = dls::chrono::compte_seconde();

                    while (true) {
                        int status;
                        pid_t result = waitpid(pid, &status, WNOHANG);

                        if (result == 0) {
                            /* L'enfant est toujours en vie, continue. */
                        }
                        else if (result == -1) {
                            auto rt = ResultatTest();
                            rt.raison_echec = ECHEC_POUR_RAISON_INCONNUE;
                            rt.fichier_origine = it.source;
                            rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

                            résultats_tests.ajoute(rt);

                            test_echoues += 1;
                            break;
                        }
                        else {
                            if (!WIFEXITED(status)) {
                                auto rt = ResultatTest();
                                rt.raison_echec = ECHEC_CAR_CRASH;
                                rt.fichier_origine = it.source;
                                rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

                                résultats_tests.ajoute(rt);

                                test_echoues += 1;
                            }
                            else {
                                if (WEXITSTATUS(status) == static_cast<int>(it.résultat_attendu)) {
                                    test_passes += 1;
                                }
                                else {
                                    auto rt = ResultatTest();
                                    rt.erreur_recue = static_cast<erreur::Genre>(
                                        WEXITSTATUS(status));
                                    rt.erreur_attendue = it.résultat_attendu;
                                    rt.raison_echec = ECHEC_CAR_MAUVAIS_CODE_ERREUR;
                                    rt.fichier_origine = it.source;
                                    rt.chemin_fichier = ecris_fichier_tmp(c.chaine(),
                                                                          test_echoues);

                                    résultats_tests.ajoute(rt);

                                    test_echoues += 1;
                                }
                            }

                            break;
                        }

                        auto temps = debut.temps();

                        if (temps > 25.0) {
                            kill(pid, SIGKILL);

                            auto rt = ResultatTest();
                            rt.raison_echec = ECHEC_CAR_BOUCLE_INFINIE;
                            rt.fichier_origine = it.source;
                            rt.chemin_fichier = ecris_fichier_tmp(c.chaine(), test_echoues);

                            résultats_tests.ajoute(rt);

                            test_echoues += 1;
                            break;
                        }
                    }
                }
            }

            std::cout << '.' << std::flush;
        }
        else {
            continue;
        }
    }

    std::cout << '\n';

    POUR (résultats_tests) {
        std::cout << "----------------------------------------\n";

        switch (it.raison_echec) {
            case ECHEC_CAR_CRASH:
            {
                std::cout << "Test échoué à cause d'un crash\n";
                break;
            }
            case ECHEC_CAR_BOUCLE_INFINIE:
            {
                std::cout << "Test échoué à cause d'une boucle infinie\n";
                break;
            }
            case ECHEC_CAR_MAUVAIS_CODE_ERREUR:
            {
                std::cout << "Test échoué à cause d'un mauvais code erreur\n";
                std::cout << "-- attendu : " << erreur::chaine_erreur(it.erreur_attendue) << '\n';
                std::cout << "-- reçu    : " << erreur::chaine_erreur(it.erreur_recue) << '\n';
                break;
            }
            case ECHEC_POUR_RAISON_INCONNUE:
            {
                std::cout << "Test échoué pour une raison inconnue\n";
                break;
            }
        }

        std::cout << "Fichier d'origine : " << it.fichier_origine << '\n';
        std::cout << "Fichier source    : " << it.chemin_fichier << '\n';
    }

    std::cout << '\n';
    std::cout << "SUCCES (" << test_passes << ")\n";
    std::cout << "ÉCHECS (" << test_echoues << ")\n";
}
