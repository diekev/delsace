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

#pragma once

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/resultat.hh"
#include "biblinternes/structures/ensemblon.hh"
#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/structures/tablet.hh"
#include "biblinternes/structures/tuples.hh"

#include <mutex>

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/table_hachage.hh"
#include "structures/tableau.hh"

#include "lexemes.hh"

struct Compilatrice;
struct EspaceDeTravail;
struct Enchaineuse;
struct IdentifiantCode;
struct MetaProgramme;
struct Module;
struct NoeudBloc;
struct NoeudDeclaration;
struct NoeudDeclarationCorpsFonction;
struct Statistiques;

struct DonneesConstantesFichier {
    double temps_chargement = 0.0;
    double temps_decoupage = 0.0;
    double temps_tampon = 0.0;

    lng::tampon_source tampon{""};

    kuri::tableau<Lexeme, int> lexemes{};

    kuri::chaine nom{""};
    kuri::chaine chemin{""};

    long id = 0;

    std::mutex mutex{};
    bool fut_lexe = false;
    bool fut_charge = false;
    bool en_chargement = false;
    bool en_lexage = false;

    void charge_tampon(lng::tampon_source &&t)
    {
        tampon = std::move(t);
        fut_charge = true;
    }
};

struct Fichier {
    double temps_analyse = 0.0;

    DonneesConstantesFichier *donnees_constantes = nullptr;

    dls::ensemblon<Module *, 16> modules_importes{};

    bool fut_parse = false;

    Module *module = nullptr;
    MetaProgramme *metaprogramme_corps_texte = nullptr;

    Fichier() = default;

    explicit Fichier(DonneesConstantesFichier *dc) : donnees_constantes(dc)
    {
    }

    COPIE_CONSTRUCT(Fichier);

    /**
     * Retourne vrai si le fichier importe un module du nom spécifié.
     */
    bool importe_module(IdentifiantCode *nom_module) const;

    kuri::chaine const &chemin() const
    {
        return donnees_constantes->chemin;
    }

    kuri::chaine const &nom() const
    {
        return donnees_constantes->nom;
    }

    long id() const
    {
        return donnees_constantes->id;
    }

    lng::tampon_source const &tampon() const
    {
        return donnees_constantes->tampon;
    }
};

template <int i>
struct EnveloppeFichier {
    Fichier *fichier = nullptr;

    EnveloppeFichier(Fichier &f) : fichier(&f)
    {
    }
};

using FichierExistant = EnveloppeFichier<0>;
using FichierNeuf = EnveloppeFichier<1>;

enum class TagPourResultatFichier {
    INVALIDE,
    NOUVEAU_FICHIER,
    FICHIER_EXISTANT,
};

template <>
struct tag_pour_donnees<TagPourResultatFichier, FichierExistant> {
    static constexpr auto tag = TagPourResultatFichier::FICHIER_EXISTANT;
};

template <>
struct tag_pour_donnees<TagPourResultatFichier, FichierNeuf> {
    static constexpr auto tag = TagPourResultatFichier::NOUVEAU_FICHIER;
};

using ResultatFichier = Resultat<FichierExistant, FichierNeuf, TagPourResultatFichier>;

struct DonneesConstantesModule {
    /* le nom du module, qui est le nom du dossier où se trouve les fichiers */
    IdentifiantCode *nom = nullptr;
    kuri::chaine chemin{""};
};

struct Module {
    DonneesConstantesModule *donnees_constantes = nullptr;

    std::mutex mutex{};
    NoeudBloc *bloc = nullptr;

    dls::tablet<Fichier *, 16> fichiers{};
    bool importe = false;

    kuri::chaine chemin_bibliotheque_32bits{};
    kuri::chaine chemin_bibliotheque_64bits{};

    explicit Module(DonneesConstantesModule *dc) : donnees_constantes(dc)
    {
        chemin_bibliotheque_32bits = enchaine(chemin(), "/lib/i386-linux-gnu/");
        chemin_bibliotheque_64bits = enchaine(chemin(), "/lib/x86_64-linux-gnu/");
    }

    COPIE_CONSTRUCT(Module);

    kuri::chaine const &chemin() const
    {
        return donnees_constantes->chemin;
    }

    IdentifiantCode *const &nom() const
    {
        return donnees_constantes->nom;
    }
};

struct SystemeModule {
    tableau_page<DonneesConstantesModule> donnees_modules{};
    tableau_page<DonneesConstantesFichier> donnees_fichiers{};

    kuri::table_hachage<kuri::chaine_statique, DonneesConstantesFichier *> table_fichiers{};

    DonneesConstantesModule *trouve_ou_cree_module(IdentifiantCode *nom,
                                                   kuri::chaine_statique chemin);

    DonneesConstantesModule *cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin);

    DonneesConstantesFichier *trouve_ou_cree_fichier(kuri::chaine_statique nom,
                                                     kuri::chaine_statique chemin);

    DonneesConstantesFichier *cree_fichier(kuri::chaine_statique nom,
                                           kuri::chaine_statique chemin);

    void rassemble_stats(Statistiques &stats) const;

    long memoire_utilisee() const;
};

void imprime_ligne_avec_message(Enchaineuse &flux,
                                const Fichier *fichier,
                                Lexeme const *lexeme,
                                kuri::chaine_statique message);

void imprime_ligne_avec_message(Enchaineuse &flux,
                                const Fichier *fichier,
                                int ligne,
                                kuri::chaine_statique message);

/* Charge le contenu du fichier, c'est la responsabilité de l'appelant de vérifier que
 * le fichier existe bel et bien. */
dls::chaine charge_contenu_fichier(dls::chaine const &chemin);
