/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

static bool verifie_transformation(Compilatrice &compilatrice,
                                   Type *type1,
                                   Type *type2,
                                   bool est_possible)
{
    auto tacheronne = Tacheronne(compilatrice);

    auto resultat = cherche_transformation(compilatrice, type1, type2);

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
                                   Typeuse const &typeuse,
                                   TypeBase type1,
                                   TypeBase type2,
                                   bool est_possible)
{
    return verifie_transformation(compilatrice, typeuse[type1], typeuse[type2], est_possible);
}

int main()
{
    auto compilatrice = Compilatrice("");
    auto &typeuse = compilatrice.typeuse;

    auto dt_tabl_fixe = typeuse.type_tableau_fixe(typeuse[TypeBase::Z32], 8);
    auto dt_tabl_dyn = typeuse.type_tableau_dynamique(typeuse[TypeBase::Z32]);

    auto reussite = true;

    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::N8, TypeBase::N8, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::N8, TypeBase::REF_N8, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::REF_N8, TypeBase::N8, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::N8, TypeBase::PTR_N8, false);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::PTR_N8, TypeBase::N8, false);
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::N8, TypeBase::Z8, false);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::N8, TypeBase::REF_Z8, false);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::N8, TypeBase::PTR_Z8, false);
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::N8, TypeBase::N64, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::N8, TypeBase::REF_N64, false);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::N8, TypeBase::CHAINE, false);
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::R64, TypeBase::N8, false);
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::R64, TypeBase::EINI, true);
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::EINI, TypeBase::R64, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::EINI, TypeBase::EINI, true);
    // test []octet -> eini => CONSTRUIT_EINI et non EXTRAIT_TABL_OCTET
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::TABL_OCTET, TypeBase::EINI, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::EINI, TypeBase::TABL_OCTET, true);

    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::PTR_Z8, TypeBase::PTR_NUL, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::PTR_Z8, TypeBase::PTR_RIEN, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::PTR_Z8, TypeBase::PTR_OCTET, true);

    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::PTR_NUL, TypeBase::PTR_Z8, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::PTR_RIEN, TypeBase::PTR_Z8, true);

    // test [4]z32 -> []z32 et [4]z32 -> eini
    reussite &= verifie_transformation(
        compilatrice, typeuse, TypeBase::TABL_N8, TypeBase::TABL_OCTET, true);

    reussite &= verifie_transformation(compilatrice, dt_tabl_fixe, dt_tabl_dyn, true);

    auto dt_eini = typeuse[TypeBase::EINI];

    reussite &= verifie_transformation(compilatrice, dt_tabl_fixe, dt_eini, true);

    auto dt_tabl_octet = typeuse[TypeBase::TABL_OCTET];
    reussite &= verifie_transformation(compilatrice, dt_tabl_fixe, dt_tabl_octet, true);

    /* test : appel fonction */
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::R16, TypeBase::R32, true);
    reussite &= verifie_transformation(compilatrice, typeuse, TypeBase::R32, TypeBase::R16, true);

    // test nul -> fonc()

    return reussite ? 0 : 1;
}
