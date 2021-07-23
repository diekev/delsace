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

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

static bool verifie_transformation(Compilatrice &compilatrice,
                                   EspaceDeTravail *espace,
                                   Type *type1,
                                   Type *type2,
                                   bool est_possible)
{
    auto unite = UniteCompilation(espace);
    auto tacheronne = Tacheronne(compilatrice);

    auto contexte = ContexteValidationCode(compilatrice, tacheronne, unite);

    auto resultat = cherche_transformation(*espace, contexte, type1, type2);

    auto transformation = TransformationType();
    if (std::holds_alternative<TransformationType>(resultat)) {
        transformation = std::get<TransformationType>(resultat);
    }
    // ignore si nous avons une Attente dans le résultat

    if (est_possible && transformation.type == TypeTransformation::IMPOSSIBLE) {
        std::cerr << "ERREUR la transformation entre ";
        std::cerr << chaine_type(type1);
        std::cerr << " et ";
        std::cerr << chaine_type(type2);
        std::cerr << " doit être possible\n";
        return false;
    }

    if (!est_possible && transformation.type != TypeTransformation::IMPOSSIBLE) {
        std::cerr << "ERREUR la transformation entre ";
        std::cerr << chaine_type(type1);
        std::cerr << " et ";
        std::cerr << chaine_type(type2);
        std::cerr << " doit être impossible\n";
        return false;
    }

    if (transformation.type == TypeTransformation::INUTILE) {
        // std::cerr << "La transformation est inutile\n";
        return true;
    }

    if (transformation.type == TypeTransformation::IMPOSSIBLE) {
        // std::cerr << "La transformation est impossible\n";
        return true;
    }

    std::cerr << "Pour transformer ";
    std::cerr << chaine_type(type1);
    std::cerr << " en ";
    std::cerr << chaine_type(type2);
    std::cerr << ", il faut faire : ";
    std::cerr << chaine_transformation(transformation.type);

    if (transformation.type == TypeTransformation::FONCTION) {
        std::cerr << " (" << transformation.fonction->lexeme->chaine << ')';
    }

    std::cerr << '\n';
    return true;
}

static bool verifie_transformation(Compilatrice &compilatrice,
                                   EspaceDeTravail *espace,
                                   Typeuse const &typeuse,
                                   TypeBase type1,
                                   TypeBase type2,
                                   bool est_possible)
{
    return verifie_transformation(
        compilatrice, espace, typeuse[type1], typeuse[type2], est_possible);
}

int main()
{
    auto compilatrice = Compilatrice();
    auto espace = compilatrice.demarre_un_espace_de_travail({}, "");
    auto &typeuse = espace->typeuse;

    auto dt_tabl_fixe = typeuse.type_tableau_fixe(typeuse[TypeBase::Z32], 8);
    auto dt_tabl_dyn = typeuse.type_tableau_dynamique(typeuse[TypeBase::Z32]);

    auto reussite = true;

    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::N8, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::REF_N8, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::REF_N8, TypeBase::N8, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::PTR_N8, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::PTR_N8, TypeBase::N8, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::Z8, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::REF_Z8, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::PTR_Z8, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::N64, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::REF_N64, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::N8, TypeBase::CHAINE, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::R64, TypeBase::N8, false);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::R64, TypeBase::EINI, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::EINI, TypeBase::R64, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::EINI, TypeBase::EINI, true);
    // test []octet -> eini => CONSTRUIT_EINI et non EXTRAIT_TABL_OCTET
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::TABL_OCTET, TypeBase::EINI, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::EINI, TypeBase::TABL_OCTET, true);

    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::PTR_Z8, TypeBase::PTR_NUL, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::PTR_Z8, TypeBase::PTR_RIEN, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::PTR_Z8, TypeBase::PTR_OCTET, true);

    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::PTR_NUL, TypeBase::PTR_Z8, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::PTR_RIEN, TypeBase::PTR_Z8, true);

    // test [4]z32 -> []z32 et [4]z32 -> eini
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::TABL_N8, TypeBase::TABL_OCTET, true);

    reussite &= verifie_transformation(compilatrice, espace, dt_tabl_fixe, dt_tabl_dyn, true);

    auto dt_eini = typeuse[TypeBase::EINI];

    reussite &= verifie_transformation(compilatrice, espace, dt_tabl_fixe, dt_eini, true);

    auto dt_tabl_octet = typeuse[TypeBase::TABL_OCTET];
    reussite &= verifie_transformation(compilatrice, espace, dt_tabl_fixe, dt_tabl_octet, true);

    /* test : appel fonction */
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::R16, TypeBase::R32, true);
    reussite &= verifie_transformation(
        compilatrice, espace, typeuse, TypeBase::R32, TypeBase::R16, true);

    // test nul -> fonc()

    return reussite ? 0 : 1;
}
