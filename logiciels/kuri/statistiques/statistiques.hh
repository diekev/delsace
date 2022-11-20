/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/chrono/outils.hh"

#include "structures/chaine.hh"
#include "structures/tableau.hh"

#undef STATISTIQUES_DETAILLEES

#ifdef STATISTIQUES_DETAILLEES
#    define CHRONO_TYPAGE(entree_stats, index)                                                    \
        dls::chrono::chrono_rappel_milliseconde VARIABLE_ANONYME(chrono)([&](double temps) {      \
            entree_stats.fusionne_entree(index, {"", temps});                                     \
        })
#else
#    define CHRONO_TYPAGE(entree_stats, nom)
#endif

#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename T>
concept TypeEntreesStats = requires(T a, T b)
{
    a += b;
};
#else
#    define TypeEntreesStats typename
#endif

struct EntreeNombreMemoire {
    kuri::chaine nom = "";
    long compte = 0;
    long memoire = 0;

    EntreeNombreMemoire &operator+=(EntreeNombreMemoire const &autre)
    {
        compte += autre.compte;
        memoire += autre.memoire;
        return *this;
    }

    bool peut_fusionner_avec(EntreeNombreMemoire const &autre) const
    {
        return autre.nom == nom;
    }
};

struct EntreeTaille {
    kuri::chaine nom = "";
    long taille = 0;

    EntreeTaille &operator+=(EntreeTaille const &autre)
    {
        taille = std::max(taille, autre.taille);
        return *this;
    }

    bool peut_fusionner_avec(EntreeTaille const &autre) const
    {
        return autre.nom == nom;
    }
};

struct EntreeFichier {
    kuri::chaine chemin = "";
    kuri::chaine nom = "";
    long memoire_lexemes = 0;
    long nombre_lexemes = 0;
    long nombre_lignes = 0;
    long memoire_tampons = 0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_chargement = 0.0;
    double temps_tampon = 0.0;

    EntreeFichier &operator+=(EntreeFichier const &autre)
    {
        memoire_lexemes += autre.memoire_lexemes;
        nombre_lignes += autre.nombre_lignes;
        temps_lexage += autre.temps_lexage;
        temps_parsage += autre.temps_parsage;
        temps_chargement += autre.temps_chargement;
        temps_tampon += autre.temps_tampon;
        nombre_lexemes += autre.nombre_lexemes;
        memoire_tampons += autre.memoire_tampons;
        return *this;
    }

    bool peut_fusionner_avec(EntreeFichier const &autre) const
    {
        return autre.chemin == chemin;
    }
};

struct EntreeTemps {
    const char *nom{};
    double temps = 0.0;

    EntreeTemps &operator+=(EntreeTemps const &autre)
    {
        temps += autre.temps;
        return *this;
    }

    bool peut_fusionner_avec(EntreeTemps const &autre) const
    {
        return autre.nom == nom;
    }
};

template <TypeEntreesStats T>
struct EntreesStats {
    kuri::chaine nom{};
    kuri::tableau<T, int> entrees{};
    T totaux{};

    explicit EntreesStats(kuri::chaine const &nom_) : nom(nom_)
    {
    }

    EntreesStats(kuri::chaine const &nom_, std::initializer_list<const char *> &&noms_entrees)
        : nom(nom_)
    {
        entrees.reserve(static_cast<int>(noms_entrees.size()));
        for (auto nom_entree : noms_entrees) {
            T e;
            e.nom = nom_entree;
            entrees.ajoute(e);
        }
    }

    void ajoute_entree(T const &entree)
    {
        totaux += entree;
        entrees.ajoute(entree);
    }

    void fusionne_entree(T const &entree)
    {
        totaux += entree;

        for (auto &e : entrees) {
            if (e.peut_fusionner_avec(entree)) {
                e += entree;
                return;
            }
        }

        entrees.ajoute(entree);
    }

    void fusionne_entree(int index, T const &entree)
    {
        totaux += entree;
        entrees[index] += entree;
    }
};

using StatistiquesFichiers = EntreesStats<EntreeFichier>;
using StatistiquesArbre = EntreesStats<EntreeNombreMemoire>;
using StatistiquesGraphe = EntreesStats<EntreeNombreMemoire>;
using StatistiquesTypes = EntreesStats<EntreeNombreMemoire>;
using StatistiquesOperateurs = EntreesStats<EntreeNombreMemoire>;
using StatistiquesNoeudCode = EntreesStats<EntreeNombreMemoire>;
using StatistiquesMessage = EntreesStats<EntreeNombreMemoire>;
using StatistiquesRI = EntreesStats<EntreeNombreMemoire>;
using StatistiquesTableaux = EntreesStats<EntreeTaille>;

