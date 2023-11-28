/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#pragma once

#include "structures/table_hachage.hh"
#include "structures/tableau.hh"
#include "structures/tablet.hh"

#include "bloc_basique.hh"

struct Atome;
struct AtomeFonction;
struct ConstructriceRI;
struct EspaceDeTravail;
struct Instruction;

struct Graphe {
  private:
    struct Connexion {
        Atome *utilise;
        Atome *utilisateur;
        int index_bloc;
    };

    kuri::tableau<Connexion> connexions{};
    mutable kuri::table_hachage<Atome const *, kuri::tablet<int, 4>> connexions_pour_inst{""};

  public:
    /* a est utilisé par b */
    void ajoute_connexion(Atome *a, Atome *b, int index_bloc);

    void construit(kuri::tableau<Instruction *, int> const &instructions, int index_bloc);

    bool est_uniquement_utilisé_dans_bloc(Instruction const *inst, int index_bloc) const;

    template <typename Fonction>
    void visite_utilisateurs(Instruction const *inst, Fonction rappel) const;

    void réinitialise();
};

/* Structure pour contenir les différentes structures utilisées pour analyser la RI afin de
 * récupérer la mémoire entre différents appels.
 * Cette structure n'est pas sûre vis-à-vis du moultfilage.
 */
struct ContexteAnalyseRI {
  private:
    Graphe graphe{};
    FonctionEtBlocs fonction_et_blocs{};
    VisiteuseBlocs visiteuse{fonction_et_blocs};

  public:
    void analyse_ri(EspaceDeTravail &espace, ConstructriceRI &constructrice, AtomeFonction *atome);

  private:
    /* Réinitialise les différentes structures. */
    void reinitialise();
};

void marque_instructions_utilisées(kuri::tableau<Instruction *, int> &instructions);
