/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/chrono/outils.hh"

#include "structures/chaine.hh"
#include "structures/tableau.hh"

#undef STATISTIQUES_DETAILLEES

#ifdef STATISTIQUES_DETAILLEES
#    define CHRONO_TYPAGE(entrée_stats, index)                                                    \
        dls::chrono::chrono_rappel_milliseconde VARIABLE_ANONYME(chrono)(                         \
            [&](double temps) { entrée_stats.fusionne_entrée(index, {"", temps}); })
#else
#    define CHRONO_TYPAGE(entrée_stats, nom)
#endif

#if defined __cpp_concepts && __cpp_concepts >= 201507
template <typename T>
concept TypeEntréesStats = requires(T a, T b) { a += b; };
#else
#    define TypeEntréesStats typename
#endif

struct EntréeNombreMémoire {
    kuri::chaine_statique nom = "";
    int64_t compte = 0;
    int64_t mémoire = 0;

    EntréeNombreMémoire &operator+=(EntréeNombreMémoire const &autre)
    {
        compte += autre.compte;
        mémoire += autre.mémoire;
        return *this;
    }

    bool peut_fusionner_avec(EntréeNombreMémoire const &autre) const
    {
        return autre.nom == nom;
    }
};

struct EntréeTaille {
    kuri::chaine_statique nom = "";
    int64_t taille = 0;

    EntréeTaille &operator+=(EntréeTaille const &autre)
    {
        taille = std::max(taille, autre.taille);
        return *this;
    }

    bool peut_fusionner_avec(EntréeTaille const &autre) const
    {
        return autre.nom == nom;
    }
};

struct EntréeTailleTableau {
    kuri::chaine_statique nom = "";
    int64_t taille_max = 0;
    int64_t taille_min = std::numeric_limits<int64_t>::max();
    kuri::tableau<int64_t> valeurs{};

    EntréeTailleTableau &operator+=(EntréeTailleTableau const &autre)
    {
        if (valeurs.est_vide()) {
            valeurs.ajoute(taille_max);
        }

        taille_max = std::max(taille_max, autre.taille_max);
        taille_min = std::min(taille_min, autre.taille_max);

        valeurs.ajoute(autre.taille_max);

        return *this;
    }

    bool peut_fusionner_avec(EntréeTailleTableau const &autre) const
    {
        return autre.nom == nom;
    }
};

struct EntréeFichier {
    kuri::chaine_statique chemin = "";
    kuri::chaine_statique nom = "";
    int64_t mémoire_lexèmes = 0;
    int64_t nombre_lexèmes = 0;
    int64_t nombre_lignes = 0;
    int64_t mémoire_tampons = 0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_chargement = 0.0;
    double temps_tampon = 0.0;

    EntréeFichier &operator+=(EntréeFichier const &autre)
    {
        mémoire_lexèmes += autre.mémoire_lexèmes;
        nombre_lignes += autre.nombre_lignes;
        temps_lexage += autre.temps_lexage;
        temps_parsage += autre.temps_parsage;
        temps_chargement += autre.temps_chargement;
        temps_tampon += autre.temps_tampon;
        nombre_lexèmes += autre.nombre_lexèmes;
        mémoire_tampons += autre.mémoire_tampons;
        return *this;
    }

    bool peut_fusionner_avec(EntréeFichier const &autre) const
    {
        return autre.chemin == chemin;
    }
};

struct EntréeTemps {
    const char *nom{};
    double temps = 0.0;

    EntréeTemps &operator+=(EntréeTemps const &autre)
    {
        temps += autre.temps;
        return *this;
    }

    bool peut_fusionner_avec(EntréeTemps const &autre) const
    {
        return autre.nom == nom;
    }
};

template <TypeEntréesStats T>
struct EntréesStats {
    kuri::chaine_statique nom{};
    /* Mutable pour pouvoir le trier avant l'impression. */
    mutable kuri::tableau<T, int> entrées{};
    T totaux{};

    explicit EntréesStats(kuri::chaine_statique nom_) : nom(nom_)
    {
    }

    EntréesStats(kuri::chaine_statique nom_, std::initializer_list<const char *> &&noms_entrées)
        : nom(nom_)
    {
        entrées.réserve(static_cast<int>(noms_entrées.size()));
        for (auto nom_entrée : noms_entrées) {
            T e;
            e.nom = nom_entrée;
            entrées.ajoute(e);
        }
    }

    void ajoute_entrée(T const &entrée)
    {
        totaux += entrée;
        entrées.ajoute(entrée);
    }

    void fusionne_entrée(T const &entrée)
    {
        totaux += entrée;

        for (auto &e : entrées) {
            if (e.peut_fusionner_avec(entrée)) {
                e += entrée;
                return;
            }
        }

        entrées.ajoute(entrée);
    }

    void fusionne_entrée(int index, T const &entrée)
    {
        totaux += entrée;
        entrées[index] += entrée;
    }
};

using StatistiquesFichiers = EntréesStats<EntréeFichier>;
using StatistiquesArbre = EntréesStats<EntréeNombreMémoire>;
using StatistiquesGraphe = EntréesStats<EntréeNombreMémoire>;
using StatistiquesOperateurs = EntréesStats<EntréeNombreMémoire>;
using StatistiquesNoeudCode = EntréesStats<EntréeNombreMémoire>;
using StatistiquesMessage = EntréesStats<EntréeNombreMémoire>;
using StatistiquesRI = EntréesStats<EntréeNombreMémoire>;
using StatistiquesTableaux = EntréesStats<EntréeTailleTableau>;
using StatistiquesGaspillage = EntréesStats<EntréeNombreMémoire>;