struct Statistiques {
    long nombre_modules = 0l;
    long nombre_identifiants = 0l;
    long nombre_metaprogrammes_executes = 0l;
    long memoire_compilatrice = 0l;
    long memoire_ri = 0l;
    long memoire_mv = 0l;
    long memoire_bibliotheques = 0l;
    long instructions_executees = 0l;
    double temps_generation_code = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_executable = 0.0;
    double temps_nettoyage = 0.0;
    double temps_ri = 0.0;
    double temps_metaprogrammes = 0.0;
    double temps_scene = 0.0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_typage = 0.0;
    double temps_chargement = 0.0;
    double temps_tampons = 0.0;

    StatistiquesFichiers stats_fichiers{"Fichiers"};
    StatistiquesArbre stats_arbre{"Arbre Syntaxique"};
    StatistiquesGraphe stats_graphe_dependance{"Graphe Dépendances"};
    StatistiquesTypes stats_types{"Types"};
    StatistiquesOperateurs stats_operateurs{"Opérateurs"};
    StatistiquesNoeudCode stats_noeuds_code{"Noeuds Code"};
    StatistiquesMessage stats_messages{"Messages"};
    StatistiquesRI stats_ri{"Représentation Intermédiaire"};
    StatistiquesTableaux stats_tableaux{"Tableaux"};
};

#define EXTRAIT_CHAINE_ENUM(ident, chaine) chaine,
#define EXTRAIT_IDENT_ENUM(ident, chaine) ident,

#define INIT_NOMS_ENTREES_IMPL(MACRO)                                                             \
    {                                                                                             \
        MACRO(EXTRAIT_CHAINE_ENUM)                                                                \
    }
#define INIT_NOMS_ENTREES(NOM) INIT_NOMS_ENTREES_IMPL(ENTREES_POUR_##NOM)

#define DEFINIS_ENUM_IMPL(NOM, MACRO) enum { MACRO(EXTRAIT_IDENT_ENUM) NOM##__COMPTE };

#define DEFINIS_ENUM(NOM) DEFINIS_ENUM_IMPL(NOM, ENTREES_POUR_##NOM)

#define ENTREES_POUR_VALIDATION_APPEL(OP)                                                         \
    OP(VALIDATION_APPEL__PREPARE_ARGUMENTS, "prépare arguments")                                  \
    OP(VALIDATION_APPEL__TROUVE_CANDIDATES, "trouve candidates")                                  \
    OP(VALIDATION_APPEL__APPARIE_CANDIDATES, "apparie candidates")                                \
    OP(VALIDATION_APPEL__COPIE_DONNEES, "copie données")

#define ENTREES_POUR_FINALISATION(OP) OP(FINALISATION__FINALISATION, "finalisation")

#define ENTREES_POUR_OPERATEUR_UNAIRE(OP)                                                         \
    OP(OPERATEUR_UNAIRE__TYPE, "opérateur unaire type")                                           \
    OP(OPERATEUR_UNAIRE__TYPE_DE_DONNEES, "opérateur unaire type (type_type_de_donnees)")         \
    OP(OPERATEUR_UNAIRE__POINTEUR, "opérateur unaire type (pointeur)")                            \
    OP(OPERATEUR_UNAIRE__REFERENCE, "opérateur unaire type (reference)")                          \
    OP(OPERATEUR_UNAIRE__OPERATEUR_UNAIRE, "opérateur unaire")

#define ENTREES_POUR_OPERATEUR_BINAIRE(OP) OP(OPERATEUR_BINAIRE__VALIDATION, "validation")

#define ENTREES_POUR_DECLARATION_VARIABLES(OP)                                                    \
    OP(DECLARATION_VARIABLES__RASSEMBLE_VARIABLES, "rassemble variables")                         \
    OP(DECLARATION_VARIABLES__RASSEMBLE_EXPRESSIONS, "rassemble expressions")                     \
    OP(DECLARATION_VARIABLES__ENFILE_VARIABLES, "enfile variables")                               \
    OP(DECLARATION_VARIABLES__ASSIGNATION_EXPRESSIONS, "assignation expressions")                 \
    OP(DECLARATION_VARIABLES__PREPARATION, "préparation")                                         \
    OP(DECLARATION_VARIABLES__REDEFINITION, "redéfinition")                                       \
    OP(DECLARATION_VARIABLES__RESOLUTION_TYPE, "résolution type")                                 \
    OP(DECLARATION_VARIABLES__VALIDATION_FINALE, "validation finale")                             \
    OP(DECLARATION_VARIABLES__COPIE_DONNEES, "copie données")

#define ENTREES_POUR_REFERENCE_DECLARATION(OP)                                                    \
    OP(REFERENCE_DECLARATION__VALIDATION, "validation")                                           \
    OP(REFERENCE_DECLARATION__TYPE_DE_DONNES, "type de données")

