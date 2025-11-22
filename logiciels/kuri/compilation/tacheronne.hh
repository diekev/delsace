/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/allocatrice.hh"

#include "parsage/outils_lexemes.hh"

#include "structures/file.hh"

#include "utilitaires/badge.hh"

#include "broyage.hh"
#include "statistiques/statistiques.hh"
#include "tache.hh"
#include "unite_compilation.hh"

#include "../representation_intermediaire/constructrice_ri.hh"

using octet_t = unsigned char;

struct Compilatrice;
struct ContexteAnalyseRI;
struct DétectriceFuiteDeMémoire;
struct MachineVirtuelle;
struct Tacheronne;

/* Drapeaux pour les tâches étant dans des files. */
enum class DrapeauxTacheronne : uint32_t {
#define ENUMERE_CAPACITE(VERBE, ACTION, CHAINE, INDICE) PEUT_##VERBE = (1 << INDICE),

    ENUMERE_TACHES_POSSIBLES(ENUMERE_CAPACITE)

#undef ENUMERE_CAPACITE

        PEUT_TOUT_FAIRE = 0xfffffff,
};

DEFINIS_OPERATEURS_DRAPEAU(DrapeauxTacheronne)

std::ostream &operator<<(std::ostream &os, DrapeauxTacheronne drapeaux);

struct OrdonnanceuseTache {
  public:
    enum {
#define ENUMERE_FILE(VERBE, ACTION, CHAINE, INDICE) FILE_##ACTION,

        ENUMERE_TACHES_POSSIBLES(ENUMERE_FILE)

#undef ENUMERE_FILE

            NOMBRE_FILES,
    };

  private:
    Compilatrice *m_compilatrice = nullptr;

    kuri::file<Tâche> tâches[NOMBRE_FILES];

    int nombre_de_tacheronnes = 0;
    bool compilation_terminée = false;

    // Tiens trace du nombre maximal de tâches par file, afin de générer des statistiques.
    struct PiqueTailleFile {
        int64_t tâches[OrdonnanceuseTache::NOMBRE_FILES];

        PiqueTailleFile()
        {
            POUR (tâches) {
                it = 0;
            }
        }

        ~PiqueTailleFile()
        {
#if 0
            std::cerr << "Pique taille files :\n";
#    define IMPRIME_NOMBRE_DE_TACHES(VERBE, ACTION, CHAINE, INDICE)                               \
        std::cerr << "-- " << CHAINE << " : " << tâches[INDICE] << '\n';

            ENUMERE_TACHES_POSSIBLES(IMPRIME_NOMBRE_DE_TACHES)

#    undef IMPRIME_NOMBRE_DE_TACHES
#endif
        }
    };

    PiqueTailleFile pique_taille{};

  public:
    OrdonnanceuseTache() = default;
    OrdonnanceuseTache(Compilatrice *compilatrice);

    EMPECHE_COPIE(OrdonnanceuseTache);

    void crée_tâche_pour_unité(UnitéCompilation *unité);

    Tâche tâche_suivante(Tâche &tache_terminee, DrapeauxTacheronne drapeaux);

    int64_t mémoire_utilisée() const;

    int enregistre_tacheronne(Badge<Tacheronne> badge);

    void supprime_toutes_les_tâches();
    void supprime_toutes_les_tâches_pour_espace(EspaceDeTravail const *espace,
                                                UnitéCompilation::État état);

    void marque_compilation_terminee()
    {
        compilation_terminée = true;
    }

    void imprime_données_files(std::ostream &os);

    int64_t nombre_de_tâches_en_attente() const;
};

struct Tacheronne {
  private:
    Compilatrice &compilatrice;

    CompilatriceRI constructrice_ri{compilatrice};
    ContexteAnalyseRI *analyseuse_ri = nullptr;
    MachineVirtuelle *mv = nullptr;

    AllocatriceNoeud allocatrice_noeud{};
    AssembleuseArbre *assembleuse = nullptr;

    LexèmesExtra lexèmes_extra{};

    Broyeuse broyeuse{};

  public:
    double temps_validation = 0.0;
    double temps_lexage = 0.0;
    double temps_parsage = 0.0;
    double temps_executable = 0.0;
    double temps_fichier_objet = 0.0;
    double temps_scene = 0.0;
    double temps_generation_code = 0.0;
    double temps_chargement = 0.0;
    double temps_tampons = 0.0;
    double temps_optimisation = 0.0;

    DrapeauxTacheronne drapeaux = DrapeauxTacheronne::PEUT_TOUT_FAIRE;

    int id = 0;
    int nombre_dodos = 0;
    double temps_passe_à_dormir = 0.0;

    Tacheronne(Compilatrice &comp);

    ~Tacheronne();

    EMPECHE_COPIE(Tacheronne);

    bool gère_tâche();

    void rassemble_statistiques(Statistiques &stats);

    void initialise_contexte(Contexte *contexte, EspaceDeTravail *espace);

  private:
    void gère_unité_pour_typage(UnitéCompilation *unité);
    bool gère_unité_pour_ri(UnitéCompilation *unité);
    void gère_unité_pour_optimisation(UnitéCompilation *unité);
    void gère_unité_pour_exécution(UnitéCompilation *unité);

    void exécute_métaprogrammes();

    /* Pour convertir le résultat des métaprogrammes en noeuds syntaxiques. */
    NoeudExpression *noeud_syntaxique_depuis_résultat(
        EspaceDeTravail *espace,
        NoeudDirectiveExécute *directive,
        Lexème const *lexème,
        Type *type,
        octet_t *pointeur,
        DétectriceFuiteDeMémoire &détectrice_fuites_de_mémoire);
};
