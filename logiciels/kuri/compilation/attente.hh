/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include <cassert>
#include <variant>

struct Atome;
struct Attente;
struct EspaceDeTravail;
struct Fichier;
struct Message;
struct MetaProgramme;
struct NoeudDeclaration;
struct NoeudExpression;
struct NoeudExpressionReference;
struct UniteCompilation;
struct NoeudDeclarationType;

using Type = NoeudDeclarationType;

namespace kuri {
struct chaine;
}

enum class PhaseCompilation : int;

/** -----------------------------------------------------------------
 * \{ */

/* Représente la condition pour laquelle l'attente est bloquée. */
struct ConditionBlocageAttente {
    PhaseCompilation phase{};
};

/** \} */

/** -----------------------------------------------------------------
 * « Table virtuelle » pour les opérations sur les attentes.
 * \{ */

struct InfoTypeAttente {
    /* Rappel pour retourner l'unité de compilation de l'objet attendu. Peut retourner nul si
     * l'objet n'a pas d'unité de compilation (par exemple un symbole indéfini n'a pas d'unité). */
    UniteCompilation *(*unité_pour_attente)(Attente const &attente);

    /* Rappel pour retourner la condition qui fait que l'attente est bloquée, que la compilation ne
     * peut continuer. */
    ConditionBlocageAttente (*condition_blocage)(Attente const &attente);

    /* Rappel pour retourner un commentaire décrivant ce sur quoi l'on attend. */
    kuri::chaine (*commentaire)(Attente const &attente);

    /* Rappel pour retourner si l'attente est résolue, que la compilation de l'unité peut
     * reprendre. */
    bool (*est_résolue)(EspaceDeTravail *espace, Attente &attente);

    /* Rappel pour émettre erreur selon l'attente. */
    void (*émets_erreur)(UniteCompilation const *unité, Attente const &attente);
};

/** \} */

/** -----------------------------------------------------------------
 * \{ */

template <typename T>
struct AttenteSur {
    T valeur;
};

// Types spéciaux pour attendre le chargement ou le lexage d'un fichier. Cette attente n'est
// possible que pour les fichiers devant être chargés plusieurs fois pour différents espaces (le
// chargement et le lexage sont uniques).
struct FichierACharger {
    Fichier *fichier;
};

struct FichierALexer {
    Fichier *fichier;
};

struct FichierAParser {
    Fichier *fichier;
};

struct OpérateurPour {
    Type const *type;
};

struct DonnéesAttenteNoeudCode {
    UniteCompilation *unité = nullptr;
    NoeudExpression *noeud = nullptr;
};

struct DonnéesAttenteMessage {
    UniteCompilation *unité = nullptr;
    Message *message = nullptr;
};

using AttenteSurType = AttenteSur<Type const *>;
using AttenteSurMetaProgramme = AttenteSur<MetaProgramme *>;
using AttenteSurDeclaration = AttenteSur<NoeudDeclaration *>;
using AttenteSurSymbole = AttenteSur<NoeudExpressionReference *>;
using AttenteSurOperateur = AttenteSur<NoeudExpression *>;
using AttenteSurMessage = AttenteSur<DonnéesAttenteMessage>;
using AttenteSurRI = AttenteSur<Atome **>;
using AttenteSurChargement = AttenteSur<FichierACharger>;
using AttenteSurLexage = AttenteSur<FichierALexer>;
using AttenteSurParsage = AttenteSur<FichierAParser>;
using AttenteSurNoeudCode = AttenteSur<DonnéesAttenteNoeudCode>;
using AttenteSurOpérateurPour = AttenteSur<OpérateurPour>;

/** \} */

/** -----------------------------------------------------------------
 * Déclaration d'InfoTypeAttente pour chaque type d'attente.
 * \{ */

template <typename T>
struct SélecteurInfoTypeAttente;

#define DÉCLARE_INFO_TYPE_ATTENTE(__nom__, __type__)                                              \
    extern InfoTypeAttente info_type_attente_sur_##__nom__;                                       \
    template <>                                                                                   \
    struct SélecteurInfoTypeAttente<__type__> {                                                   \
        static constexpr InfoTypeAttente *type = &info_type_attente_sur_##__nom__;                \
    }

DÉCLARE_INFO_TYPE_ATTENTE(type, AttenteSurType);
DÉCLARE_INFO_TYPE_ATTENTE(déclaration, AttenteSurDeclaration);
DÉCLARE_INFO_TYPE_ATTENTE(opérateur, AttenteSurOperateur);
DÉCLARE_INFO_TYPE_ATTENTE(métaprogramme, AttenteSurMetaProgramme);
DÉCLARE_INFO_TYPE_ATTENTE(ri, AttenteSurRI);
DÉCLARE_INFO_TYPE_ATTENTE(symbole, AttenteSurSymbole);
DÉCLARE_INFO_TYPE_ATTENTE(message, AttenteSurMessage);
DÉCLARE_INFO_TYPE_ATTENTE(chargement, AttenteSurChargement);
DÉCLARE_INFO_TYPE_ATTENTE(lexage, AttenteSurLexage);
DÉCLARE_INFO_TYPE_ATTENTE(parsage, AttenteSurParsage);
DÉCLARE_INFO_TYPE_ATTENTE(noeud_code, AttenteSurNoeudCode);
DÉCLARE_INFO_TYPE_ATTENTE(opérateur_pour, AttenteSurOpérateurPour);

#undef DÉCLARE_INFO_TYPE_ATTENTE

