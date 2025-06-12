/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#include <fstream>
#include <iostream>

#include "biblinternes/json/json.hh"

#include "compilation/compilatrice.hh"
#include "compilation/erreur.h"

#include "parsage/lexeuse.hh"
#include "parsage/modules.hh"
#include "parsage/outils_lexemes.hh"

#include "options.hh"

#include "structures/chemin_systeme.hh"

/* petit outil pour remplacer des mot-clés dans les scripts préexistants afin de
 * faciliter l'évolution du langage
 * nous pourrions également avoir un sorte de fichier de configuration pour
 * définir comment changer les choses, ou formater le code
 */

struct Configuration {
    kuri::tableau<dls::chaine> dossiers{};
    kuri::tableau<dls::chaine> fichiers{};

    using paire = std::pair<dls::chaine, dls::chaine>;

    kuri::tableau<paire> mots_cles{};
};

static void analyse_liste_chemin(tori::ObjetTableau *tableau, kuri::tableau<dls::chaine> &chaines)
{
    for (auto objet : tableau->valeur) {
        if (objet->type != tori::type_objet::CHAINE) {
            std::cerr << "liste : l'objet n'est pas une chaine !\n";
            continue;
        }

        auto obj_chaine = extrait_chaine(objet.get());
        chaines.ajoute(obj_chaine->valeur);
    }
}

static Configuration analyse_configuration(const char *chemin)
{
    auto config = Configuration{};
    auto obj = json::compile_script(chemin);

    if (obj == nullptr) {
        std::cerr << "La compilation du script a renvoyé un objet nul !\n";
        return config;
    }

    if (obj->type != tori::type_objet::DICTIONNAIRE) {
        std::cerr << "La compilation du script n'a pas produit de dictionnaire !\n";
        return config;
    }

    auto dico = tori::extrait_dictionnaire(obj.get());

    auto obj_fichiers = cherche_tableau(dico, "fichiers");

    if (obj_fichiers != nullptr) {
        analyse_liste_chemin(obj_fichiers, config.fichiers);
    }

    auto obj_dossiers = cherche_tableau(dico, "dossiers");

    if (obj_dossiers != nullptr) {
        analyse_liste_chemin(obj_dossiers, config.dossiers);
    }

    auto obj_change = cherche_dico(dico, "change");

    if (obj_change != nullptr) {
        auto obj_mots_cles = cherche_dico(obj_change, "mots-clés");

        if (obj_mots_cles == nullptr) {
            return config;
        }

        for (auto objet : obj_mots_cles->valeur) {
            auto const &nom_objet = objet.first;

            if (objet.second->type != tori::type_objet::CHAINE) {
                std::cerr << "mots-clés : la valeur l'objet '" << nom_objet
                          << "' n'est pas une chaine !\n";
                continue;
            }

            auto obj_chaine = extrait_chaine(objet.second.get());
            config.mots_cles.ajoute({nom_objet, obj_chaine->valeur});

            // std::cerr << "Remplacement de '" << nom_objet << "' par '" << obj_chaine->valeur <<
            // "'\n";
        }
    }

    return config;
}

static void reecris_fichier(kuri::chemin_systeme &chemin, Configuration const &config)
{
    std::cerr << "Réécriture du fichier " << chemin << "\n";

    {
        chemin = kuri::chemin_systeme::absolu(chemin);

        auto compilatrice = Compilatrice("", {});
        auto donnees_fichier = Fichier();
        auto tampon = charge_contenu_fichier({chemin.pointeur(), chemin.taille()});
        donnees_fichier.charge_tampon(TamponSource(std::move(tampon)));

        auto lexeuse = Lexeuse(compilatrice.contexte_lexage(nullptr),
                               &donnees_fichier,
                               INCLUS_CARACTERES_BLANC | INCLUS_COMMENTAIRES);
        lexeuse.performe_lexage();

        auto os = std::ofstream(vers_std_path(chemin));

        for (auto const &lexeme : donnees_fichier.lexèmes) {
            if (!est_mot_clé(lexeme.genre)) {
                os << lexeme.chaine;
                continue;
            }

            auto trouve = false;

            for (auto const &paire : config.mots_cles) {
                if (paire.first != dls::chaine(lexeme.chaine.pointeur(), lexeme.chaine.taille())) {
                    continue;
                }

                os << paire.second;
                trouve = true;

                break;
            }

            if (!trouve) {
                os << lexeme.chaine;
            }
        }
    }
}

int main(int argc, char **argv)
{
    std::ios::sync_with_stdio(false);

    if (argc < 2) {
        std::cerr << "Utilisation " << argv[0] << " CONFIG.json\n";
        return 1;
    }

    auto config = analyse_configuration(argv[1]);

    if (config.dossiers.est_vide() && config.fichiers.est_vide()) {
        std::cerr << "Aucun fichier ni dossier précisé dans le fichier de configuration !\n";
        return 1;
    }

    if (config.mots_cles.est_vide()) {
        std::cerr << "Aucun mot-clé précisé !\n";
        return 1;
    }

    auto const &chemin_racine_kuri = getenv("RACINE_KURI");

    if (chemin_racine_kuri == nullptr) {
        std::cerr << "Impossible de trouver le chemin racine de l'installation de kuri !\n";
        std::cerr << "Possible solution : veuillez faire en sorte que la variable d'environnement "
                     "'RACINE_KURI' soit définie !\n";
        return 1;
    }

    for (auto const &chemin_dossier : config.dossiers) {
        auto chemin = kuri::chemin_systeme(chemin_dossier.c_str());

        if (!kuri::chemin_systeme::existe(chemin)) {
            std::cerr << "Le chemin " << chemin << " ne pointe vers rien !\n";
            continue;
        }

        if (!kuri::chemin_systeme::est_dossier(chemin)) {
            std::cerr << "Le chemin " << chemin << " ne pointe pas vers un dossier !\n";
            continue;
        }

        for (auto donnees : kuri::chemin_systeme::fichiers_du_dossier_recursif(chemin)) {
            if (donnees.extension() != ".kuri") {
                continue;
            }

            reecris_fichier(donnees, config);
        }
    }

    for (auto const &chemin_fichier : config.fichiers) {
        auto chemin = kuri::chemin_systeme(chemin_fichier.c_str());

        if (!kuri::chemin_systeme::existe(chemin)) {
            std::cerr << "Le chemin " << chemin << " ne pointe vers rien !\n";
            continue;
        }

        if (!kuri::chemin_systeme::est_fichier_regulier(chemin)) {
            std::cerr << "Le chemin " << chemin << " ne pointe pas vers un fichier !\n";
            continue;
        }

        if (chemin.extension() != ".kuri") {
            std::cerr << "Le chemin " << chemin << " ne pointe pas vers un fichier kuri !\n";
            continue;
        }

        reecris_fichier(chemin, config);
    }

    return 0;
}
