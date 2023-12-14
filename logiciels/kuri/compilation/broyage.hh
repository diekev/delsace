/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2019 Kévin Dietrich. */

#pragma once

#include "structures/chaine_statique.hh"
#include "structures/enchaineuse.hh"

struct IdentifiantCode;
struct NoeudDeclarationEnteteFonction;
struct NoeudDeclarationType;
using Type = NoeudDeclarationType;

namespace kuri {
template <typename T, uint64_t>
class tablet;
}

/**
 * Une Broyeuse s'occupe de transformer une chaine de caractère en une chaine unique.
 * Cette structure n'est pas sure pour le moultfilage: chaque fil doit avoir sa propre
 * broyeuse.
 */
class Broyeuse {
    /* Stockage pour toutes les chaines. */
    Enchaineuse stockage_chaines{};

    /* Stockage temporaire pour éviter d'allouer trop de chaines.
     * Une fois le broyage terminé, la chaine temporaire est ajoutée au #stockage_chaine.
     */
    Enchaineuse stockage_temp{};

  public:
    /* Retourne le nom broyé de la chaine donnée. */
    kuri::chaine_statique broye_nom_simple(kuri::chaine_statique const &nom);

    /* Retourne le nom broyé de l'identifiant.
     * Le résultat sera mis en cache dans le type. */
    kuri::chaine_statique nom_broyé_type(Type *type);

    /* Retourne le nom broyé de la fonction.
     * Le résultat sera mis en cache dans la fonction. */
    kuri::chaine_statique broye_nom_fonction(
        NoeudDeclarationEnteteFonction *decl,
        const kuri::tablet<IdentifiantCode *, 6> &noms_hiérarchie);

    /* Retourne le nom broyé de l'identifiant.
     * Le résultat sera mis en cache dans l'identifiant. */
    kuri::chaine_statique broye_nom_simple(IdentifiantCode *ident);

    int64_t mémoire_utilisée() const;

  private:
    /* Déplace la chaine du stockage temporaire dans le stockage final, et
     * retourne une chaine statique pour celle-ci. */
    kuri::chaine_statique chaine_finale_pour_stockage_temp();
};
