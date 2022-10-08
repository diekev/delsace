/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2022 Kévin Dietrich. */

#pragma once

#include "structures/tableau.hh"

struct AtomeFonction;
struct ConstructriceRI;
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

struct FonctionEtBlocs {
    AtomeFonction *fonction = nullptr;
    kuri::tableau<Bloc *, int> blocs{};

    ~FonctionEtBlocs();

    bool convertis_en_blocs(EspaceDeTravail &espace, AtomeFonction *atome_fonc);
};
