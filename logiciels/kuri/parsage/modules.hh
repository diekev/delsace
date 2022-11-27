/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/resultat.hh"
#include "biblinternes/structures/tableau_page.hh"
#include "biblinternes/structures/tuples.hh"

#include <mutex>

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/ensemblon.hh"
#include "structures/table_hachage.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

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
struct NoeudDirectivePreExecutable;
struct SiteSource;
struct Statistiques;

enum class SourceFichier : unsigned char {
    /* Le fichier vient du disque dur (d'une instruction « importe » ou « charge ». */
    DISQUE,
    /* Le fichier vient d'une instruction "ajoute_chaine_*". */
    CHAINE_AJOUTEE,
};

struct Fichier {
    double temps_analyse = 0.0;
    double temps_chargement = 0.0;
    double temps_decoupage = 0.0;
    double temps_tampon = 0.0;

    lng::tampon_source tampon_{""};

    kuri::tableau<Lexeme, int> lexemes{};

    kuri::chaine nom_{""};
    kuri::chaine chemin_{""};

    long id_ = 0;

    std::mutex mutex{};

    SourceFichier source = SourceFichier::DISQUE;
    bool fut_lexe = false;
    bool fut_charge = false;
    bool en_chargement = false;
    bool en_lexage = false;
    bool fut_parse = false;

    kuri::ensemblon<Module *, 16> modules_importes{};

    Module *module = nullptr;
    MetaProgramme *metaprogramme_corps_texte = nullptr;

    /* Pour les fichier venant de CHAINE_AJOUTEE, le décalage dans le fichier final. */
    long decalage_fichier = 0;

    Fichier() = default;

    EMPECHE_COPIE(Fichier);

    /**
     * Retourne vrai si le fichier importe un module du nom spécifié.
     */
    bool importe_module(IdentifiantCode *nom_module) const;

    kuri::chaine const &chemin() const
    {
        return chemin_;
    }

    kuri::chaine const &nom() const
    {
        return nom_;
    }

    long id() const
    {
        return id_;
    }

    lng::tampon_source const &tampon() const
    {
        return tampon_;
    }

    void charge_tampon(lng::tampon_source &&t)
    {
        tampon_ = std::move(t);
        fut_charge = true;
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

struct Module {
    /* le nom du module, qui est le nom du dossier où se trouve les fichiers */
    IdentifiantCode *nom_ = nullptr;

    kuri::chaine chemin_{""};
    kuri::chaine chemin_bibliotheque_32bits{};
    kuri::chaine chemin_bibliotheque_64bits{};

    std::mutex mutex{};
    NoeudBloc *bloc = nullptr;

    kuri::tablet<Fichier *, 16> fichiers{};
    bool importe = false;

    /* Pour le #GestionnaireCode afin de savoir si nous devons vérifier qu'il reste des fichiers à
     * parser. */
    bool fichiers_sont_sales = true;
    bool execution_directive_requise = false;

    NoeudDirectivePreExecutable *directive_pre_executable = nullptr;

    Module(kuri::chaine chm) : chemin_(chm)
    {
        chemin_bibliotheque_32bits = enchaine(chemin(), "/lib/i386-linux-gnu/");
        chemin_bibliotheque_64bits = enchaine(chemin(), "/lib/x86_64-linux-gnu/");
    }

    EMPECHE_COPIE(Module);

    void ajoute_fichier(Fichier *fichier);

    kuri::chaine_statique chemin() const
    {
        return chemin_;
    }

    IdentifiantCode *const &nom() const
    {
        return nom_;
    }
};

struct SystemeModule {
    tableau_page<Module> modules{};
    tableau_page<Fichier> fichiers{};

    kuri::table_hachage<kuri::chaine_statique, Fichier *> table_fichiers{
        "Fichiers système modules"};

    Module *trouve_ou_cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin);

    Module *cree_module(IdentifiantCode *nom, kuri::chaine_statique chemin);

    Module *module(const IdentifiantCode *nom) const;

    ResultatFichier trouve_ou_cree_fichier(Module *module,
                                           kuri::chaine_statique nom,
                                           kuri::chaine_statique chemin);

    FichierNeuf cree_fichier(Module *module,
                             kuri::chaine_statique nom,
                             kuri::chaine_statique chemin);

    void rassemble_stats(Statistiques &stats) const;

    long memoire_utilisee() const;

    Fichier *fichier(long index)
    {
        return &fichiers.a_l_index(index);
    }

    const Fichier *fichier(long index) const
    {
        return &fichiers.a_l_index(index);
    }

    Fichier *fichier(kuri::chaine_statique chemin) const;
};

void imprime_ligne_avec_message(Enchaineuse &flux, SiteSource site, kuri::chaine_statique message);

/* Charge le contenu du fichier, c'est la responsabilité de l'appelant de vérifier que
 * le fichier existe bel et bien. */
dls::chaine charge_contenu_fichier(dls::chaine const &chemin);