/** \} */

/** -----------------------------------------------------------------
 * \{ */

/* Représente une attente, c'est-à-dire ce dont une unité de compilation nécessite pour continuer
 * son chemin dans la compilation. */
struct Attente {
  protected:
    using TypeAttente = std::variant<std::monostate,
                                     AttenteSurType,
                                     AttenteSurMetaProgramme,
                                     AttenteSurDeclaration,
                                     AttenteSurSymbole,
                                     AttenteSurOperateur,
                                     AttenteSurMessage,
                                     AttenteSurRI,
                                     AttenteSurChargement,
                                     AttenteSurLexage,
                                     AttenteSurParsage,
                                     AttenteSurNoeudCode,
                                     AttenteSurOpérateurPour>;

    TypeAttente attente{};

  public:
    InfoTypeAttente *info = nullptr;

  protected:
    template <typename T>
    Attente(AttenteSur<T> attente_sur)
        : attente(attente_sur), info(SélecteurInfoTypeAttente<AttenteSur<T>>::type)
    {
    }

  public:
    /* Construction. */

    Attente() = default;

    static Attente sur_lexage(Fichier *fichier)
    {
        assert(fichier);
        return AttenteSurLexage{FichierALexer{fichier}};
    }

    static Attente sur_chargement(Fichier *fichier)
    {
        assert(fichier);
        return AttenteSurChargement{FichierACharger{fichier}};
    }

    static Attente sur_parsage(Fichier *fichier)
    {
        assert(fichier);
        return AttenteSurParsage{FichierAParser{fichier}};
    }

    static Attente sur_type(Type const *type)
    {
        assert(type);
        return AttenteSurType{type};
    }

    static Attente sur_metaprogramme(MetaProgramme *metaprogramme)
    {
        assert(metaprogramme);
        return AttenteSurMetaProgramme{metaprogramme};
    }

    static Attente sur_declaration(NoeudDeclaration *declaration)
    {
        assert(declaration);
        return AttenteSurDeclaration{declaration};
    }

    static Attente sur_symbole(NoeudExpressionReference *ident)
    {
        assert(ident);
        return AttenteSurSymbole{ident};
    }

    static Attente sur_operateur(NoeudExpression *operateur)
    {
        assert(operateur);
        return AttenteSurOperateur{operateur};
    }

    static Attente sur_message(UniteCompilation *unité, Message *message)
    {
        assert(message);
        return AttenteSurMessage{DonnéesAttenteMessage{unité, message}};
    }

    static Attente sur_ri(Atome **atome)
    {
        assert(atome);
        return AttenteSurRI{atome};
    }

    static Attente sur_noeud_code(UniteCompilation *unité, NoeudExpression *noeud)
    {
        assert(noeud);
        return AttenteSurNoeudCode{DonnéesAttenteNoeudCode{unité, noeud}};
    }

    static Attente sur_opérateur_pour(Type const *type)
    {
        assert(type);
        return AttenteSurOpérateurPour{OpérateurPour{type}};
    }

    /* Discrimination. */

    /* Retourne vrai si l'attente est valide, c'est-à-dire qu'elle contient quelque chose sur quoi
     * attendre. */
    bool est_valide() const
    {
        return attente.index() != std::variant_npos && attente.index() != 0;
    }

    template <typename T>
    bool est() const
    {
        return std::holds_alternative<T>(attente);
    }

    /* Accès. */

    Type const *type() const
    {
        assert(est<AttenteSurType>());
        return std::get<AttenteSurType>(attente).valeur;
    }

    MetaProgramme *metaprogramme() const
    {
        assert(est<AttenteSurMetaProgramme>());
        return std::get<AttenteSurMetaProgramme>(attente).valeur;
    }

    NoeudDeclaration *declaration() const
    {
        assert(est<AttenteSurDeclaration>());
        return std::get<AttenteSurDeclaration>(attente).valeur;
    }

    NoeudExpressionReference *symbole() const
    {
        assert(est<AttenteSurSymbole>());
        return std::get<AttenteSurSymbole>(attente).valeur;
    }

    NoeudExpression *operateur() const
    {
        assert(est<AttenteSurOperateur>());
        return std::get<AttenteSurOperateur>(attente).valeur;
    }

    DonnéesAttenteMessage message() const
    {
        assert(est<AttenteSurMessage>());
        return std::get<AttenteSurMessage>(attente).valeur;
    }

    Atome **ri() const
    {
        assert(est<AttenteSurRI>());
        return std::get<AttenteSurRI>(attente).valeur;
    }

    Fichier *fichier_a_charger() const
    {
        assert(est<AttenteSurChargement>());
        return std::get<AttenteSurChargement>(attente).valeur.fichier;
    }

    Fichier *fichier_a_lexer() const
    {
        assert(est<AttenteSurLexage>());
        return std::get<AttenteSurLexage>(attente).valeur.fichier;
    }

    Fichier *fichier_a_parser() const
    {
        assert(est<AttenteSurParsage>());
        return std::get<AttenteSurParsage>(attente).valeur.fichier;
    }

    DonnéesAttenteNoeudCode noeud_code() const
    {
        assert(est<AttenteSurNoeudCode>());
        return std::get<AttenteSurNoeudCode>(attente).valeur;
    }

    Type const *opérateur_pour() const
    {
        assert(est<AttenteSurOpérateurPour>());
        return std::get<AttenteSurOpérateurPour>(attente).valeur.type;
    }
};

/** \} */
