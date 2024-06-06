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
 * The Original Code is Copyright (C) 2015 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#include "expression.h"

#include <iostream>

#include "biblinternes/structures/pile.hh"

#include "manipulable.h"
#include "morceaux.h"

namespace danjo {

bool est_operateur(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::PLUS:
        case id_morceau::DIVISE:
        case id_morceau::FOIS:
        case id_morceau::MOINS:
        case id_morceau::EGALITE:
        case id_morceau::DIFFERENCE:
        case id_morceau::INFERIEUR:
        case id_morceau::SUPERIEUR:
        case id_morceau::INFERIEUR_EGAL:
        case id_morceau::SUPERIEUR_EGAL:
        case id_morceau::ESPERLUETTE:
        case id_morceau::BARRE:
        case id_morceau::CHAPEAU:
            return true;
        default:
            return false;
    }
}

auto est_operateur_logique(id_morceau identifiant)
{
    switch (identifiant) {
        case id_morceau::TILDE:
            return true;
        default:
            return false;
    }
}

static id_morceau promeut(id_morceau id1, id_morceau id2)
{
    if (id1 == id2) {
        return id1;
    }

    return id_morceau::NOMBRE_REEL;
}

#define DEFINI_FONCTION(__nom, __op)                                                              \
    template <typename T1, typename T2 = T1>                                                      \
    Symbole __nom(const Symbole &s1, const Symbole &s2)                                           \
    {                                                                                             \
        Symbole ret;                                                                              \
        ret.valeur = std::any_cast<T1>(s1.valeur)                                                 \
            __op static_cast<T1>(std::any_cast<T2>(s2.valeur));                                   \
        ret.genre = promeut(s1.genre, s2.genre);                                                  \
        return ret;                                                                               \
    }

DEFINI_FONCTION(additionne, +)
DEFINI_FONCTION(soustrait, -)
DEFINI_FONCTION(divise, /)
DEFINI_FONCTION(multiplie, *)
DEFINI_FONCTION(compare_egalite, ==)
DEFINI_FONCTION(compare_difference, !=)
DEFINI_FONCTION(compare_inferiorite, <)
DEFINI_FONCTION(compare_superiorite, >)
DEFINI_FONCTION(compare_inf_egal, <=)
DEFINI_FONCTION(compare_sup_egal, >=)
DEFINI_FONCTION(octet_et, &)
DEFINI_FONCTION(octet_ou, |)
DEFINI_FONCTION(octet_oux, ^)

#define DEFINI_CAS_SIMPLE(__id, __fonction)                                                       \
    case __id:                                                                                    \
        if (s1.genre == id_morceau::NOMBRE_ENTIER) {                                              \
            return __fonction<int>(s1, s2);                                                       \
        }                                                                                         \
        if (s1.genre == id_morceau::NOMBRE_REEL) {                                                \
            return __fonction<float>(s1, s2);                                                     \
        }                                                                                         \
        if (s1.genre == id_morceau::BOOL) {                                                       \
            return __fonction<bool>(s1, s2);                                                      \
        }                                                                                         \
        break;

#define DEFINI_CAS_DOUBLE(__id, __fonction)                                                       \
    case __id:                                                                                    \
        if (s1.genre == id_morceau::NOMBRE_ENTIER && s2.genre == id_morceau::NOMBRE_REEL) {       \
            return __fonction<int, float>(s1, s2);                                                \
        }                                                                                         \
        if (s1.genre == id_morceau::NOMBRE_REEL && s2.genre == id_morceau::NOMBRE_ENTIER) {       \
            return __fonction<float, int>(s1, s2);                                                \
        }                                                                                         \
        break;

Symbole evalue_operation(const Symbole &s1, const Symbole &s2, id_morceau operation)
{
    if (s1.genre == s2.genre) {
        switch (operation) {
            default:
                break;
                DEFINI_CAS_SIMPLE(id_morceau::PLUS, additionne);
                DEFINI_CAS_SIMPLE(id_morceau::MOINS, soustrait);
                DEFINI_CAS_SIMPLE(id_morceau::DIVISE, divise);
                DEFINI_CAS_SIMPLE(id_morceau::FOIS, multiplie);
                DEFINI_CAS_SIMPLE(id_morceau::EGALITE, compare_egalite);
                DEFINI_CAS_SIMPLE(id_morceau::DIFFERENCE, compare_difference);
                DEFINI_CAS_SIMPLE(id_morceau::INFERIEUR, compare_inferiorite);
                DEFINI_CAS_SIMPLE(id_morceau::SUPERIEUR, compare_superiorite);
                DEFINI_CAS_SIMPLE(id_morceau::INFERIEUR_EGAL, compare_inf_egal);
                DEFINI_CAS_SIMPLE(id_morceau::SUPERIEUR_EGAL, compare_sup_egal);
        }
    }
    else {
        switch (operation) {
            default:
                break;
                DEFINI_CAS_DOUBLE(id_morceau::PLUS, additionne);
                DEFINI_CAS_DOUBLE(id_morceau::MOINS, soustrait);
                DEFINI_CAS_DOUBLE(id_morceau::DIVISE, divise);
                DEFINI_CAS_DOUBLE(id_morceau::FOIS, multiplie);
                DEFINI_CAS_DOUBLE(id_morceau::EGALITE, compare_egalite);
                DEFINI_CAS_DOUBLE(id_morceau::DIFFERENCE, compare_difference);
                DEFINI_CAS_DOUBLE(id_morceau::INFERIEUR, compare_inferiorite);
                DEFINI_CAS_DOUBLE(id_morceau::SUPERIEUR, compare_superiorite);
                DEFINI_CAS_DOUBLE(id_morceau::INFERIEUR_EGAL, compare_inf_egal);
                DEFINI_CAS_DOUBLE(id_morceau::SUPERIEUR_EGAL, compare_sup_egal);
        }
    }

    return {};
}