struct MémoireUtilisée {
    kuri::chaine_statique catégorie{};
    int64_t quantité = 0;
};

struct Statistiques {
  private:
    mutable kuri::tableau<MémoireUtilisée> m_mémoires_utilisées{};

  public:
    int64_t nombre_modules = 0l;
    int64_t nombre_identifiants = 0l;
    int64_t nombre_métaprogrammes_exécutés = 0l;
    int64_t instructions_exécutées = 0l;
    double temps_génération_code = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_exécutable = 0.0;
    double temps_ri = 0.0;
    double temps_métaprogrammes = 0.0;
    double temps_scène = 0.0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_typage = 0.0;
    double temps_chargement = 0.0;
    double temps_tampons = 0.0;

    StatistiquesFichiers stats_fichiers{"Fichiers"};
    StatistiquesArbre stats_arbre{"Arbre Syntaxique"};
    StatistiquesGraphe stats_graphe_dépendance{"Graphe Dépendances"};
    StatistiquesOperateurs stats_opérateurs{"Opérateurs"};
    StatistiquesNoeudCode stats_noeuds_code{"Noeuds Code"};
    StatistiquesMessage stats_messages{"Messages"};
    StatistiquesRI stats_ri{"Représentation Intermédiaire"};
    StatistiquesTableaux stats_tableaux{"Tableaux"};
    StatistiquesGaspillage stats_gaspillage{"Gaspillage"};

    kuri::tableau<MémoireUtilisée> const &donne_mémoire_utilisée_pour_impression() const;

    void ajoute_mémoire_utilisée(kuri::chaine_statique catégorie, int64_t quantité) const;
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
    EntréesStats<EntréeTemps> validation_decl{"Déclarations Variables",
                                              INIT_NOMS_ENTREES(DECLARATION_VARIABLES)};
    EntréesStats<EntréeTemps> validation_appel{"Appels", INIT_NOMS_ENTREES(VALIDATION_APPEL)};
    EntréesStats<EntréeTemps> ref_decl{"Références Déclarations",
                                       INIT_NOMS_ENTREES(REFERENCE_DECLARATION)};
    EntréesStats<EntréeTemps> opérateurs_unaire{"Opérateurs Unaire",
                                                INIT_NOMS_ENTREES(OPERATEUR_UNAIRE)};
    EntréesStats<EntréeTemps> opérateurs_binaire{"Opérateurs Binaire",
                                                 INIT_NOMS_ENTREES(OPERATEUR_BINAIRE)};
    EntréesStats<EntréeTemps> entêtes_fonctions{"Entêtes Fonctions",
                                                INIT_NOMS_ENTREES(ENTETE_FONCTION)};
    EntréesStats<EntréeTemps> corps_fonctions{"Corps Fonctions",
                                              INIT_NOMS_ENTREES(CORPS_FONCTION)};
    EntréesStats<EntréeTemps> énumérations{"Énumérations", INIT_NOMS_ENTREES(ENUMERATION)};
    EntréesStats<EntréeTemps> structures{"Structures", INIT_NOMS_ENTREES(STRUCTURE)};
    EntréesStats<EntréeTemps> assignations{"Assignations", INIT_NOMS_ENTREES(ASSIGNATION)};
    EntréesStats<EntréeTemps> finalisation{"Finalisation", INIT_NOMS_ENTREES(FINALISATION)};

    void imprime_stats();
};

#define ENTREES_POUR_GESTIONNAIRE_CODE(OP)                                                        \
    OP(GESTION__DÉTERMINE_DÉPENDANCES, "détermine dépendances")                                   \
    OP(GESTION__RASSEMBLE_DÉPENDANCES, "rassemble dépendances")                                   \
    OP(GESTION__AJOUTE_DÉPENDANCES, "ajoute dépendances")                                         \
    OP(GESTION__GARANTIE_TYPAGE_DÉPENDANCES, "garantie typage dépendances")                       \
    OP(GESTION__AJOUTE_RACINES, "ajoute racine")                                                  \
    OP(GESTION__TYPAGE_TERMINÉ, "typage terminé")                                                 \
    OP(GESTION__DOIT_DÉTERMINER_DÉPENDANCES, "doit déterminer dépendances")                       \
    OP(GESTION__VÉRIFIE_ENTÊTE_VALIDÉES, "vérifie entête validées")                               \
    OP(GESTION__CRÉATION_TÂCHES, "création tâches")

DEFINIS_ENUM(GESTIONNAIRE_CODE)

/* Stats pour le GestionnaireCode */
struct StatistiquesGestion {
    EntréesStats<EntréeTemps> stats{"Gestionnaire Code", INIT_NOMS_ENTREES(GESTIONNAIRE_CODE)};

    void imprime_stats();
};

void imprime_stats(Statistiques const &stats, dls::chrono::compte_seconde début_compilation);

void imprime_stats_détaillées(Statistiques const &stats);

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
#undef ENTREES_POUR_GESTIONNAIRE_CODE
