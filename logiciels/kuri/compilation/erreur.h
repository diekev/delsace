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
 * The Original Code is Copyright (C) 2018 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include "biblinternes/structures/ensemble.hh"

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"

#include "validation_expression_appel.hh"

struct EspaceDeTravail;
struct Fichier;
struct Lexeme;
struct NoeudExpression;
struct Type;
struct TypeCompose;

namespace erreur {

#define ENUMERE_GENRES_ERREUR                                                                     \
    ENUMERE_GENRE_ERREUR_EX(AUCUNE_ERREUR)                                                        \
    ENUMERE_GENRE_ERREUR_EX(NORMAL)                                                               \
    ENUMERE_GENRE_ERREUR_EX(LEXAGE)                                                               \
    ENUMERE_GENRE_ERREUR_EX(SYNTAXAGE)                                                            \
    ENUMERE_GENRE_ERREUR_EX(NOMBRE_ARGUMENT)                                                      \
    ENUMERE_GENRE_ERREUR_EX(TYPE_ARGUMENT)                                                        \
    ENUMERE_GENRE_ERREUR_EX(ARGUMENT_INCONNU)                                                     \
    ENUMERE_GENRE_ERREUR_EX(ARGUMENT_REDEFINI)                                                    \
    ENUMERE_GENRE_ERREUR_EX(VARIABLE_INCONNUE)                                                    \
    ENUMERE_GENRE_ERREUR_EX(VARIABLE_REDEFINIE)                                                   \
    ENUMERE_GENRE_ERREUR_EX(FONCTION_INCONNUE)                                                    \
    ENUMERE_GENRE_ERREUR_EX(FONCTION_REDEFINIE)                                                   \
    ENUMERE_GENRE_ERREUR_EX(ASSIGNATION_RIEN)                                                     \
    ENUMERE_GENRE_ERREUR_EX(TYPE_INCONNU)                                                         \
    ENUMERE_GENRE_ERREUR_EX(TYPE_DIFFERENTS)                                                      \
    ENUMERE_GENRE_ERREUR_EX(STRUCTURE_INCONNUE)                                                   \
    ENUMERE_GENRE_ERREUR_EX(STRUCTURE_REDEFINIE)                                                  \
    ENUMERE_GENRE_ERREUR_EX(MEMBRE_INCONNU)                                                       \
    ENUMERE_GENRE_ERREUR_EX(MEMBRE_INACTIF)                                                       \
    ENUMERE_GENRE_ERREUR_EX(MEMBRE_REDEFINI)                                                      \
    ENUMERE_GENRE_ERREUR_EX(ASSIGNATION_INVALIDE)                                                 \
    ENUMERE_GENRE_ERREUR_EX(ASSIGNATION_MAUVAIS_TYPE)                                             \
    ENUMERE_GENRE_ERREUR_EX(CONTROLE_INVALIDE)                                                    \
    ENUMERE_GENRE_ERREUR_EX(MODULE_INCONNU)                                                       \
    ENUMERE_GENRE_ERREUR_EX(APPEL_INVALIDE)                                                       \
    ENUMERE_GENRE_ERREUR_EX(AVERTISSEMENT)

enum class Genre : int {
#define ENUMERE_GENRE_ERREUR_EX(type) type,
    ENUMERE_GENRES_ERREUR
#undef ENUMERE_GENRE_ERREUR_EX
};

const char *chaine_erreur(Genre genre);
std::ostream &operator<<(std::ostream &os, Genre genre);

void imprime_site(EspaceDeTravail const &espace, NoeudExpression const *site);

void lance_erreur(const kuri::chaine &quoi,
                  EspaceDeTravail const &espace,
                  const NoeudExpression *site,
                  Genre type = Genre::NORMAL);

void redefinition_fonction(EspaceDeTravail const &espace,
                           const NoeudExpression *site_redefinition,
                           const NoeudExpression *site_original);

void redefinition_symbole(EspaceDeTravail const &espace,
                          const NoeudExpression *site_redefinition,
                          const NoeudExpression *site_original);

void lance_erreur_transtypage_impossible(const Type *type_cible,
                                         const Type *type_enf,
                                         EspaceDeTravail const &espace,
                                         const NoeudExpression *site_expression,
                                         const NoeudExpression *site);

void lance_erreur_assignation_type_differents(const Type *type_gauche,
                                              const Type *type_droite,
                                              EspaceDeTravail const &espace,
                                              const NoeudExpression *site);

void lance_erreur_type_operation(const Type *type_gauche,
                                 const Type *type_droite,
                                 EspaceDeTravail const &espace,
                                 const NoeudExpression *site);

void lance_erreur_fonction_inconnue(EspaceDeTravail const &espace,
                                    NoeudExpression *n,
                                    dls::tablet<DonneesCandidate, 10> const &candidates);

void lance_erreur_fonction_nulctx(EspaceDeTravail const &espace,
                                  NoeudExpression const *appl_fonc,
                                  NoeudExpression const *decl_fonc,
                                  NoeudExpression const *decl_appel);

void lance_erreur_acces_hors_limites(EspaceDeTravail const &espace,
                                     NoeudExpression *b,
                                     long taille_tableau,
                                     Type *type_tableau,
                                     long index_acces);

void membre_inconnu(EspaceDeTravail const &espace,
                    NoeudExpression *acces,
                    NoeudExpression *structure,
                    NoeudExpression *membre,
                    TypeCompose *type);

void valeur_manquante_discr(EspaceDeTravail const &espace,
                            NoeudExpression *expression,
                            const dls::ensemble<kuri::chaine_statique> &valeurs_manquantes);

void fonction_principale_manquante(EspaceDeTravail const &espace);
}  // namespace erreur

