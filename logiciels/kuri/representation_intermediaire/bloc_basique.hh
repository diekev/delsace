/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "structures/ensemble.hh"
#include "structures/file.hh"
#include "structures/table_hachage.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

#include "instructions.hh"

struct EspaceDeTravail;
struct FonctionEtBlocs;

enum class GenreInstruction : uint32_t;

namespace kuri {
struct chaine;
}

struct Bloc {
    FonctionEtBlocs *fonction_et_blocs = nullptr;

    InstructionLabel *label = nullptr;

    kuri::tableau<Instruction *, int> instructions{};

    kuri::tableau<Bloc *, int> parents{};
    kuri::tableau<Bloc *, int> enfants{};

    bool est_atteignable = false;

  private:
    bool instructions_à_supprimer = false;
    uint32_t masque_instructions = 0;

  public:
    void ajoute_instruction(Instruction *inst);

    bool possède_instruction_de_genre(GenreInstruction genre) const;

    int donne_id() const;

    void ajoute_enfant(Bloc *enfant);

    void remplace_parent(Bloc *parent, Bloc *par);

    void enlève_parent(Bloc *parent);

    void enlève_enfant(Bloc *enfant);

    void fusionne_enfant(Bloc *enfant);

    void réinitialise();

    /* Enlève la relation parent/enfant de ce bloc avec le bloc parent passé en paramètre.
     * Si le parent était le seul parent, déconnecte également ce bloc de ses enfants. */
    void déconnecte_pour_branche_morte(Bloc *parent);

    void tag_instruction_à_supprimer(Instruction *inst);

    /* Supprime du bloc les instructions dont l'état est EST_A_SUPPRIMER. */
    bool supprime_instructions_à_supprimer();

  private:
    void enlève_du_tableau(kuri::tableau<Bloc *, int> &tableau, const Bloc *bloc);

    void ajoute_parent(Bloc *parent);
};

[[nodiscard]] kuri::chaine imprime_bloc(const Bloc *bloc,
                                        int decalage_instruction,
                                        bool surligne_inutilisees = false);

[[nodiscard]] kuri::chaine imprime_blocs(const kuri::tableau<Bloc *, int> &blocs);

struct VisiteuseBlocs;

/* ------------------------------------------------------------------------- */
/** \name Table des utilisateurs pour le graphe de dépendances.
 * \{ */

class TableUtilisateurs {
    struct DonnéesUtilisateur {
        /* Index du premier utilisateur dans m_utilisateurs. */
        int premier_utilisateur = 0;
        /* Index du dernier utilisateur dans m_utilisateurs. */
        int dernier_utilisateur = 0;
        /* Puisque nous ne supprimons pas les utilisateurs (nous recréons la table à chaque
         * changement), nous mettons en cache le nombre de ceux-ci. */
        int nombre_utilisateurs = 0;
    };
    kuri::tableau<DonnéesUtilisateur, int> m_données_utilisateurs{};

    struct Utilisateur {
        Atome *utilisateur = nullptr;
        int suivant = 0;
        int index_bloc = 0;
    };
    kuri::tableau<Utilisateur, int> m_utilisateurs{};

    kuri::table_hachage<Atome const *, int> m_table_données_utilisateurs{"Données utilisateurs"};

  public:
    void réinitialise();

    void ajoute_connexion(Atome *utilisé, Atome *utilisateur, int index_bloc);

    bool est_uniquement_utilisé_dans_bloc(Instruction const *inst, int index_bloc) const;

    int64_t nombre_d_utilisateurs(Instruction const *inst) const;

    template <typename Fonction>
    void visite_utilisateurs(Instruction const *inst, Fonction rappel) const
    {
        auto index_données_utilisateur = m_table_données_utilisateurs.valeur_ou(inst, -1);
        if (index_données_utilisateur == -1) {
            return;
        }

        auto &données_utilisateur = m_données_utilisateurs[index_données_utilisateur];
        auto utilisateur = &m_utilisateurs[données_utilisateur.premier_utilisateur];

        while (true) {
            rappel(utilisateur->utilisateur);

            if (utilisateur->suivant == -1) {
                break;
            }

            utilisateur = &m_utilisateurs[utilisateur->suivant];
        }
    }
};

/** \} */

/* ------------------------------------------------------------------------- */
/** \name Graphe.
 *  Contiens les connexions entre les instructions et leurs atomes.
 * \{ */

struct Graphe {
  private:
    TableUtilisateurs m_table{};

  public:
    /* a est utilisé par b */
    void ajoute_connexion(Atome *a, Atome *b, int index_bloc);

    void construit(kuri::tableau<Instruction *, int> const &instructions, int index_bloc);

    bool est_uniquement_utilisé_dans_bloc(Instruction const *inst, int index_bloc) const;

    int64_t nombre_d_utilisateurs(Instruction const *inst) const;

    template <typename Fonction>
    void visite_utilisateurs(Instruction const *inst, Fonction rappel) const
    {
        m_table.visite_utilisateurs(inst, rappel);
    }

    void réinitialise();
};

/** \} */

struct FonctionEtBlocs {
    AtomeFonction *fonction = nullptr;
    kuri::tableau<Bloc *, int> blocs{};
    kuri::tableau<Bloc *, int> blocs_libres{};

  private:
    kuri::tableau<Bloc *, int> table_blocs{};
    Graphe graphe{};

    bool les_blocs_ont_été_modifiés = false;
    /* Vrai par défaut pour le construire au moins 1 fois. */
    bool graphe_nécessite_ajournement = true;

  public:
    ~FonctionEtBlocs();

    bool convertis_en_blocs(EspaceDeTravail &espace, AtomeFonction *atome_fonc);

    void réinitialise();

    void marque_blocs_modifiés();

    void marque_instructions_modifiés();

    void supprime_blocs_inatteignables(VisiteuseBlocs &visiteuse);

    /**
     * Reconstruit les instructions de la fonction en se basant sur les instructions des blocs
     * existants. Ne fait rien si les blocs n'ont pas été modifiés (p.e. aucun bloc n'a été
     * supprimé ou fusionné dans un autre).
     */
    void ajourne_instructions_fonction_si_nécessaire();

    Graphe &donne_graphe_ajourné();
};

/* ------------------------------------------------------------------------- */
/** \name Fonctions auxiliaires.
 * \{ */

void transfère_instructions_blocs_à_fonction(kuri::tableau_statique<Bloc *> blocs,
                                             AtomeFonction *fonction);

/** \} */

/* ------------------------------------------------------------------------- */
/** \name VisiteuseBlocs
 * Structure pour visiter les blocs de manière hiérarchique.
 * \{ */

struct VisiteuseBlocs {
  private:
    FonctionEtBlocs const &m_fonction_et_blocs;

    /* Mémoire pour la visite. */
    kuri::ensemble<Bloc *> blocs_visités{};
    kuri::file<Bloc *> à_visiter{};

  public:
    VisiteuseBlocs(FonctionEtBlocs const &fonction_et_blocs);

    void prépare_pour_nouvelle_traversée();

    bool a_visité(Bloc *bloc) const;

    Bloc *bloc_suivant();
};

/** \} */
