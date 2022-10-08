/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse.hh"

#include "options.hh"

#include "coulisse_asm.hh"
#include "coulisse_c.hh"
#include "coulisse_llvm.hh"
#include "coulisse_mv.hh"

#include "structures/enchaineuse.hh"

Coulisse *Coulisse::cree_pour_options(OptionsDeCompilation options)
{
    switch (options.coulisse) {
        case TypeCoulisse::C:
        {
            return memoire::loge<CoulisseC>("CoulisseC");
        }
        case TypeCoulisse::LLVM:
        {
#ifdef AVEC_LLVM
            return memoire::loge<CoulisseLLVM>("CoulisseLLVM");
#else
            return nullptr;
#endif
        }
        case TypeCoulisse::ASM:
        {
            return memoire::loge<CoulisseASM>("CoulisseASM");
        }
    }

    assert(false);
    return nullptr;
}

Coulisse *Coulisse::cree_pour_metaprogramme()
{
    return memoire::loge<CoulisseMV>("CoulisseMV");
}

void Coulisse::detruit(Coulisse *coulisse)
{
    if (dynamic_cast<CoulisseC *>(coulisse)) {
        auto c = dynamic_cast<CoulisseC *>(coulisse);
        memoire::deloge("CoulisseC", c);
        coulisse = nullptr;
    }
#ifdef AVEC_LLVM
    else if (dynamic_cast<CoulisseLLVM *>(coulisse)) {
        auto c = dynamic_cast<CoulisseLLVM *>(coulisse);
        memoire::deloge("CoulisseLLVM", c);
        coulisse = nullptr;
    }
#endif
    else if (dynamic_cast<CoulisseASM *>(coulisse)) {
        auto c = dynamic_cast<CoulisseASM *>(coulisse);
        memoire::deloge("CoulisseASM", c);
        coulisse = nullptr;
    }
    else if (dynamic_cast<CoulisseMV *>(coulisse)) {
        auto c = dynamic_cast<CoulisseMV *>(coulisse);
        memoire::deloge("CoulisseMV", c);
        coulisse = nullptr;
    }
}

kuri::chaine nom_sortie_fichier_objet(OptionsDeCompilation const &ops)
{
    if (ops.resultat == ResultatCompilation::FICHIER_OBJET) {
        if (ops.nom_sortie == "") {
            return "a.o";
        }

        return enchaine(ops.nom_sortie, ".o");
    }

    return "/tmp/compilation_kuri.o";
}

kuri::chaine nom_sortie_resultat_final(OptionsDeCompilation const &ops)
{
    if (ops.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        if (ops.nom_sortie == "") {
            return "a.so";
        }

        return enchaine(ops.nom_sortie, ".so");
    }

    if (ops.resultat == ResultatCompilation::BIBLIOTHEQUE_STATIQUE) {
        if (ops.nom_sortie == "") {
            return "a.a";
        }

        return enchaine(ops.nom_sortie, ".a");
    }

    if (ops.nom_sortie == "") {
        return "a.out";
    }

    return ops.nom_sortie;
}
