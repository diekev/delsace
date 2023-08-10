/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "biblinternes/langage/tampon_source.hh"
#include "biblinternes/outils/definitions.h"
#include "biblinternes/outils/resultat.hh"
#include "biblinternes/structures/tableau_page.hh"

#include <mutex>

#include "structures/chaine.hh"
#include "structures/chemin_systeme.hh"
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
struct NoeudExpression;
struct SiteSource;
struct Statistiques;

enum class SourceFichier : unsigned char {
    /* Le fichier vient du disque dur (d'une instruction « importe » ou « charge ». */
    DISQUE,
    /* Le fichier vient d'une instruction "ajoute_chaine_*". */
    CHAINE_AJOUTEE,
};

/* Énum drapeau pour définir quelles fonctionnalités du langage sont présentes dans un fichier afin
 * d'aider à diriger la compilation du fichier. */
enum class FonctionnalitéLangage : uint16_t {
    /* Le fichier contient des instructions de chargement de fichiers. */
    CHARGE = (1 << 0),
    /* Le fichier contient des imports. */
    IMPORTE = (1 << 1),
    /* Le fichier contient des directives #exécute. */
    EXÉCUTE = (1 << 2),
    /* Le fichier contient des directives #ajoute_init. */
    AJOUTE_INIT = (1 << 3),
    /* Le fichier contient des directives #ajoute_fini. */
    AJOUTE_FINI = (1 << 4),
    /* Le fichier contient des directives #assert. */
    ASSERT = (1 << 5),
    /* Le fichier contient des directives #test. */
    TEST = (1 << 6),
    /* Le fichier contient des directives #si. */
    SI_STATIQUE = (1 << 7),
    /* Le fichier contient des directives #cuisine. */
    CUISINE = (1 << 8),
    /* Le fichier contient des directives #pré_exécutable. */
    PRÉ_EXÉCUTABLE = (1 << 9),
};
DEFINIS_OPERATEURS_DRAPEAU(FonctionnalitéLangage)

struct Fichier {
    double temps_analyse = 0.0;
    double temps_chargement = 0.0;
    double temps_decoupage = 0.0;
    double temps_tampon = 0.0;

    lng::tampon_source tampon_{""};

    kuri::tableau<Lexeme, int> lexemes{};

    kuri::chaine nom_{""};
    kuri::chaine chemin_{""};

    int64_t id_ = 0;

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
    int64_t decalage_fichier = 0;

    /* Mis en place par la Syntaxeuse, c'est la liste de toutes les déclarations dans le bloc
     * global du module à valider impérativement (sans attendre que quelque chose d'autre le
     * requiers). Nous avons ça car les fichiers n'ont pas de blocs propres alors que le
     * GestionnaireCode doit savoir pour chaque fichier quels sont les noeuds à valider
     * impérativement. */
    kuri::tableau<NoeudExpression *, int> noeuds_à_valider{};

    FonctionnalitéLangage fonctionnalités_utilisées{};

    Fichier() = default;

    EMPECHE_COPIE(Fichier);

    /**
     * Retourne vrai si le fichier importe un module du nom spécifié.
     */
    bool importe_module(IdentifiantCode *nom_module) const;

    kuri::chaine_statique chemin() const
    {
        return chemin_;
    }

    kuri::chaine_statique nom() const
    {
        return nom_;
    }

    int64_t id() const
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

    kuri::chemin_systeme chemin_{""};
    kuri::chemin_systeme chemin_bibliotheque_32bits{};
    kuri::chemin_systeme chemin_bibliotheque_64bits{};

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

    int64_t memoire_utilisee() const;

    Fichier *fichier(int64_t index)
    {
        return &fichiers.a_l_index(index);
    }

    const Fichier *fichier(int64_t index) const
    {
        return &fichiers.a_l_index(index);
    }

    Fichier *fichier(kuri::chaine_statique chemin) const;
};

void imprime_ligne_avec_message(Enchaineuse &flux, SiteSource site, kuri::chaine_statique message);

/* Charge le contenu du fichier, c'est la responsabilité de l'appelant de vérifier que
 * le fichier existe bel et bien. */
dls::chaine charge_contenu_fichier(dls::chaine const &chemin);
