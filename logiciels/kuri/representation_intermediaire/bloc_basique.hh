/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "structures/ensemble.hh"
#include "structures/file.hh"
#include "structures/tableau.hh"

struct AtomeFonction;
struct EspaceDeTravail;
struct Instruction;
struct InstructionAllocation;
struct InstructionLabel;

enum class GenreInstruction : uint32_t;

struct Bloc {
    InstructionLabel *label = nullptr;

    kuri::tableau<Instruction *, int> instructions{};

    kuri::tableau<Bloc *, int> parents{};
    kuri::tableau<Bloc *, int> enfants{};

    /* les variables déclarées dans ce bloc */
    kuri::tableau<InstructionAllocation const *, int> variables_declarees{};

    /* les variables utilisées dans ce bloc */
    kuri::tableau<InstructionAllocation const *, int> variables_utilisees{};

    bool est_atteignable = false;

  private:
    uint32_t masque_instructions = 0;

  public:
    void ajoute_instruction(Instruction *inst);

    bool possède_instruction_de_genre(GenreInstruction genre) const;

    int donne_id() const;

    void ajoute_enfant(Bloc *enfant);

    void remplace_enfant(Bloc *enfant, Bloc *par);

    void remplace_parent(Bloc *parent, Bloc *par);

    void enlève_parent(Bloc *parent);

    void enlève_enfant(Bloc *enfant);

    bool peut_fusionner_enfant();

    void utilise_variable(const InstructionAllocation *variable);

    void fusionne_enfant(Bloc *enfant);

    void réinitialise();

    /* Enlève la relation parent/enfant de ce bloc avec le bloc parent passé en paramètre.
     * Si le parent était le seul parent, déconnecte également ce bloc de ses enfants. */
    void déconnecte_pour_branche_morte(Bloc *parent);

  private:
    void enlève_du_tableau(kuri::tableau<Bloc *, int> &tableau, const Bloc *bloc);

    void ajoute_parent(Bloc *parent);
};

void imprime_bloc(const Bloc *bloc,
                  int decalage_instruction,
                  std::ostream &os,
                  bool surligne_inutilisees = false);

void imprime_blocs(const kuri::tableau<Bloc *, int> &blocs, std::ostream &os);

void construit_liste_variables_utilisées(Bloc *bloc);

struct VisiteuseBlocs;

struct FonctionEtBlocs {
    AtomeFonction *fonction = nullptr;
    kuri::tableau<Bloc *, int> blocs{};
    kuri::tableau<Bloc *, int> blocs_libres{};

  private:
    bool les_blocs_ont_été_modifiés = false;

  public:
    ~FonctionEtBlocs();

    bool convertis_en_blocs(EspaceDeTravail &espace, AtomeFonction *atome_fonc);

    void réinitialise();

    void marque_blocs_modifiés();

    void supprime_blocs_inatteignables(VisiteuseBlocs &visiteuse);

    /**
     * Reconstruit les instructions de la fonction en se basant sur les instructions des blocs
     * existants. Ne fait rien si les blocs n'ont pas été modifiés (p.e. aucun bloc n'a été
     * supprimé ou fusionné dans un autre).
     */
    void ajourne_instructions_fonction_si_nécessaire();
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
