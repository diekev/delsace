/*
 * ***** BEGIN GPL LICENSE BLOCK *****
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software  Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "modules.hh"

#include <fstream>

#include "biblinternes/langage/erreur.hh"
#include "biblinternes/outils/numerique.hh"

#include "structures/enchaineuse.hh"

#include "statistiques/statistiques.hh"

/* ************************************************************************** */

bool Fichier::importe_module(IdentifiantCode *nom_module) const
{
    bool importe = false;
    pour_chaque_element(modules_importes, [nom_module, &importe](Module *module_) {
        if (module_->nom() == nom_module) {
            importe = true;
            return kuri::DecisionIteration::Arrete;
        }

        return kuri::DecisionIteration::Continue;
    });
    return importe;
}

Module *SystemeModule::trouve_ou_cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
    auto chemin_normalise = kuri::chaine(chemin);

    if (chemin_normalise.taille() > 0 && chemin_normalise[chemin_normalise.taille() - 1] != '/') {
        chemin_normalise = enchaine(chemin_normalise, '/');
    }

    POUR_TABLEAU_PAGE (modules) {
        if (it.chemin() == chemin_normalise) {
            return &it;
        }
    }

    return cree_module(nom, chemin_normalise);
}

Module *SystemeModule::cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin)
{
    auto dm = modules.ajoute_element(chemin);
    dm->nom_ = nom;
    return dm;
}

Module *SystemeModule::module(const IdentifiantCode *nom) const
{
    POUR_TABLEAU_PAGE (modules) {
        if (it.nom() == nom) {
            return const_cast<Module *>(&it);
        }
    }

    return nullptr;
}

ResultatFichier SystemeModule::trouve_ou_cree_fichier(Module *module,
                                                      kuri::chaine_statique nom,
                                                      kuri::chaine_statique chemin)
{
    auto fichier = table_fichiers.valeur_ou(chemin, nullptr);

    if (fichier) {
        return FichierExistant(*fichier);
    }

    return cree_fichier(module, nom, chemin);
}

FichierNeuf SystemeModule::cree_fichier(Module *module,
                                        kuri::chaine_statique nom,
                                        kuri::chaine_statique chemin)
{
    auto df = fichiers.ajoute_element();
    df->nom_ = nom;
    df->chemin_ = chemin;
    df->id_ = fichiers.taille() - 1;
    df->module = module;

    table_fichiers.insere(df->chemin(), df);

    return FichierNeuf(*df);
}

void SystemeModule::rassemble_stats(Statistiques &stats) const
{
    stats.nombre_modules = modules.taille();

    auto &stats_fichiers = stats.stats_fichiers;
    POUR_TABLEAU_PAGE (fichiers) {
        auto entree = EntreeFichier();
        entree.chemin = it.chemin();
        entree.nom = it.nom();
        entree.nombre_lignes = it.tampon().nombre_lignes();
        entree.memoire_tampons = it.tampon().taille_donnees();
        entree.memoire_lexemes = it.lexemes.taille() * taille_de(Lexeme);
        entree.nombre_lexemes = it.lexemes.taille();
        entree.temps_chargement = it.temps_chargement;
        entree.temps_tampon = it.temps_tampon;
        entree.temps_lexage = it.temps_decoupage;
        entree.temps_parsage = it.temps_analyse;

        stats_fichiers.fusionne_entree(entree);
    }
}

long SystemeModule::memoire_utilisee() const
{
    auto memoire = 0l;
    memoire += modules.memoire_utilisee();
    memoire += fichiers.memoire_utilisee();

    POUR_TABLEAU_PAGE (fichiers) {
        memoire += it.nom().taille();
        memoire += it.chemin().taille();
        memoire += it.tampon().chaine().taille();
        // les autres membres sont gérés dans rassemble_statistiques()
        if (!it.modules_importes.est_stocke_dans_classe()) {
            memoire += it.modules_importes.taille() * taille_de(dls::vue_chaine_compacte);
        }
    }

    POUR_TABLEAU_PAGE (modules) {
        memoire += it.chemin().taille();
        memoire += it.fichiers.taille() * taille_de(Fichier *);
    }

    return memoire;
}

Fichier *SystemeModule::fichier(kuri::chaine_statique chemin) const
{
    POUR_TABLEAU_PAGE (fichiers) {
        if (it.chemin() == chemin) {
            return const_cast<Fichier *>(&it);
        }
    }

    return nullptr;
}

void imprime_ligne_avec_message(Enchaineuse &flux,
                                const Fichier *fichier,
                                Lexeme const *lexeme,
                                kuri::chaine_statique message)
{
    flux << fichier->chemin() << ':' << lexeme->ligne + 1 << ':' << lexeme->colonne + 1 << " : ";
    flux << message << "\n";

    auto nc = dls::num::nombre_de_chiffres(lexeme->ligne + 1);

    for (auto i = 0; i < 5 - nc; ++i) {
        flux << ' ';
    }

    flux << lexeme->ligne + 1 << " | " << fichier->tampon()[lexeme->ligne];
    flux << "      | ";

    lng::erreur::imprime_caractere_vide(flux, lexeme->colonne, fichier->tampon()[lexeme->ligne]);
    flux << '^';
    lng::erreur::imprime_tilde(flux, lexeme->chaine);
    flux << '\n';
}

void imprime_ligne_avec_message(Enchaineuse &flux,
                                const Fichier *fichier,
                                int ligne,
                                kuri::chaine_statique message)
{
    flux << fichier->chemin() << ':' << ligne << " : ";
    flux << message << "\n";

    auto nc = dls::num::nombre_de_chiffres(ligne);

    for (auto i = 0; i < 5 - nc; ++i) {
        flux << ' ';
    }

    flux << ligne << " | " << fichier->tampon()[ligne - 1];
}

dls::chaine charge_contenu_fichier(const dls::chaine &chemin)
{
    std::ifstream fichier;
    fichier.open(chemin.c_str());

    if (!fichier.is_open()) {
        return "";
    }

    return dls::chaine(std::istreambuf_iterator<char>(fichier), std::istreambuf_iterator<char>());
}
