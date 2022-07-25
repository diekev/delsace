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
 * The Original Code is Copyright (C) 2020 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/outils/badge.hh"

#include "arbre_syntaxique/allocatrice.hh"

#include "structures/file.hh"

#include "statistiques/statistiques.hh"
#include "unite_compilation.hh"
#include "validation_semantique.hh"

#include "../representation_intermediaire/constructrice_ri.hh"
#include "../representation_intermediaire/machine_virtuelle.hh"

struct Compilatrice;
struct MetaProgramme;
struct Tacheronne;

#define ENUMERE_TACHES_POSSIBLES(O)                                                               \
    O(CHARGER, CHARGEMENT, "chargement", 0)                                                       \
    O(LEXER, LEXAGE, "lexage", 1)                                                                 \
    O(PARSER, PARSAGE, "parsage", 2)                                                              \
    O(CREER_FONCTION_INIT_TYPE, CREATION_FONCTION_INIT_TYPE, "création fonction init type", 3)    \
    O(TYPER, TYPAGE, "typage", 4)                                                                 \
    O(CONVERTIR_NOEUD_CODE, CONVERSION_NOEUD_CODE, "conversion noeud code", 5)                    \
    O(ENVOYER_MESSAGE, ENVOIE_MESSAGE, "envoie message", 6)                                       \
    O(GENERER_RI, GENERATION_RI, "génération RI", 7)                                              \
    O(EXECUTER, EXECUTION, "exécution", 8)                                                        \
    O(OPTIMISER, OPTIMISATION, "optimisation", 9)                                                 \
    O(GENERER_CODE, GENERATION_CODE_MACHINE, "génération code machine", 10)                       \
    O(LIER_PROGRAMME, LIAISON_PROGRAMME, "liaison programme", 11)

#define ENUMERE_GENRES_TACHE(O)                                                                   \
    O(DORMIR, DORS, "dormir", 0)                                                                  \
    O(COMPILATION_TERMINEE, COMPILATION_TERMINEE, "compilation terminée", 0)                      \
    ENUMERE_TACHES_POSSIBLES(O)

enum class GenreTache {
#define ENUMERE_GENRE_TACHE(VERBE, ACTION, CHAINE, INDEX) ACTION,
    ENUMERE_GENRES_TACHE(ENUMERE_GENRE_TACHE)
#undef ENUMERE_GENRE_TACHE
};

const char *chaine_genre_tache(GenreTache genre);

std::ostream &operator<<(std::ostream &os, GenreTache genre);

struct Tache {
    GenreTache genre = GenreTache::DORS;
    UniteCompilation *unite = nullptr;
    EspaceDeTravail *espace = nullptr;

    static Tache dors(EspaceDeTravail *espace_);

    static Tache compilation_terminee();

    static Tache genere_fichier_objet(EspaceDeTravail *espace_);

    static Tache liaison_objet(EspaceDeTravail *espace_);
};

/* Drapeaux pour les tâches étant dans des files. */
enum class DrapeauxTacheronne : uint32_t {
#define ENUMERE_CAPACITE(VERBE, ACTION, CHAINE, INDEX) PEUT_##VERBE = (1 << INDEX),

    ENUMERE_TACHES_POSSIBLES(ENUMERE_CAPACITE)

#undef ENUMERE_CAPACITE

        PEUT_TOUT_FAIRE = 0xfffffff,
};

DEFINIE_OPERATEURS_DRAPEAU(DrapeauxTacheronne, unsigned int)

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
        long taches[OrdonnanceuseTache::NOMBRE_FILES];

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

    void cree_tache_pour_unite(UniteCompilation *unite);

    Tache tache_suivante(Tache &tache_terminee, DrapeauxTacheronne drapeaux);

    long memoire_utilisee() const;

    int enregistre_tacheronne(Badge<Tacheronne> badge);

    void supprime_toutes_les_taches();
    void supprime_toutes_les_taches_pour_espace(EspaceDeTravail const *espace);

    void marque_compilation_terminee()
    {
        compilation_terminee = true;
    }

    void imprime_donnees_files(std::ostream &os);

  private:
    long nombre_de_taches_en_attente() const;
};

struct Tacheronne {
    Compilatrice &compilatrice;

    ConstructriceRI constructrice_ri{compilatrice};
    MachineVirtuelle mv{compilatrice};

    AllocatriceNoeud allocatrice_noeud{};
    AssembleuseArbre *assembleuse = nullptr;

    ConvertisseuseNoeudCode convertisseuse_noeud_code{};

    StatistiquesTypage stats_typage{};

    ContexteValidationDeclaration contexte_validation_declaration{};

    tableau_page<Lexeme> lexemes_extra{};

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

    COPIE_CONSTRUCT(Tacheronne);

    void gere_tache();

  private:
    void gere_unite_pour_typage(UniteCompilation *unite);
    bool gere_unite_pour_ri(UniteCompilation *unite);
    void gere_unite_pour_optimisation(UniteCompilation *unite);
    void gere_unite_pour_execution(UniteCompilation *unite);

    void execute_metaprogrammes();

    /* Pour convertir le résultat des métaprogrammes en noeuds syntaxiques. */
    NoeudExpression *noeud_syntaxique_depuis_resultat(EspaceDeTravail *espace,
                                                      NoeudDirectiveExecute *directive,
                                                      Lexeme const *lexeme,
                                                      Type *type,
                                                      octet_t *pointeur);
};
