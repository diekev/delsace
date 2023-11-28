/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#include "coulisse.hh"

#include "biblinternes/chrono/chronometrage.hh"

#include "options.hh"

#include "coulisse_asm.hh"
#include "coulisse_c.hh"
#include "coulisse_llvm.hh"
#include "coulisse_mv.hh"
#include "environnement.hh"
#include "espace_de_travail.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

Coulisse::~Coulisse() = default;

Coulisse *Coulisse::crée_pour_options(OptionsDeCompilation options)
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

Coulisse *Coulisse::crée_pour_metaprogramme()
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

bool Coulisse::crée_fichier_objet(Compilatrice &compilatrice,
                                  EspaceDeTravail &espace,
                                  Programme *programme,
                                  CompilatriceRI &compilatrice_ri,
                                  Broyeuse &broyeuse)
{
    if (!est_coulisse_métaprogramme()) {
        std::cout << "Génération du code..." << std::endl;
    }
    auto début_génération_code = dls::chrono::compte_seconde();
    if (!génère_code_impl(compilatrice, espace, programme, compilatrice_ri, broyeuse)) {
        return false;
    }
    temps_generation_code = début_génération_code.temps();

    if (!est_coulisse_métaprogramme()) {
        std::cout << "Création du fichier objet..." << std::endl;
    }
    auto debut_fichier_objet = dls::chrono::compte_seconde();
    if (!crée_fichier_objet_impl(compilatrice, espace, programme, compilatrice_ri)) {
        espace.rapporte_erreur_sans_site("Impossible de générer les fichiers objets");
        return false;
    }
    temps_fichier_objet = debut_fichier_objet.temps();

    return true;
}

bool Coulisse::crée_exécutable(Compilatrice &compilatrice,
                               EspaceDeTravail &espace,
                               Programme *programme)
{
    if (!est_coulisse_métaprogramme()) {
        std::cout << "Liaison du programme..." << std::endl;
    }
    auto début_exécutable = dls::chrono::compte_seconde();
    if (!crée_exécutable_impl(compilatrice, espace, programme)) {
        espace.rapporte_erreur_sans_site("Ne peut pas créer l'exécutable !");
        return false;
    }
    temps_executable = début_exécutable.temps();

    return true;
}

bool Coulisse::est_coulisse_métaprogramme() const
{
    return dynamic_cast<CoulisseMV const *>(this) != nullptr;
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

    if (ops.resultat == ResultatCompilation::FICHIER_OBJET) {
        return nom_fichier_objet_pour(nom_fichier_sortie(ops));
    }

    /* Pour un exécutable nous utilisons le nom donné via les options, en laissant les plateformes
     * gérer le cas où le nom n'est pas renseigné. */
    return nom_executable_pour(ops.nom_sortie);
}
