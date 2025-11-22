/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2020 Kévin Dietrich. */

#include <iostream>

#include "compilation/compilatrice.hh"
#include "compilation/espace_de_travail.hh"

static bool verifie_transformation(Compilatrice &compilatrice,
                                   Type *type1,
                                   Type *type2,
                                   bool est_possible)
{
    auto tacheronne = Tacheronne(compilatrice);

    auto résultat = cherche_transformation(type1, type2);

    auto transformation = TransformationType();
    if (std::holds_alternative<TransformationType>(résultat)) {
        transformation = std::get<TransformationType>(résultat);
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

    std::cerr << '\n';
    return true;
}

int main()
{
    auto arguments = ArgumentsCompilatrice{};
    arguments.importe_kuri = false;
    auto compilatrice = Compilatrice("", arguments);
    auto espace = EspaceDeTravail(compilatrice, {}, "");
    auto &typeuse = espace.typeuse;

    auto dt_tabl_fixe = typeuse.type_tableau_fixe(typeuse.type_z32, 8);
    auto dt_tranche = typeuse.crée_type_tranche(typeuse.type_z32);

    auto reussite = true;

    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_n8, true);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_ref_n8, true);
    reussite &= verifie_transformation(compilatrice, typeuse.type_ref_n8, typeuse.type_n8, true);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_ptr_n8, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_ptr_n8, typeuse.type_n8, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_z8, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_ref_z8, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_ptr_z8, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_n64, true);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_ref_n64, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_n8, typeuse.type_chaine, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_r64, typeuse.type_n8, false);
    reussite &= verifie_transformation(compilatrice, typeuse.type_r64, typeuse.type_eini, true);
    reussite &= verifie_transformation(compilatrice, typeuse.type_eini, typeuse.type_r64, true);
    reussite &= verifie_transformation(compilatrice, typeuse.type_eini, typeuse.type_eini, true);
    // test []octet -> eini => CONSTRUIS_EINI et non EXTRAIT_TABL_OCTET
    reussite &= verifie_transformation(
        compilatrice, typeuse.type_tranche_octet, typeuse.type_eini, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse.type_eini, typeuse.type_tranche_octet, true);

    reussite &= verifie_transformation(
        compilatrice, typeuse.type_ptr_z8, typeuse.type_ptr_nul, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse.type_ptr_z8, typeuse.type_ptr_rien, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse.type_ptr_z8, typeuse.type_ptr_octet, true);

    reussite &= verifie_transformation(
        compilatrice, typeuse.type_ptr_nul, typeuse.type_ptr_z8, true);
    reussite &= verifie_transformation(
        compilatrice, typeuse.type_ptr_rien, typeuse.type_ptr_z8, true);

    reussite &= verifie_transformation(
        compilatrice, typeuse.type_tabl_n8, typeuse.type_tranche_octet, true);

    reussite &= verifie_transformation(compilatrice, dt_tabl_fixe, dt_tranche, true);

    auto dt_eini = typeuse.type_eini;

    reussite &= verifie_transformation(compilatrice, dt_tabl_fixe, dt_eini, true);

    auto dt_tabl_octet = typeuse.type_tranche_octet;
    reussite &= verifie_transformation(compilatrice, dt_tabl_fixe, dt_tabl_octet, true);

    /* test : appel fonction */
    reussite &= verifie_transformation(compilatrice, typeuse.type_r16, typeuse.type_r32, true);

    // test nul -> fonc()

    return reussite ? 0 : 1;
}
