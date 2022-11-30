/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse.hh"

#include "options.hh"

#include "coulisse_asm.hh"
#include "coulisse_c.hh"
#include "coulisse_llvm.hh"
#include "coulisse_mv.hh"
#include "environnement.hh"

#include "structures/chemin_systeme.hh"
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

static kuri::chaine_statique nom_fichier_sortie(OptionsDeCompilation const &ops)
{
    if (ops.nom_sortie == "") {
        return "a";
    }

    return ops.nom_sortie;
}

kuri::chaine nom_sortie_fichier_objet(OptionsDeCompilation const &ops)
{
    if (ops.resultat == ResultatCompilation::FICHIER_OBJET) {
        return nom_fichier_objet_pour(nom_fichier_sortie(ops));
    }

    return kuri::chaine(chemin_fichier_objet_temporaire_pour("compilation_kuri"));
}

kuri::chaine nom_sortie_resultat_final(OptionsDeCompilation const &ops)
{
    if (ops.resultat == ResultatCompilation::BIBLIOTHEQUE_DYNAMIQUE) {
        return nom_bibliothèque_dynamique_pour(nom_fichier_sortie(ops));
    }

    if (ops.resultat == ResultatCompilation::BIBLIOTHEQUE_STATIQUE) {
        return nom_bibliothèque_statique_pour(nom_fichier_sortie(ops));
    }

    /* Pour un exécutable nous utilisons le nom donné via les options, en laissant les plateformes
     * gérer le cas où le nom n'est pas renseigné. */
    return nom_executable_pour(ops.nom_sortie);
}
