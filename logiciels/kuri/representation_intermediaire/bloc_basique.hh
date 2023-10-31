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

struct Bloc {
    InstructionLabel *label = nullptr;

    kuri::tableau<Instruction *, int> instructions{};

    kuri::tableau<Bloc *, int> parents{};
    kuri::tableau<Bloc *, int> enfants{};

    /* les variables déclarées dans ce bloc */
    kuri::tableau<InstructionAllocation *, int> variables_declarees{};

    /* les variables utilisées dans ce bloc */
    kuri::tableau<InstructionAllocation *, int> variables_utilisees{};

    bool est_atteignable = false;

    void ajoute_enfant(Bloc *enfant);

    void remplace_enfant(Bloc *enfant, Bloc *par);

    void remplace_parent(Bloc *parent, Bloc *par);

    void enleve_parent(Bloc *parent);

    void enleve_enfant(Bloc *enfant);

    bool peut_fusionner_enfant();

    void utilise_variable(InstructionAllocation *variable);

    void fusionne_enfant(Bloc *enfant);

    void reinitialise();

  private:
    void enleve_du_tableau(kuri::tableau<Bloc *, int> &tableau, Bloc *bloc);

    void ajoute_parent(Bloc *parent);
};

void imprime_bloc(Bloc *bloc,
                  int decalage_instruction,
                  std::ostream &os,
                  bool surligne_inutilisees = false);

void imprime_blocs(const kuri::tableau<Bloc *, int> &blocs, std::ostream &os);

void construit_liste_variables_utilisees(Bloc *bloc);

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

    void reinitialise();

    void supprime_blocs_inatteignables(VisiteuseBlocs &visiteuse);

    /**
     * Reconstruit les instructions de la fonction en se basant sur les instructions des blocs
     * existants. Ne fait rien si les blocs n'ont pas été modifiés (p.e. aucun bloc n'a été
     * supprimé ou fusionné dans un autre).
     */
    void ajourne_instructions_fonction_si_nécessaire();
};

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