auto evalue_operation_logique(const Symbole &s1, id_morceau identifiant)
{
    Symbole resultat;
    resultat.genre = id_morceau::NOMBRE_ENTIER;
    resultat.valeur = 0;

    auto op1 = std::any_cast<int>(s1.valeur);

    switch (identifiant) {
        case id_morceau::TILDE:
            resultat.valeur = ~op1;
            break;
        default:
            break;
    }

    return resultat;
}

enum {
    GAUCHE,
    DROITE,
};

static std::pair<int, int> associativite(id_morceau identifiant)
{
    switch (identifiant) {
        default:
            break;
        case id_morceau::PLUS:
        case id_morceau::MOINS:
            return {GAUCHE, 0};
        case id_morceau::FOIS:
        case id_morceau::DIVISE:
            /* À FAIRE : modulo */
            return {GAUCHE, 1};
#if 0
		case id_morceau::PUISSANCE:
			return { DROITE, 2};
#endif
    }

    return {GAUCHE, 0};
}

bool precedence_faible(id_morceau identifiant1, id_morceau identifiant2)
{
    auto p1 = associativite(identifiant1);
    auto p2 = associativite(identifiant2);

    return (p1.first == GAUCHE && p1.second <= p2.second) ||
           ((p2.first == DROITE) && (p1.second < p2.second));
}

Symbole evalue_expression(const dls::tableau<Symbole> &expression, Manipulable *manipulable)
{
    dls::pile<Symbole> pile;

    /* Pousse un zéro sur la pile si jamais l'expression est vide ou démarre
     * avec un nombre négatif. */
    pile.empile({std::any(0), id_morceau::NOMBRE_ENTIER});

    for (const Symbole &symbole : expression) {
        if (est_operateur(symbole.genre)) {
            auto s2 = pile.depile();
            auto s1 = pile.depile();

            auto resultat = evalue_operation(s1, s2, symbole.genre);
            pile.empile(resultat);
        }
        else if (est_operateur_logique(symbole.genre)) {
            auto s1 = pile.depile();

            auto resultat = evalue_operation_logique(s1, symbole.genre);
            pile.empile(resultat);
        }
        else {
            switch (symbole.genre) {
                default:
                    break;
                case id_morceau::BOOL:
                case id_morceau::NOMBRE_ENTIER:
                case id_morceau::NOMBRE_REEL:
                case id_morceau::CHAINE_LITTERALE:
                case id_morceau::COULEUR:
                case id_morceau::VECTEUR:
                {
                    pile.empile(symbole);
                    break;
                }
                case id_morceau::CHAINE_CARACTERE:
                {
                    auto nom = std::any_cast<dls::chaine>(symbole.valeur);

                    Symbole tmp;

                    switch (manipulable->type_propriete(nom)) {
                        case TypePropriete::ENTIER:
                        {
                            tmp.valeur = manipulable->evalue_entier(nom);
                            tmp.genre = id_morceau::NOMBRE_ENTIER;
                            break;
                        }
                        case TypePropriete::DECIMAL:
                        {
                            tmp.valeur = manipulable->evalue_decimal(nom);
                            tmp.genre = id_morceau::NOMBRE_REEL;
                            break;
                        }
                        case TypePropriete::BOOL:
                        {
                            tmp.valeur = manipulable->evalue_bool(nom);
                            tmp.genre = id_morceau::BOOL;
                            break;
                        }
                        case TypePropriete::COULEUR:
                        {
                            tmp.valeur = manipulable->evalue_couleur(nom);
                            tmp.genre = id_morceau::COULEUR;
                            break;
                        }
                        case TypePropriete::VECTEUR_DECIMAL:
                        {
                            tmp.valeur = manipulable->evalue_vecteur(nom);
                            tmp.genre = id_morceau::VECTEUR;
                            break;
                        }
                        case TypePropriete::VECTEUR_ENTIER:
                        {
                            // À FAIRE
                            break;
                        }
                        case TypePropriete::ENUM:
                        case TypePropriete::FICHIER_ENTREE:
                        case TypePropriete::FICHIER_SORTIE:
                        case TypePropriete::DOSSIER:
                        case TypePropriete::CHAINE_CARACTERE:
                        {
                            tmp.valeur = manipulable->evalue_chaine(nom);
                            tmp.genre = id_morceau::CHAINE_LITTERALE;
                            break;
                        }
                        default:
                        {
                            std::cerr << "Le type de propriété n'est pas supporté !\n";
                            break;
                        }
                    }

                    pile.empile(tmp);
                    break;
                }
            }
        }
    }

    return pile.haut();
}

void imprime_valeur_symbole(Symbole symbole, std::ostream &os)
{
    switch (symbole.genre) {
        case id_morceau::NOMBRE_ENTIER:
            os << std::any_cast<int>(symbole.valeur) << ' ';
            break;
        case id_morceau::NOMBRE_REEL:
            os << std::any_cast<float>(symbole.valeur) << ' ';
            break;
        case id_morceau::BOOL:
            os << std::any_cast<bool>(symbole.valeur) << ' ';
            break;
        default:
            os << std::any_cast<dls::chaine>(symbole.valeur) << ' ';
            break;
    }
}

} /* namespace danjo */
