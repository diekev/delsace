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
#include "programme.hh"
#include "utilitaires/log.hh"

#include "statistiques/statistiques.hh"

#include "structures/chemin_systeme.hh"
#include "structures/enchaineuse.hh"

ArgsGénérationCode crée_args_génération_code(Compilatrice &compilatrice,
                                             EspaceDeTravail &espace,
                                             Programme const *programme,
                                             ProgrammeRepreInter &ri_programme,
                                             Broyeuse &broyeuse)
{
    auto résultat = ArgsGénérationCode{};
    résultat.compilatrice = &compilatrice;
    résultat.espace = &espace;
    résultat.programme = programme;
    résultat.ri_programme = &ri_programme;
    résultat.broyeuse = &broyeuse;
    return résultat;
}

ArgsCréationFichiersObjets crée_args_création_fichier_objet(Compilatrice &compilatrice,
                                                            EspaceDeTravail &espace,
                                                            Programme const *programme,
                                                            ProgrammeRepreInter &ri_programme)
{
    auto résultat = ArgsCréationFichiersObjets{};
    résultat.compilatrice = &compilatrice;
    résultat.espace = &espace;
    résultat.programme = programme;
    résultat.ri_programme = &ri_programme;
    return résultat;
}

ArgsLiaisonObjets crée_args_liaison_objets(Compilatrice &compilatrice,
                                           EspaceDeTravail &espace,
                                           Programme *programme)
{
    auto résultat = ArgsLiaisonObjets{};
    résultat.compilatrice = &compilatrice;
    résultat.espace = &espace;
    résultat.programme = programme;
    return résultat;
}

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

void Coulisse::détruit(Coulisse *coulisse)
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

bool Coulisse::crée_fichier_objet(ArgsGénérationCode const &args)
{
    if (!est_coulisse_métaprogramme()) {
        info() << "Génération du code...";
    }
    auto début_génération_code = dls::chrono::compte_seconde();
    auto err = génère_code_impl(args);
    if (err.has_value()) {
        args.espace->rapporte_erreur_sans_site("Impossible de générer le code machine.")
            .ajoute_message(err.value().message);
        return false;
    }
    temps_génération_code = début_génération_code.temps();

    m_bibliothèques = args.ri_programme->donne_bibliothèques_utilisées();

    if (!est_coulisse_métaprogramme()) {
        info() << "Création du fichier objet...";
    }
    auto args_fichier_objet = crée_args_création_fichier_objet(
        *args.compilatrice, *args.espace, args.programme, *args.ri_programme);
    auto debut_fichier_objet = dls::chrono::compte_seconde();
    err = crée_fichier_objet_impl(args_fichier_objet);
    if (err.has_value()) {
        args.espace->rapporte_erreur_sans_site("Impossible de générer les fichiers objets.")
            .ajoute_message(err.value().message);
        return false;
    }
    temps_fichier_objet = debut_fichier_objet.temps();

    return true;
}

bool Coulisse::crée_exécutable(ArgsLiaisonObjets const &args)
{
    if (!est_coulisse_métaprogramme()) {
        info() << "Liaison du programme...";
    }
    auto début_exécutable = dls::chrono::compte_seconde();
    auto err = crée_exécutable_impl(args);
    if (err.has_value()) {
        args.espace->rapporte_erreur_sans_site("Impossible de générer le compilat.")
            .ajoute_message(err.value().message);
        return false;
    }
    temps_exécutable = début_exécutable.temps();

    return true;
}

void Coulisse::rassemble_statistiques(Statistiques &stats)
{
    auto mémoire_sous_classe = mémoire_utilisée();
    mémoire_sous_classe += m_bibliothèques.taille_mémoire();
    stats.ajoute_mémoire_utilisée("Coulisse", mémoire_sous_classe);
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
    if (ops.résultat == RésultatCompilation::FICHIER_OBJET) {
        return nom_fichier_objet_pour(nom_fichier_sortie(ops));
    }

    return kuri::chaine(chemin_fichier_objet_temporaire_pour("compilation_kuri"));
}

kuri::chaine nom_sortie_résultat_final(OptionsDeCompilation const &ops)
{
    if (ops.résultat == RésultatCompilation::BIBLIOTHÈQUE_DYNAMIQUE) {
        return nom_bibliothèque_dynamique_pour(nom_fichier_sortie(ops),
                                               ops.préfixe_bibliothèque_système);
    }

    if (ops.résultat == RésultatCompilation::BIBLIOTHÈQUE_STATIQUE) {
        return nom_bibliothèque_statique_pour(nom_fichier_sortie(ops),
                                              ops.préfixe_bibliothèque_système);
    }

    if (ops.résultat == RésultatCompilation::FICHIER_OBJET) {
        return nom_fichier_objet_pour(nom_fichier_sortie(ops));
    }

    /* Pour un exécutable nous utilisons le nom donné via les options, en laissant les plateformes
     * gérer le cas où le nom n'est pas renseigné. */
    return nom_executable_pour(ops.nom_sortie);
}
