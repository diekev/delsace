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
 * The Original Code is Copyright (C) 2021 Kévin Dietrich.
 * All rights reserved.
 *
 * ***** END GPL LICENSE BLOCK *****
 *
 */

#pragma once

#include <cassert>
#include <variant>

struct Atome;
struct Fichier;
struct IdentifiantCode;
struct Message;
struct MetaProgramme;
struct NoeudCode;
struct NoeudDeclaration;
struct NoeudExpression;
struct NoeudExpressionReference;
struct Type;

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

using AttenteSurType = AttenteSur<Type *>;
using AttenteSurInterfaceKuri = AttenteSur<IdentifiantCode *>;
using AttenteSurMetaProgramme = AttenteSur<MetaProgramme *>;
using AttenteSurDeclaration = AttenteSur<NoeudDeclaration *>;
using AttenteSurSymbole = AttenteSur<NoeudExpressionReference *>;
using AttenteSurOperateur = AttenteSur<NoeudExpression *>;
using AttenteSurMessage = AttenteSur<Message *>;
using AttenteSurRI = AttenteSur<Atome **>;
using AttenteSurChargement = AttenteSur<FichierACharger>;
using AttenteSurLexage = AttenteSur<FichierALexer>;
using AttenteSurParsage = AttenteSur<FichierAParser>;
using AttenteSurNoeudCode = AttenteSur<NoeudCode **>;

/* Représente une attente, c'est-à-dire ce dont une unité de compilation nécessite pour continuer
 * son chemin dans la compilation. */
struct Attente {
  protected:
    using TypeAttente = std::variant<std::monostate,
                                     AttenteSurType,
                                     AttenteSurInterfaceKuri,
                                     AttenteSurMetaProgramme,
                                     AttenteSurDeclaration,
                                     AttenteSurSymbole,
                                     AttenteSurOperateur,
                                     AttenteSurMessage,
                                     AttenteSurRI,
                                     AttenteSurChargement,
                                     AttenteSurLexage,
                                     AttenteSurParsage,
                                     AttenteSurNoeudCode>;

    TypeAttente attente{};

    template <typename T>
    Attente(AttenteSur<T> attente_sur) : attente(attente_sur)
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

    static Attente sur_type(Type *type)
    {
        assert(type);
        return AttenteSurType{type};
    }

    static Attente sur_interface_kuri(IdentifiantCode *nom_fonction)
    {
        assert(nom_fonction);
        return AttenteSurInterfaceKuri{nom_fonction};
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

    static Attente sur_message(Message *message)
    {
        assert(message);
        return AttenteSurMessage{message};
    }

    static Attente sur_ri(Atome **atome)
    {
        assert(atome);
        return AttenteSurRI{atome};
    }

    static Attente sur_noeud_code(NoeudCode **code)
    {
        assert(code);
        return AttenteSurNoeudCode{code};
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

    Type *type() const
    {
        assert(est<AttenteSurType>());
        return std::get<AttenteSurType>(attente).valeur;
    }

    IdentifiantCode *interface_kuri() const
    {
        assert(est<AttenteSurInterfaceKuri>());
        return std::get<AttenteSurInterfaceKuri>(attente).valeur;
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

    Message *message() const
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

    NoeudCode **noeud_code() const
    {
        assert(est<AttenteSurNoeudCode>());
        return std::get<AttenteSurNoeudCode>(attente).valeur;
    }
};
