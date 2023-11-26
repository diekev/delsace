/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#include "ssa.hh"

#include "structures/ensemble.hh"

// https://pp.ipd.kit.edu/uploads/publikationen/braun13cc.pdf

class ConvertisseuseSSA {

    struct Bloc {
        kuri::tableau<Bloc *> précédents{};  // ancêtre
    };
    struct Valeur {
    };

    struct NoeudPhi : public Valeur {
        Bloc *bloc = nullptr;
        kuri::tableau<Valeur *> opérandes{};
        kuri::tableau<Valeur *> users{};

        void appendOperand(Valeur *valeur)
        {
            opérandes.ajoute(valeur);
        }
    };
    // À FAIRE : utilise drapeau
    kuri::ensemble<Bloc *> m_sealed_blocks{};

  public:
    // algorithme 1 : local value numbering

    void writeVariable(void *variable, Bloc *bloc, Valeur *valeur)
    {
        //  À FAIRE : currentDef[variable][bloc] = valeur
    }

    Valeur *readVariable(void *variable, Bloc *bloc)
    {
        //  À FAIRE : if (currentDef[variable] contains bloc) {
        // // Local value numbering
        // return currentDef[variable][bloc];
        // }

        return readVariableRecursive(variable, bloc);
    }

    // algorithme 2 : global value numbering
    Valeur *readVariableRecursive(void *variable, Bloc *bloc)
    {
        Valeur *résultat = nullptr;

        if (!m_sealed_blocks.possède(bloc)) {
            /* Graphe de controle incomplet. */
            résultat = crée_noeud_phi(bloc);
            //  À FAIRE : incompletePhis[bloc][variable].ajoute(résultat);
        }
        else if (bloc->précédents.taille() == 1) {
            /* Optimisation du cas commun d'un ancêtre : aucun Phi n'est nécessaire. */
            résultat = readVariable(variable, bloc->précédents[0]);
        }
        else {
            /* Brise les cycles potentiels avec des phis sans-opérandes. */
            auto phi = crée_noeud_phi(bloc);
            writeVariable(variable, bloc, phi);
            résultat = addPhiOperands(variable, phi);
        }

        writeVariable(variable, bloc, résultat);
        return résultat;
    }

    Valeur *addPhiOperands(void *variable, NoeudPhi *phi)
    {
        POUR (phi->bloc->précédents) {
            phi->appendOperand(readVariable(variable, it));
        }

        return tryRemoveTrivialPhi(phi);
    }

    // algorithm 3 : détection et suppression récursive des phi triviaux

    Valeur *tryRemoveTrivialPhi(NoeudPhi *phi)
    {
        Valeur *same = nullptr;

        POUR_NOMME (op, phi->opérandes) {
            if (op == same || op == phi) {
                /* Valeur unique ou auto-référence. */
                continue;
            }

            if (same != nullptr) {
                /* Le phi fusionne au moins 2 valeurs : non-trivial. */
                return phi;
            }

            same = op;
        }

        if (same == nullptr) {
            /* Le phi est inatteignable ou dans le bloc de début. */
            same = new Undef();
        }

        /* Remémore tous les utilisateurs sauf le phi lui-même. */
        auto users = phi->users.remove(phi);

        /* Dévie toutes les utilisations de phi vers same et supprime phi. */
        phi->replaceBy(same);

        /* Essaie de supprimer tous les utilisateurs de phi, qui peuvent être devenus triviaux. */
        POUR_NOMME (use, phi->users) {
            if (use->est_un<NoeudPhi>()) {
                tryRemoveTrivialPhi(use->comme<NoeudPhi>());
            }
        }

        return same;
    }

    // --------------------------------------------

    NoeudPhi *crée_noeud_phi(Bloc *bloc)
    {
        auto résultat = new NoeudPhi;
        résultat->bloc = bloc;
        return résultat;
    }
};
