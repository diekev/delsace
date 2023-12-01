/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2023 Kévin Dietrich. */

#pragma once

/* Simple bibliothèque de calcul de valeur.
 * Ceci est utilisé principalement pour évaluer les expressions constantes
 * lors de la compilation.
 */

#if defined(__GNUC__)
#    pragma GCC diagnostic push
#    pragma GCC diagnostic ignored "-Wconversion"
#endif

#define GABARIT_OPERATION_ARITHMETIQUE(nom, lexeme_op)                                            \
    struct nom {                                                                                  \
        template <typename T>                                                                     \
        static T applique_opération(T a, T b)                                                     \
        {                                                                                         \
            return a lexeme_op b;                                                                 \
        }                                                                                         \
    }

GABARIT_OPERATION_ARITHMETIQUE(Addition, +);
GABARIT_OPERATION_ARITHMETIQUE(Soustraction, -);
GABARIT_OPERATION_ARITHMETIQUE(Division, /);
GABARIT_OPERATION_ARITHMETIQUE(Multiplication, *);
GABARIT_OPERATION_ARITHMETIQUE(Modulo, %);
GABARIT_OPERATION_ARITHMETIQUE(DécalageGauche, <<);
GABARIT_OPERATION_ARITHMETIQUE(DécalageDroite, >>);
GABARIT_OPERATION_ARITHMETIQUE(DisjonctionBinaire, |);
GABARIT_OPERATION_ARITHMETIQUE(DisjonctionBinaireExclusive, ^);
GABARIT_OPERATION_ARITHMETIQUE(ConjonctionBinaire, &);

#undef GABARIT_OPERATION_ARITHMETIQUE

#define GABARIT_OPERATION_COMPARAISON(nom, lexeme_op)                                             \
    struct nom {                                                                                  \
        template <typename T>                                                                     \
        static bool applique_opération(T a, T b)                                                  \
        {                                                                                         \
            return a lexeme_op b;                                                                 \
        }                                                                                         \
    }

GABARIT_OPERATION_COMPARAISON(Égal, ==);
GABARIT_OPERATION_COMPARAISON(Différent, !=);
GABARIT_OPERATION_COMPARAISON(Supérieur, >);
GABARIT_OPERATION_COMPARAISON(SupérieurÉgal, >=);
GABARIT_OPERATION_COMPARAISON(Inférieur, <);
GABARIT_OPERATION_COMPARAISON(InférieurÉgal, <=);

#undef GABARIT_OPERATION_COMPARAISON

#if defined(__GNUC__)
#    pragma GCC diagnostic pop
#endif
