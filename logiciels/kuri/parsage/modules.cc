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

#include "lexeuse.hh"

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

static dls::vue_chaine sous_chaine(dls::vue_chaine chaine, int debut, int fin)
{
    return {&chaine[debut], fin - debut};
}

void imprime_ligne_avec_message(Enchaineuse &enchaineuse,
                                SiteSource site,
                                kuri::chaine_statique message)
{
    auto const fichier = site.fichier;

    if (fichier == nullptr) {
        /* Il est possible que le fichier soit nul dans certains cas, par exemple quand une erreur
         * est rapportée sans site. */
        return;
    }

    auto const index_ligne = site.index_ligne;
    auto const numero_ligne = site.index_ligne + 1;
    auto const index_colonne = site.index_colonne;
    auto const texte_ligne = fichier->tampon()[index_ligne];

    enchaineuse << fichier->chemin() << ':' << numero_ligne << ':' << index_colonne << " : ";
    enchaineuse << message << "\n";

    for (auto i = 0; i < 5 - dls::num::nombre_de_chiffres(numero_ligne); ++i) {
        enchaineuse << ' ';
    }

    enchaineuse << numero_ligne << " | " << texte_ligne;

    if (index_colonne != -1) {
        enchaineuse << "      | ";

        if (site.index_colonne_min != -1 && site.index_colonne_min != site.index_colonne) {
            lng::erreur::imprime_caractere_vide(enchaineuse, site.index_colonne_min, texte_ligne);
            lng::erreur::imprime_tilde(
                enchaineuse, sous_chaine(texte_ligne, site.index_colonne_min, index_colonne + 1));
        }
        else {
            lng::erreur::imprime_caractere_vide(enchaineuse, index_colonne, texte_ligne);
        }

        enchaineuse << '^';
        lng::erreur::imprime_tilde(
            enchaineuse, sous_chaine(texte_ligne, index_colonne, site.index_colonne_max));
        enchaineuse << '\n';
    }
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
