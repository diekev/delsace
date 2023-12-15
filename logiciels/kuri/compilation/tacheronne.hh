/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "biblinternes/outils/badge.hh"

#include "arbre_syntaxique/allocatrice.hh"
#include "arbre_syntaxique/noeud_code.hh"

#include "structures/file.hh"

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
#define ENUMERE_CAPACITE(VERBE, ACTION, CHAINE, INDEX) PEUT_##VERBE = (1 << INDEX),

    ENUMERE_TACHES_POSSIBLES(ENUMERE_CAPACITE)

#undef ENUMERE_CAPACITE

        PEUT_TOUT_FAIRE = 0xfffffff,
};

DEFINIS_OPERATEURS_DRAPEAU(DrapeauxTacheronne)

std::ostream &operator<<(std::ostream &os, DrapeauxTacheronne drapeaux);

struct OrdonnanceuseTache {
  public:
    enum {
#define ENUMERE_FILE(VERBE, ACTION, CHAINE, INDEX) FILE_##ACTION,

        ENUMERE_TACHES_POSSIBLES(ENUMERE_FILE)

#undef ENUMERE_FILE

            NOMBRE_FILES,
    };

  private:
    Compilatrice *m_compilatrice = nullptr;

    kuri::file<Tache> taches[NOMBRE_FILES];

    int nombre_de_tacheronnes = 0;
    bool compilation_terminee = false;

    // Tiens trace du nombre maximal de tâches par file, afin de générer des statistiques.
    struct PiqueTailleFile {
        int64_t taches[OrdonnanceuseTache::NOMBRE_FILES];

        PiqueTailleFile()
        {
            POUR (taches) {
                it = 0;
            }
        }

        ~PiqueTailleFile()
        {
#if 0
            std::cerr << "Pique taille files :\n";
#    define IMPRIME_NOMBRE_DE_TACHES(VERBE, ACTION, CHAINE, INDEX)                                \
        std::cerr << "-- " << CHAINE << " : " << taches[INDEX] << '\n';

            ENUMERE_TACHES_POSSIBLES(IMPRIME_NOMBRE_DE_TACHES)

#    undef IMPRIME_NOMBRE_DE_TACHES
#endif
        }
    };

    PiqueTailleFile pique_taille{};

  public:
    OrdonnanceuseTache() = default;
    OrdonnanceuseTache(Compilatrice *compilatrice);

    OrdonnanceuseTache(OrdonnanceuseTache const &) = delete;
    OrdonnanceuseTache &operator=(OrdonnanceuseTache const &) = delete;

    void crée_tache_pour_unite(UniteCompilation *unite);

    Tache tache_suivante(Tache &tache_terminee, DrapeauxTacheronne drapeaux);

    int64_t memoire_utilisee() const;

    int enregistre_tacheronne(Badge<Tacheronne> badge);

    void supprime_toutes_les_taches();
    void supprime_toutes_les_taches_pour_espace(EspaceDeTravail const *espace,
                                                UniteCompilation::État état);

    void marque_compilation_terminee()
    {
        compilation_terminee = true;
    }

    void imprime_donnees_files(std::ostream &os);

  private:
    int64_t nombre_de_taches_en_attente() const;
};

struct Tacheronne {
    Compilatrice &compilatrice;

    CompilatriceRI constructrice_ri{compilatrice};
    ContexteAnalyseRI *analyseuse_ri = nullptr;
    MachineVirtuelle *mv = nullptr;

    AllocatriceNoeud allocatrice_noeud{};
    AssembleuseArbre *assembleuse = nullptr;

    ConvertisseuseNoeudCode convertisseuse_noeud_code{};

    tableau_page<Lexeme> lexemes_extra{};

    Broyeuse broyeuse{};

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
    double temps_passe_a_dormir = 0.0;

    Tacheronne(Compilatrice &comp);

    ~Tacheronne();

    EMPECHE_COPIE(Tacheronne);

    void gere_tache();

    void rassemble_statistiques(Statistiques &stats);

  private:
    void gere_unite_pour_typage(UniteCompilation *unite);
    bool gere_unite_pour_ri(UniteCompilation *unite);
    void gere_unite_pour_optimisation(UniteCompilation *unite);
    void gere_unite_pour_execution(UniteCompilation *unite);

    void execute_metaprogrammes();

    /* Pour convertir le résultat des métaprogrammes en noeuds syntaxiques. */
    NoeudExpression *noeud_syntaxique_depuis_résultat(
        EspaceDeTravail *espace,
        NoeudDirectiveExecute *directive,
        Lexeme const *lexeme,
        Type *type,
        octet_t *pointeur,
        DétectriceFuiteDeMémoire &détectrice_fuites_de_mémoire);
};
