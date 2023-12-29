/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2018 Kévin Dietrich. */

#pragma once

#include "structures/chaine.hh"
#include "structures/enchaineuse.hh"
#include "structures/ensemble.hh"

#include "validation_expression_appel.hh"

struct EspaceDeTravail;
struct NoeudExpression;
struct SiteSource;
struct NoeudDeclarationType;
struct NoeudDeclarationTypeCompose;
using Type = NoeudDeclarationType;
using TypeCompose = NoeudDeclarationTypeCompose;

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

void imprime_site(Enchaineuse &enchaineuse,
                  const EspaceDeTravail &espace,
                  const NoeudExpression *site);
[[nodiscard]] kuri::chaine imprime_site(EspaceDeTravail const &espace,
                                        NoeudExpression const *site);

kuri::chaine_statique chaine_expression(EspaceDeTravail const &espace,
                                        const NoeudExpression *expr);

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
                                    NoeudExpression const *n,
                                    kuri::tablet<ErreurAppariement, 10> const &erreurs);

void lance_erreur_fonction_nulctx(EspaceDeTravail const &espace,
                                  NoeudExpression const *appl_fonc,
                                  NoeudExpression const *decl_fonc,
                                  NoeudExpression const *decl_appel);

void lance_erreur_acces_hors_limites(EspaceDeTravail const &espace,
                                     NoeudExpression const *b,
                                     int64_t taille_tableau,
                                     Type const *type_tableau,
                                     int64_t index_acces);

void membre_inconnu(EspaceDeTravail const &espace,
                    NoeudExpression const *acces,
                    NoeudExpression const *membre,
                    TypeCompose const *type);

void valeur_manquante_discr(EspaceDeTravail const &espace,
                            NoeudExpression const *expression,
                            const kuri::ensemble<kuri::chaine_statique> &valeurs_manquantes);

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

    template <typename Fonction>
    Erreur &ajoute_donnees(Fonction rappel)
    {
        rappel(*this);
        return *this;
    }

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
                       SiteSource site,
                       const kuri::chaine &message,
                       erreur::Genre genre = erreur::Genre::NORMAL);

kuri::chaine genere_entete_erreur(EspaceDeTravail const *espace,
                                  SiteSource site,
                                  erreur::Genre genre,
                                  const kuri::chaine_statique message);
