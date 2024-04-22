/* SPDX-License-Identifier: GPL-2.0-or-later
 * The Original Code is Copyright (C) 2021 Kévin Dietrich. */

#pragma once

#include "arbre_syntaxique/prodeclaration.hh"

#include <cassert>
#include <variant>

struct Atome;
struct Attente;
struct EspaceDeTravail;
struct Fichier;
struct Message;
struct MetaProgramme;
struct UniteCompilation;

using Type = NoeudDéclarationType;

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
struct FichierÀCharger {
    Fichier *fichier;
};

struct FichierÀLexer {
    Fichier *fichier;
};

struct FichierÀParser {
    Fichier *fichier;
};

struct OpérateurPour {
    Type const *type;
};

struct InitialisationType {
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

struct SymboleAttendu {
    NoeudExpression *expression = nullptr;
};

using AttenteSurType = AttenteSur<Type const *>;
using AttenteSurMétaProgramme = AttenteSur<MetaProgramme *>;
using AttenteSurDéclaration = AttenteSur<NoeudDéclaration *>;
using AttenteSurSymbole = AttenteSur<SymboleAttendu>;
using AttenteSurOpérateur = AttenteSur<NoeudExpression *>;
using AttenteSurMessage = AttenteSur<DonnéesAttenteMessage>;
using AttenteSurRI = AttenteSur<Atome **>;
using AttenteSurChargement = AttenteSur<FichierÀCharger>;
using AttenteSurLexage = AttenteSur<FichierÀLexer>;
using AttenteSurParsage = AttenteSur<FichierÀParser>;
using AttenteSurNoeudCode = AttenteSur<DonnéesAttenteNoeudCode>;
using AttenteSurOpérateurPour = AttenteSur<OpérateurPour>;
using AttenteSurInitialisationType = AttenteSur<InitialisationType>;

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
DÉCLARE_INFO_TYPE_ATTENTE(déclaration, AttenteSurDéclaration);
DÉCLARE_INFO_TYPE_ATTENTE(opérateur, AttenteSurOpérateur);
DÉCLARE_INFO_TYPE_ATTENTE(métaprogramme, AttenteSurMétaProgramme);
DÉCLARE_INFO_TYPE_ATTENTE(ri, AttenteSurRI);
DÉCLARE_INFO_TYPE_ATTENTE(symbole, AttenteSurSymbole);
DÉCLARE_INFO_TYPE_ATTENTE(message, AttenteSurMessage);
DÉCLARE_INFO_TYPE_ATTENTE(chargement, AttenteSurChargement);
DÉCLARE_INFO_TYPE_ATTENTE(lexage, AttenteSurLexage);
DÉCLARE_INFO_TYPE_ATTENTE(parsage, AttenteSurParsage);
DÉCLARE_INFO_TYPE_ATTENTE(noeud_code, AttenteSurNoeudCode);
DÉCLARE_INFO_TYPE_ATTENTE(opérateur_pour, AttenteSurOpérateurPour);
DÉCLARE_INFO_TYPE_ATTENTE(initialisation_type, AttenteSurInitialisationType);

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
                                     AttenteSurMétaProgramme,
                                     AttenteSurDéclaration,
                                     AttenteSurSymbole,
                                     AttenteSurOpérateur,
                                     AttenteSurMessage,
                                     AttenteSurRI,
                                     AttenteSurChargement,
                                     AttenteSurLexage,
                                     AttenteSurParsage,
                                     AttenteSurNoeudCode,
                                     AttenteSurOpérateurPour,
                                     AttenteSurInitialisationType>;

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
        return AttenteSurLexage{FichierÀLexer{fichier}};
    }

    static Attente sur_chargement(Fichier *fichier)
    {
        assert(fichier);
        return AttenteSurChargement{FichierÀCharger{fichier}};
    }

    static Attente sur_parsage(Fichier *fichier)
    {
        assert(fichier);
        return AttenteSurParsage{FichierÀParser{fichier}};
    }

    static Attente sur_type(Type const *type)
    {
        assert(type);
        return AttenteSurType{type};
    }

    static Attente sur_métaprogramme(MetaProgramme *metaprogramme)
    {
        assert(metaprogramme);
        return AttenteSurMétaProgramme{metaprogramme};
    }

    static Attente sur_déclaration(NoeudDéclaration *déclaration)
    {
        assert(déclaration);
        return AttenteSurDéclaration{déclaration};
    }

    static Attente sur_symbole(NoeudExpression *ident)
    {
        assert(ident);
        return AttenteSurSymbole{SymboleAttendu{ident}};
    }

    static Attente sur_opérateur(NoeudExpression *operateur)
    {
        assert(operateur);
        return AttenteSurOpérateur{operateur};
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

    static Attente sur_initialisation_type(Type const *type)
    {
        assert(type);
        return AttenteSurInitialisationType{InitialisationType{type}};
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

    MetaProgramme *métaprogramme() const
    {
        assert(est<AttenteSurMétaProgramme>());
        return std::get<AttenteSurMétaProgramme>(attente).valeur;
    }

    NoeudDéclaration *déclaration() const
    {
        assert(est<AttenteSurDéclaration>());
        return std::get<AttenteSurDéclaration>(attente).valeur;
    }

    NoeudExpression *symbole() const
    {
        assert(est<AttenteSurSymbole>());
        return std::get<AttenteSurSymbole>(attente).valeur.expression;
    }

    NoeudExpression *opérateur() const
    {
        assert(est<AttenteSurOpérateur>());
        return std::get<AttenteSurOpérateur>(attente).valeur;
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

    Fichier *fichier_à_charger() const
    {
        assert(est<AttenteSurChargement>());
        return std::get<AttenteSurChargement>(attente).valeur.fichier;
    }

    Fichier *fichier_à_lexer() const
    {
        assert(est<AttenteSurLexage>());
        return std::get<AttenteSurLexage>(attente).valeur.fichier;
    }

    Fichier *fichier_à_parser() const
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

    Type const *initialisation_type() const
    {
        assert(est<AttenteSurInitialisationType>());
        return std::get<AttenteSurInitialisationType>(attente).valeur.type;
    }
};

/** \} */