#define ENTREES_POUR_ENTETE_FONCTION(OP)                                                          \
    OP(ENTETE_FONCTION__TENTATIVES_RATEES, "tentatives râtés")                                    \
    OP(ENTETE_FONCTION__ENTETE_FONCTION, "valide_entete_fonction")                                \
    OP(ENTETE_FONCTION__ARBRE_APLATIS, "arbre aplatis")                                           \
    OP(ENTETE_FONCTION__TYPES_OPERATEURS, "types opérateurs")                                     \
    OP(ENTETE_FONCTION__PARAMETRES, "paramètres")                                                 \
    OP(ENTETE_FONCTION__TYPES_PARAMETRES, "types paramètres")                                     \
    OP(ENTETE_FONCTION__TYPES_FONCTION, "type fonction")                                          \
    OP(ENTETE_FONCTION__REDEFINITION, "redéfinition fonction")                                    \
    OP(ENTETE_FONCTION__REDEFINITION_OPERATEUR, "redéfinition opérateur")

#define ENTREES_POUR_CORPS_FONCTION(OP) OP(CORPS_FONCTION__VALIDATION, "validation")

#define ENTREES_POUR_ENUMERATION(OP) OP(ENUMERATION__VALIDATION, "validation")

#define ENTREES_POUR_STRUCTURE(OP) OP(STRUCTURE__VALIDATION, "validation")

#define ENTREES_POUR_ASSIGNATION(OP) OP(ASSIGNATION__VALIDATION, "validation")

DEFINIS_ENUM(VALIDATION_APPEL)
DEFINIS_ENUM(FINALISATION)
DEFINIS_ENUM(OPERATEUR_UNAIRE)
DEFINIS_ENUM(OPERATEUR_BINAIRE)
DEFINIS_ENUM(DECLARATION_VARIABLES)
DEFINIS_ENUM(REFERENCE_DECLARATION)
DEFINIS_ENUM(ENTETE_FONCTION)
DEFINIS_ENUM(CORPS_FONCTION)
DEFINIS_ENUM(ENUMERATION)
DEFINIS_ENUM(STRUCTURE)
DEFINIS_ENUM(ASSIGNATION)

struct StatistiquesTypage {
    EntreesStats<EntreeTemps> validation_decl{"Déclarations Variables",
                                              INIT_NOMS_ENTREES(DECLARATION_VARIABLES)};
    EntreesStats<EntreeTemps> validation_appel{"Appels", INIT_NOMS_ENTREES(VALIDATION_APPEL)};
    EntreesStats<EntreeTemps> ref_decl{"Références Déclarations",
                                       INIT_NOMS_ENTREES(REFERENCE_DECLARATION)};
    EntreesStats<EntreeTemps> operateurs_unaire{"Opérateurs Unaire",
                                                INIT_NOMS_ENTREES(OPERATEUR_UNAIRE)};
    EntreesStats<EntreeTemps> operateurs_binaire{"Opérateurs Binaire",
                                                 INIT_NOMS_ENTREES(OPERATEUR_BINAIRE)};
    EntreesStats<EntreeTemps> entetes_fonctions{"Entêtes Fonctions",
                                                INIT_NOMS_ENTREES(ENTETE_FONCTION)};
    EntreesStats<EntreeTemps> corps_fonctions{"Corps Fonctions",
                                              INIT_NOMS_ENTREES(CORPS_FONCTION)};
    EntreesStats<EntreeTemps> enumerations{"Énumérations", INIT_NOMS_ENTREES(ENUMERATION)};
    EntreesStats<EntreeTemps> structures{"Structures", INIT_NOMS_ENTREES(STRUCTURE)};
    EntreesStats<EntreeTemps> assignations{"Assignations", INIT_NOMS_ENTREES(ASSIGNATION)};
    EntreesStats<EntreeTemps> finalisation{"Finalisation", INIT_NOMS_ENTREES(FINALISATION)};

    void imprime_stats();
};

void imprime_stats(Statistiques const &stats, dls::chrono::compte_seconde debut_compilation);

void imprime_stats_detaillee(Statistiques &stats);

#undef EXTRAIT_CHAINE_ENUM
#undef EXTRAIT_IDENT_ENUM
#undef INIT_NOMS_ENTREES_IMPL
#undef INIT_NOMS_ENTREES
#undef DEFINIS_ENUM_IMPL
#undef DEFINIS_ENUM
#undef ENTREES_POUR_VALIDATION_APPEL
#undef ENTREES_POUR_FINALISATION
#undef ENTREES_POUR_OPERATEUR_UNAIRE
#undef ENTREES_POUR_OPERATEUR_BINAIRE
#undef ENTREES_POUR_DECLARATION_VARIABLES
#undef ENTREES_POUR_REFERENCE_DECLARATION
#undef ENTREES_POUR_ENTETE_FONCTION
#undef ENTREES_POUR_CORPS_FONCTION
#undef ENTREES_POUR_ENUMERATION
#undef ENTREES_POUR_STRUCTURE
#undef ENTREES_POUR_ASSIGNATION
