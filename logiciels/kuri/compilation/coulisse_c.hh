/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "coulisse.hh"

#include "structures/tableau.hh"

struct AtomeFonction;
struct AtomeGlobale;
struct Bibliothèque;
struct DonnéesConstantes;
struct ProgrammeRepreInter;

struct NoeudDéclarationType;
using Type = NoeudDéclarationType;

struct CoulisseC final : public Coulisse {
    struct FichierC {
        kuri::chaine chemin_fichier{};
        kuri::chaine chemin_fichier_objet{};
        kuri::chaine chemin_fichier_erreur_objet{};

        const DonnéesConstantes *données_constantes = nullptr;
        kuri::tableau_statique<Type *> types{};
        kuri::tableau_statique<AtomeGlobale *> globales{};
        kuri::tableau_statique<AtomeFonction *> fonctions{};
        kuri::tableau_statique<AtomeFonction *> fonctions_enlignées{};

        bool est_entête = false;
    };

  private:
    kuri::tableau<FichierC> m_fichiers{};

    int64_t m_mémoire_génératrice = 0;

    std::optional<ErreurCoulisse> génère_code_impl(ArgsGénérationCode const &args) override;

    std::optional<ErreurCoulisse> crée_fichier_objet_impl(
        ArgsCréationFichiersObjets const &args) override;

    std::optional<ErreurCoulisse> crée_exécutable_impl(ArgsLiaisonObjets const &args) override;

    int64_t mémoire_utilisée() const override;

    void crée_fichiers(ProgrammeRepreInter const &repr_inter, const EspaceDeTravail &espace);

    FichierC &ajoute_fichier_c(kuri::chaine_statique nom_espace, bool entête);
};
