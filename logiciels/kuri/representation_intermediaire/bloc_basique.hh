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
 * The Original Code is Copyright (C) 2022 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "structures/tableau.hh"

struct AtomeFonction;
struct ConstructriceRI;
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

    void ajoute_enfant(Bloc *enfant);

    void remplace_enfant(Bloc *enfant, Bloc *par);

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

Bloc *bloc_pour_label(kuri::tableau<Bloc *, int> &blocs, InstructionLabel *label);

struct FonctionEtBlocs {
    AtomeFonction *fonction = nullptr;
    kuri::tableau<Bloc *, int> blocs{};

    ~FonctionEtBlocs();
};

FonctionEtBlocs convertis_en_blocs(AtomeFonction *atome_fonc);