struct Erreur {
    EspaceDeTravail const *espace = nullptr;
    bool fut_bougee = false;
    Enchaineuse enchaineuse{};
    erreur::Genre genre = erreur::Genre::NORMAL;

    Erreur(EspaceDeTravail const *espace_);

    Erreur(Erreur &) = delete;
    Erreur &operator=(Erreur &) = delete;

    Erreur(Erreur &&autre)
    {
        this->permute(autre);
    }

    Erreur &operator=(Erreur &&autre)
    {
        this->permute(autre);
        return *this;
    }

    ~Erreur() noexcept(false);

    Erreur &ajoute_message(kuri::chaine const &m);

    template <typename... Ts>
    Erreur &ajoute_message(Ts... ts)
    {
        ((enchaineuse << ts), ...);
        return *this;
    }

    Erreur &ajoute_site(NoeudExpression const *site);

    Erreur &ajoute_conseil(kuri::chaine const &c);

    void genre_erreur(erreur::Genre genre_)
    {
        genre = genre_;
    }

    void permute(Erreur &autre)
    {
        enchaineuse.permute(autre.enchaineuse);
        std::swap(genre, autre.genre);
        std::swap(espace, autre.espace);

        autre.fut_bougee = true;
    }
};

Erreur rapporte_erreur(EspaceDeTravail const *espace,
                       NoeudExpression const *site,
                       kuri::chaine const &message,
                       erreur::Genre genre = erreur::Genre::NORMAL);

Erreur rapporte_erreur_sans_site(EspaceDeTravail const *espace,
                                 const kuri::chaine &message,
                                 erreur::Genre genre = erreur::Genre::NORMAL);

Erreur rapporte_erreur(EspaceDeTravail const *espace,
                       const kuri::chaine &fichier,
                       int ligne,
                       const kuri::chaine &message);

kuri::chaine genere_entete_erreur(EspaceDeTravail const *espace,
                                  NoeudExpression const *site,
                                  erreur::Genre genre,
                                  const kuri::chaine_statique message);

kuri::chaine genere_entete_erreur(EspaceDeTravail const *espace,
                                  const Fichier *fichier,
                                  int ligne,
                                  erreur::Genre genre,
                                  const kuri::chaine_statique message);
